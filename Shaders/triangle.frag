#version 450

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_uv;

layout(location = 0) out vec4 final_color;

layout(set=2, binding=0) uniform sampler2D tex_sampler;

void main()
{
	final_color = texture(tex_sampler, frag_uv) * vec4(frag_color, 1.0);
}