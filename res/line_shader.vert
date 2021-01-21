#version 430 core

layout(location = 0) in ivec3 in_position;

uniform mat4 u_view_proj;
uniform mat4 u_model;

out vec4 frag_color;

void main()
{
	gl_Position = u_view_proj * u_model * vec4(in_position, 1);
	frag_color = vec4(0,1,0,1);
}
