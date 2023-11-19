#version 410

struct ViewProjTransforms
{
	mat4 view_projection;
	mat4 view_projection_inverse;
};

layout (std140) uniform CameraViewProjTransforms
{
	ViewProjTransforms camera;
};

layout (std140) uniform LightViewProjTransforms
{
	ViewProjTransforms lights[4];
};

uniform int light_index;

uniform sampler2D depth_texture;
uniform sampler2D normal_texture;
uniform sampler2D shadow_texture;

uniform vec2 inverse_screen_resolution;

uniform vec3 camera_position;

uniform vec3 light_color;
uniform vec3 light_position;
uniform vec3 light_direction;
uniform float light_intensity;
uniform float light_angle_falloff;

layout (location = 0) out vec4 light_diffuse_contribution;
layout (location = 1) out vec4 light_specular_contribution;


void main()
{
	vec2 shadowmap_texel_size = 1.0f / textureSize(shadow_texture, 0);
	vec2 texcoords;
	texcoords.x = gl_FragCoord.x * inverse_screen_resolution.x;
	texcoords.y = gl_FragCoord.y * inverse_screen_resolution.y;
	vec3 normal = (2*vec3(texture(normal_texture, texcoords))-vec3(1.0));
	float depth = texture(depth_texture, texcoords).r; // Fetches the z value from depth buffer;
	vec4 proj_pos = vec4(texcoords, depth, 1.0) * 2 - 1; // takes the x,y of our view, and adds the z from depth buffer. changes from [0,1] -> [-1, 1]
	vec4 world_pos = vec4(camera.view_projection_inverse * proj_pos); //camera->world
	vec3 vertex_pos = world_pos.xyz / world_pos.w;
	vec3 L = normalize(-light_direction);
	vec3 r = reflect(-L, normal);
	vec3 v = normalize(camera_position - vertex_pos.xyz);
	vec3 light_to_vertex = light_position - vertex_pos.xyz;
	float distance= length(light_to_vertex); //get the distance from light to point
	float distance_fall_off = 1/(distance*distance); //inverse-square law
	float vertex_angle = acos(dot(L, light_to_vertex)/(length(L)*distance)); // [0, pi]
	float angle = clamp(vertex_angle, 0, light_angle_falloff); //sets angle to [0, light_angle_falloff]
	float angular_fall_off = smoothstep(light_angle_falloff,0,angle); //[0,1] where 1 is bright, and 0 is none
	float total_fall_off = light_intensity*distance_fall_off * angular_fall_off;
	light_diffuse_contribution  = vec4(light_color * max(dot(normal,L), 0), 1.0) * total_fall_off;
	light_specular_contribution = vec4(light_color * max(dot(r,v), 0),1.0) * total_fall_off ;
	
	//--- SHADOWS ---

	mat4 shadow_projection = lights[light_index].view_projection;
	vec4 shadow_pos = shadow_projection * vec4(vertex_pos, 1.0f);
	vec3 shadow_vertex= shadow_pos.xyz / shadow_pos.w;
	shadow_vertex = shadow_vertex * 0.5f + 0.5f;
	float shadow_depth = texture(shadow_texture, shadow_vertex.xy).r;


	if (shadow_vertex.z-0.001 > shadow_depth){ //If spot it in the shadow -> check surrounding area
	
		int window_size = 6; // should be n
		int loop_range = window_size/2;
		float n_lit = window_size*window_size; //amount of squares, assume all in light
		for (int i = -loop_range; i < loop_range; ++i){
			for (int j = -loop_range; j < loop_range; ++j){

				//--- Basic Shadow Check for nearby pixels---
				vec2 temp = shadow_vertex.xy;
				temp.x += (shadowmap_texel_size.x*(i+0.5));
				temp.y += (shadowmap_texel_size.y*(j+0.5));
				float shadow_depth = texture(shadow_texture, temp.xy).r;

				if (shadow_vertex.z-0.001 > shadow_depth){ //If nearby spot it in the shadow
					n_lit -= 1.0;
				}
			}
		}

	float shaded_ratio = n_lit/(window_size*window_size);
	light_diffuse_contribution  *= shaded_ratio; //our calculated float
	light_specular_contribution *= shaded_ratio;
	light_diffuse_contribution.w = 1.0;
	light_specular_contribution.w = 1.0;
	}
		//--- Basic Shadow ---

	
	// light_diffuse_contribution = vec4(shadow_vertex.xy/shadowmap_texel_size, 0.0,1.0); //Test

	//--- Percentage Closer Filtering ---

	//float shadow_sum = 0;
	//float window_size = 2;

	//for (int a = 0; a < window_size; ++a){
		//for (int b = 0; b < window_size; ++b){

		// Test2: THIS IS CORRECT!
			//vec4 shadow_pos = shadow_projection * vec4(vertex_pos.x -window_size/2 + a, vertex_pos.y -window_size/2 + b,vertex_pos.z, 1.0f);
			//vec3 shadow_vertex= shadow_pos.xyz / shadow_pos.w ;
			//shadow_vertex =  shadow_vertex * 0.5f + 0.5f;
			//float shadow_depth = texture(shadow_texture, shadow_vertex.xy).r;

			// Test1: Use same shadow_vertex for and just sample differnet depths. - DOESN'T SEEM TO WORK!
			//float shadow_depth = texture(shadow_texture, vec2(shadow_vertex.x  -1 + a, shadow_vertex.y -1 +b)).r; //Take a step along coordinates
	
			//if (shadow_vertex.z-0.001 > shadow_depth){ //if nearby square is in shadow
				//light_diffuse_contribution -= light_diffuse_contribution/(window_size*window_size); // this is incorrect, now we will 1-(3/4)-(3/16)-(3/64)
				//light_specular_contribution -= light_specular_contribution/(window_size*window_size); //make square a little darker
				//shadow_sum++;
			//}
		//}
	//}
	//light_diffuse_contribution *= (1.0-shadow_sum);
	//light_specular_contribution *= (1.0-shadow_sum);

	//----- all of this is probably useless now -----


	//shadow_sum/(4); //normalize
	//If all nearby squares are in shadow -> shadow_sum = 1
	//If no nearby squares are in shadow -> shadow_sum = 0

	// Testing! - Correct results here!!
	//shadow_sum = 0; //no shadows
	//shadow_sum = 1; //all shadows


	/*THIS IS PROBABLY THE ISSUE... NOT SURE HOW TO DECIDE COLORS...
	If full in shadow -> vec4(0.001,0.001,0.001,1.0)
	If fully in light -> vec4(light_color * max(dot(normal,L), 0), 1.0) * total_fall_off
	*/
	//light_diffuse_contribution  =+ vec4(0.01,0.01,0.01,1.0); // + (1-shadow_sum) * vec4(light_color * max(dot(normal,L), 0), 1.0) * total_fall_off;
	
	//light_specular_contribution =+ vec4(0.01,0.01,0.01,1.0); // + (1-shadow_sum) * vec4(light_color * max(dot(r,v), 0),1.0) * total_fall_off ;
}




