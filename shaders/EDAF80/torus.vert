#version 410

layout (location = 0) in vec3 vertex;
layout (location = 2) in vec3 texcoords;

uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;

out vec2 tex_pos;

void main()
{
	tex_pos = texcoords.xy;
	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}
