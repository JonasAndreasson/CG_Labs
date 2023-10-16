#version 410

layout (location = 0) in vec3 vertex;
layout (location = 2) in vec3 tex;
uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;
out vec2 fragCoord;

void main()
{
fragCoord = tex.xy;
gl_Position = vertex_model_to_world*vertex_world_to_clip*vec4(vertex, 1.0);
}
