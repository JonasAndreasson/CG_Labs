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
	vec4 world_pos = vec4(camera.view_projection_inverse * proj_pos);
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
	light_specular_contribution = vec4(light_color * max(dot(r,v), 0),1.0) * total_fall_off;
}

