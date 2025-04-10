#version 450

layout(location = 0) out vec3 cubemap_frag;

layout(set=1, binding = 0)uniform VBO {
	mat4 proj;
};

// Skybox cube vertices
vec3 skybox_pos[36] = vec3[](
	vec3(-1.0,  1.0, -1.0),
	vec3(-1.0, -1.0, -1.0),
	vec3( 1.0, -1.0, -1.0),
	vec3( 1.0, -1.0, -1.0),
	vec3( 1.0,  1.0, -1.0),
	vec3(-1.0,  1.0, -1.0),

	vec3(-1.0, -1.0,  1.0),
	vec3(-1.0, -1.0, -1.0),
	vec3(-1.0,  1.0, -1.0),
	vec3(-1.0,  1.0, -1.0),
	vec3(-1.0,  1.0,  1.0),
	vec3(-1.0, -1.0,  1.0),

	vec3( 1.0, -1.0, -1.0),
	vec3( 1.0, -1.0,  1.0),
	vec3( 1.0,  1.0,  1.0),
	vec3( 1.0,  1.0,  1.0),
	vec3( 1.0,  1.0, -1.0),
	vec3( 1.0, -1.0, -1.0),

	vec3(-1.0, -1.0,  1.0),
	vec3(-1.0,  1.0,  1.0),
	vec3( 1.0,  1.0,  1.0),
	vec3( 1.0,  1.0,  1.0),
	vec3( 1.0, -1.0,  1.0),
	vec3(-1.0, -1.0,  1.0),

	vec3(-1.0,  1.0, -1.0),
	vec3( 1.0,  1.0, -1.0),
	vec3( 1.0,  1.0,  1.0),
	vec3( 1.0,  1.0,  1.0),
	vec3(-1.0,  1.0,  1.0),
	vec3(-1.0,  1.0, -1.0),

	vec3(-1.0, -1.0, -1.0),
	vec3(-1.0, -1.0,  1.0),
	vec3( 1.0, -1.0, -1.0),
	vec3( 1.0, -1.0, -1.0),
	vec3(-1.0, -1.0,  1.0),
	vec3( 1.0, -1.0,  1.0)
);

void main()
{
	gl_Position = proj * vec4(skybox_pos[gl_VertexIndex], 1.0);
	gl_Position.z = gl_Position.w;
	cubemap_frag = skybox_pos[gl_VertexIndex]; 
}