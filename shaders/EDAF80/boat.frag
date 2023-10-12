#version 410

uniform float elapsed_time_s;
uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform vec3 light_position;
uniform vec3 camera_position;
uniform vec3 diffuse_colour;
uniform vec3 specular_colour;
uniform vec3 ambient_colour;
uniform vec3 emissive_colour;
uniform float shininess_value;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec3 binormal;
	vec3 tangent;
	vec2 texture;
} fs_in;

out vec4 frag_color;


void main()
{
	mat3 tbn = mat3(normalize(fs_in.tangent), normalize(fs_in.binormal), normalize(fs_in.normal));
	vec3 norm = normalize(fs_in.normal);
	vec3 V_norm = normalize(camera_position-fs_in.vertex);
	vec3 L = normalize(light_position-fs_in.vertex);
	vec3 specular = vec3(1.0);
	float K = dot(reflect(-L, norm),V_norm);
	specular = ambient_colour * pow(max(K,0), shininess_value);
	vec3 diffuse = specular_colour * max(dot(norm,L), 0);
	vec3 ambient = diffuse_colour;
	frag_color = vec4(ambient,1.0)+
				vec4(diffuse,1.0) +
				vec4(specular,1.0);

}




