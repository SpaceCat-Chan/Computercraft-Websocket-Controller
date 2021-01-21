#version 430 core

in vec4 frag_color;
in vec2 frag_uv;

out vec4 out_color;

uniform sampler2D u_Texture;

void main()
{
	out_color = texture(u_Texture, frag_uv) * frag_color;
	if(out_color.a == 0)
	{
		//basic transparancy
		discard;
	}
}
