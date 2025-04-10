#version 450

layout(location = 0) in vec3 cubemap_frag;

layout(location = 0) out vec4 final_color;

layout(set=2, binding=0) uniform samplerCube cube_sampler;

void main()
{
	final_color = texture(cube_sampler, cubemap_frag);
}