#version 430 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;

layout(std430, binding = 0) buffer offset_b
{
	ivec4 offset[];
};

layout(std430, binding = 1) buffer color_b
{
	vec4 color[];
};

uniform mat4 u_view_proj;
uniform mat4 u_model;

out vec4 frag_color;
out vec2 frag_uv;

void main()
{
	vec3 vert_position = vec3(u_model * vec4(in_position, 1));
	switch(offset[gl_InstanceID].w)
	{
		case 0:
			break;
		case 1:
			vert_position = vert_position.zyx * vec3(-1,1,1);
			break;
		case 2:
			vert_position = vert_position.xyz * vec3(-1,1,-1);
			break;
		case 3:
			vert_position = vert_position.zyx * vec3(1,1,-1);
			break;
	}
	gl_Position = u_view_proj * vec4(vert_position + offset[gl_InstanceID].xyz, 1);
	frag_color = color[gl_InstanceID];
	frag_uv = in_uv;
}
