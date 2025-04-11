#version 450

layout(set=3, binding = 0)uniform UBO {
	vec3 object_color_base;
	vec3 incoming_light_color;
	vec3 sun_direction;
};

layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 frag_uv;
layout(location = 2) in vec3 frag_pos;

layout(location = 0) out vec4 final_color;

layout(set=2, binding=0) uniform sampler2D tex_sampler;

void main()
{
	vec3 light_dir = normalize(vec3(-sun_direction.x, -sun_direction.y, -sun_direction.z));

	const float ambient_strength = 0.4;
	vec3 ambient = ambient_strength * incoming_light_color;

	float diffuse_intensity = max(dot(normal, light_dir), 0.0);
	vec3 diffuse = diffuse_intensity * incoming_light_color;

	vec3 final_result = (ambient + diffuse) * object_color_base;

	final_color = vec4(final_result, 1.0);
}