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
	vec3 n = (2*vec3(texture(normal_texture, frag_pos))-vec3(1.0));
	vec4 ws_pos = texture(depth_texture, frag_pos)*camera.view_projection_inverse;
	ws_pos = ws_pos/gl_FragCoord.w;
	vec3 light = light_position-ws_pos.xyz;
	//light_diffuse_contribution  = vec4(vec3(1.0,1.0,1.0) * max(dot(light,n), 0), 1.0);
	vec3 r = reflect(-light, n.xyz);
	vec3 v = camera_position - ws_pos.xyz;
	light_specular_contribution = vec4(vec3(1.0,1.0,1.0) * max(dot(r,v), 0),1.0);
	light_diffuse_contribution = vec4(0.0,0.0,0.0,1.0);
	light_specular_contribution = vec4(vec3(frag_pos.y, 0.0, 0.0),1.0);
}

