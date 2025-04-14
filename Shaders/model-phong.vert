#version 450

layout(set=1, binding = 0)uniform UBO {
	mat4 model;
	mat4 mvp;
};

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;

layout(location = 0) out vec3 normal;
layout(location = 1) out vec2 frag_uv;
layout(location = 2) out vec3 frag_pos;

void main()
{
	gl_Position = mvp * vec4(a_pos, 1.0);
	normal = mat3(transpose(inverse(model))) * a_normal;
	frag_uv = a_uv;
	frag_pos = vec3(model * vec4(a_pos, 1.0));
}