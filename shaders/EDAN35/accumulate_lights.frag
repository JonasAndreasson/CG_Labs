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
	//light_diffuse_contribution  = vec4(light_color * max(dot(normal,L), 0), 1.0) * total_fall_off;
	//light_specular_contribution = vec4(light_color * max(dot(r,v), 0),1.0) * total_fall_off ;
	
	//SHADOWS

	mat4 shadow_projection = lights[light_index].view_projection;

	vec4 shadow_pos = shadow_projection * vec4(vertex_pos, 1.0f);
	vec3 shadow_vertex= shadow_pos.xyz / shadow_pos.w;
	shadow_vertex = shadow_vertex * 0.5f + 0.5f;
	float shadow_depth = texture(shadow_texture, shadow_vertex.xy).r;
	
	//if (shadow_vertex.z-0.001 > shadow_depth){ //If spot it in the shadow
	//light_diffuse_contribution  = vec4(0.0,0.0,0.0,1.0);
	//light_specular_contribution = vec4(0.0,0.0,0.0,1.0);}
	
	//light_diffuse_contribution = vec4(shadow_vertex.xy/shadowmap_texel_size, 0.0,1.0);

	/* LAB1------------
	for (int j = 0; j < imageHeight; ++j) {
        for (int i = 0; i < imageWidth; ++i) {

            Color pixel;
            // Get center of pixel coordinate
            for (int x = 0; x < 3; ++x) {
                for (int y = 0; y < 3; ++y) {

                    
                    float cx = ((float)i) + (1 + 2 * x) / 6.0f + 2 * (uniform() - 0.5)/6.0f;
                    float cy = ((float)j) + (1 + 2 * y) / 6.0f + 2 * (uniform() - 0.5)/6.0f;

                    // Get a ray and trace it
                    Ray r = camera.getRay(cx, cy);
                    pixel += traceRay(r, scene, depth);
                }
            }
            pixel *= (1.0f/9.0f); 
            // Write pixel value to image
            writeColor((j * imageWidth + i) * numChannels, pixel, pixels);
        }
    }
	---------------
*/
	
	float shadow_sum;

	for (int a = 0; a < 2; a++){
		for (int b = 0; b < 2; b++){

			float temp_depth = texture(shadow_texture, vec2(shadow_vertex.x  -1 + a, shadow_vertex.y -1 +b)).r; //Take a step along coordinates
	
			if (shadow_vertex.z-0.001 > temp_depth){ //If in shadow - wrong shadow_vertex.z.....?
				shadow_sum ++; //count how many of nearby squares are blocked/shaded
			}
		}
	}

	shadow_sum/(4); //normalize
	//If all nearby squares are in shadow -> shadow_sum = 1
	//If no nearby squares are in shadow -> shadow_sum = 0

	// Testing! - Correct results!!
	//shadow_sum = 0; //no shadows
	//shadow_sum = 1; //all shadows


	//This is no different!!!
	light_diffuse_contribution  = shadow_sum* vec4(0.0,0.0,0.0,1.0) + (1-shadow_sum) * vec4(light_color * max(dot(normal,L), 0), 1.0) * total_fall_off;
	light_specular_contribution = shadow_sum* vec4(0.0,0.0,0.0,1.0) + (1-shadow_sum) * vec4(light_color * max(dot(r,v), 0),1.0) * total_fall_off ;
}




