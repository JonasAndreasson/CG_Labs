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
	vec2 frag_pos;
	frag_pos.x = gl_FragCoord.x * inverse_screen_resolution.x;
	frag_pos.y = gl_FragCoord.x * inverse_screen_resolution.y;
	vec4 n = texture(normal_texture, frag_pos);
	n = 2*n-1;
	light_diffuse_contribution  = n;
	light_specular_contribution = vec4(0.0, 0.0, 0.0, 1.0);
}

