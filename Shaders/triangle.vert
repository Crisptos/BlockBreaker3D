#version 450

layout(location = 0) out vec3 frag_color;

vec3 positions[3] = vec3[](
	vec3(0.0, -0.4, 0.0),
	vec3(0.4, 0.4, 0.0),
	vec3(-0.4, 0.4, 0.0)
);

vec3 colors[3] = vec3[](
	vec3(0.45, 1.0, 0.23),
	vec3(0.56, 0.65, 1.0),
	vec3(0.45, 1.0, 1.0)
);

void main()
{
	gl_Position = vec4(positions[gl_VertexIndex], 1.0);
	frag_color = colors[gl_VertexIndex];
}