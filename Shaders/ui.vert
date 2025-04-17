#version 450

layout(set=1, binding = 0)uniform UBO {
	mat4 proj;
};

layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_color;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec2 frag_uv;

void main()
{
	gl_Position = proj * vec4(a_pos.xy, 0.0, 1.0);
	frag_color = a_color;
	frag_uv = a_uv;
}