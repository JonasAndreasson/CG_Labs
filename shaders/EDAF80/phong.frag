#version 410
uniform vec3 light_position;
uniform vec3 camera_position;
uniform int use_normal_mapping;
uniform mat4 normal_model_to_world;

uniform sampler2D diff_texture;
uniform sampler2D specular_texture;
uniform sampler2D normal_texture;


uniform vec3 ambient_colour;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess_value;

in VS_OUT {
	vec3 V;
	vec3 n;
	vec2 texcoord;
} fs_in;

out vec4 frag_color;

void main(){
	vec3 norm = normalize(fs_in.n);
	vec3 V_norm = normalize(fs_in.V);
	vec3 L = normalize(light_position);
	vec4 specular = vec4(1.0);
	if (use_normal_mapping==1){
		norm = (normal_model_to_world * vec4(2*vec3(texture(normal_texture, fs_in.texcoord))-vec3(1.0),0)).xyz;
	}
	float K = dot(reflect(-L, norm),V_norm);
	specular = texture(specular_texture,fs_in.texcoord) * pow(max(K,0), shininess_value);
	vec4 diffuse = texture(diff_texture,fs_in.texcoord) * max(dot(norm,L), 0);
	frag_color = vec4(ambient_colour,1.0) +
				diffuse +
				specular;
}
