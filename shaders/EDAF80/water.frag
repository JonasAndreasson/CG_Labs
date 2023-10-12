#version 410


in VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec3 binormal;
	vec3 tangent;
	vec2 texture;
} fs_in;

uniform samplerCube cubemap;
uniform sampler2D wave_texture;
uniform float elapsed_time_s;
uniform vec3 camera_position;

out vec4 frag_color;

float fresnel_calc(in float R0, in vec3 V, in vec3 n)
{
	return R0 + (1 - R0)*pow(1-dot(V,n),5);
}

vec3 ripples(in vec3 texCoord)
{
	vec2 texScale = vec2(8, 4);
	float normalTime = mod(elapsed_time_s, 100.0);
	vec2 normalSpeed = vec2(-0.05, 0.0);
	vec2 normalCoord0 = texCoord.xy * texScale + normalTime * normalSpeed;
	vec2 normalCoord1 = texCoord.xy * texScale * 2 + normalTime * normalSpeed * 4;
	vec2 normalCoord2 = texCoord.xy * texScale * 4 + normalTime * normalSpeed * 8;
	vec4 n0 = 2*texture(wave_texture, normalCoord0)-1;
	vec4 n1 = 2*texture(wave_texture, normalCoord1)-1;
	vec4 n2 = 2*texture(wave_texture, normalCoord2)-1;
	vec3 n_bump = normalize(vec4(n0+n1+n2).xyz);
	mat3 tbn = mat3(normalize(fs_in.tangent), normalize(fs_in.binormal),normalize(fs_in.normal));
	return normalize(tbn*n_bump);
}

void main()
{
	vec3 V = normalize(camera_position-fs_in.vertex);
	vec3 n = ripples(vec3(fs_in.texture, 0.0)); //bump map

	if (gl_FrontFacing) {
	vec4 color_deep = vec4(0.0,0.0,0.1,1.0);
	vec4 color_shallow = vec4(0.0,0.5,0.5,1.0);
	float facing = 1 - max(dot(V,n), 0);
	vec4 water_color = mix(color_deep, color_shallow, facing);
	vec3 R = normalize(reflect(-V, n));
	vec4 reflection = texture(cubemap, R);
	float eta = 1.0/1.33;
	vec4 refraction = texture(cubemap,	normalize(refract(-V, n, eta)));
	float fresnel = fresnel_calc(0.02037, V, n);
	frag_color = water_color + reflection*fresnel +refraction * (1-fresnel);
	}
	else {
	vec3 R = normalize(reflect(V, n));
	vec4 reflection = texture(cubemap, R);
	float eta = 1.33/1.0;
	vec4 refraction = texture(cubemap,	normalize(refract(-V, -n, eta)));
	float fresnel = fresnel_calc(0.02037, -V, n);
	frag_color = reflection*fresnel +refraction * (1-fresnel);


	}
}
