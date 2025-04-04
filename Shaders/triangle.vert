#version 450

layout(location = 0) out vec3 frag_color;

layout(set=1, binding = 0)uniform VBO {
	mat4 mvp;
};

vec3 positions[3] = vec3[](
	vec3(0.0, 0.5, 0.0),
	vec3(0.5, -0.5, 0.0),
	vec3(-0.5, -0.5, 0.0)
);

vec3 colors[3] = vec3[](
	vec3(0.95, 0.0, 0.0),
	vec3(0.0, 0.95, 0.0),
	vec3(0.0, 0.0, 0.95)
);

void main()
{
	gl_Position = mvp * vec4(positions[gl_VertexIndex], 1.0);
	frag_color = colors[gl_VertexIndex];
}