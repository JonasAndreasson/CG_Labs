#version 410

uniform sampler2D rainbow;
in vec2 tex_pos;

out vec4 frag_color;


void main()
{
	
	frag_color = texture(rainbow, tex_pos);
}
