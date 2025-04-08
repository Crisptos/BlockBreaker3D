#version 450

layout(set=1, binding = 0)uniform VBO {
	mat4 mvp;
};

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_color;
layout(location = 2) in vec2 a_uv;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_uv;

void main()
{
	gl_Position = mvp * vec4(a_pos, 1.0);
	frag_color = a_color;
	frag_uv = a_uv;
}