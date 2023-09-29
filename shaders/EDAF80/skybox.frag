#version 410

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform int has_textures;
uniform int has_diffuse_texture;
uniform int has_opacity_texture;
uniform samplerCube cubemap;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
} fs_in;

out vec4 frag_color;

void main(){
	frag_color = texture(cubemap, fs_in.normal);
}
