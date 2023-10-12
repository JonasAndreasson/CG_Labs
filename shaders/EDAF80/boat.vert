#version 410
layout (location = 0) in vec3 vertex;
layout (location = 2) in vec3 texcoords;

uniform float elapsed_time_s;
uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform vec3 camera_position;
out VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec3 binormal;
	vec3 tangent;
	vec2 texture;
} vs_out;

void wave_height(in float x, in float z, in float t, in float A, in float Dx, in float Dz, in float f, in float p, in float k,out float y, out float dgdx, out float dgdz){
	float sinexpr = 0.5*sin(f*(Dx*x+Dz*z)+t*p)+0.5;
	y = A*pow(sinexpr,k);

	float cos_expr = cos(f*(Dx*x + Dz*z)+t*p);
	float dg = 0.5*k*f*A*pow(sinexpr, (k-1))*cos_expr;
	dgdx = dg*Dx;
	dgdz = dg*Dz;

}



void main()
{

	vec4 vertex_displaced = vertex_model_to_world * vec4(vertex, 1.0);
	float y1 = 0;
	float dgdx1 = 0;
	float dgdz1 = 0;
	float A1 = 1.0;
	float Dx1 = -1.0;
	float Dz1 = 0.0;
	float f1 = 0.2;
	float p1 = 0.5;
	float k1 = 2.0;
	wave_height(vertex_displaced.x, vertex_displaced.z, elapsed_time_s,A1,Dx1,Dz1,f1,p1,k1,y1, dgdx1, dgdz1);
	float y2 = 0;
	float dgdx2 = 0;
	float dgdz2 = 0;
	float A2 = 0.5;
	float Dx2 = -0.7;
	float Dz2 = 0.7;
	float f2 = 0.4;
	float p2 = 1.3;
	float k2 = 2.0;
	wave_height(vertex_displaced.x, vertex_displaced.z, elapsed_time_s,A2,Dx2,Dz2,f2,p2,k2,y2, dgdx2, dgdz2);

	float y = y1 + y2;
	float dhdx = dgdx1 + dgdx2;
	float dhdz = dgdz1 + dgdz2;
	vs_out.vertex = vec3(vertex_displaced.x,vertex_displaced.y+y,vertex_displaced.z);
	vs_out.binormal = vec3(0.0, dhdz,1.0);	
	vs_out.tangent = vec3(1.0, dhdx, 0.0);
	vs_out.normal = vec3(-dhdx, 1.0, -dhdz);
	vs_out.texture = texcoords.xy;
	gl_Position = vertex_world_to_clip * vec4(vs_out.vertex, 1.0);
}




