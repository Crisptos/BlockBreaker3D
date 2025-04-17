#version 450

layout(location = 0) in vec2 frag_uv;

layout(location = 0) out vec4 final_color;

layout(set=2, binding=0) uniform sampler2D tex_sampler;

void main()
{
	final_color = vec4(0.96, 0.96, 0.96, 0.96) * texture(tex_sampler, frag_uv);
}