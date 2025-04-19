#version 450

layout(location = 0) in vec4 frag_color;
layout(location = 1) in vec2 frag_uv;

layout(location = 0) out vec4 final_color;

layout(set=2, binding=0) uniform sampler2D tex_sampler;

void main()
{
	vec4 prefinal_color = frag_color * texture(tex_sampler, frag_uv);
	if(prefinal_color.a < 0.1)
	{
		discard;
	}
	final_color = prefinal_color;
}