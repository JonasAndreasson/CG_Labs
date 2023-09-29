#version 410
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;
uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

out VS_OUT {
	vec3 V;
	vec3 n;
	vec2 texcoord;
	vec3 tangent;
	vec3 binormal;
} vs_out;

void main()
{
	vs_out.texcoord = vec2(texcoord.x, texcoord.y);
	vs_out.V = vec3(vertex_model_to_world * vec4(vertex, 1.0));
	vs_out.n = vec3(normal_model_to_world * vec4(normal, 0.0));
	vs_out.tangent = vec3(normal_model_to_world * vec4(tangent, 0.0));
	vs_out.binormal = vec3(normal_model_to_world * vec4(binormal, 0.0));


	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}
