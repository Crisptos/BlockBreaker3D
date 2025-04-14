#version 450

// GLSL pads vec3 to 16 bytes per std140, caused a fun lighting bug
layout(set=3, binding = 0)uniform UBO {
	vec3 object_color_base;
	vec3 incoming_light_color;
	vec3 light_pos;
	vec3 view_pos;
};

layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 frag_uv;
layout(location = 2) in vec3 frag_pos;

layout(location = 0) out vec4 final_color;

layout(set=2, binding=0) uniform sampler2D tex_sampler;

void main()
{
	const float ambient_strength = 0.2;
	vec3 ambient = ambient_strength * incoming_light_color;

	vec3 norm = normalize(normal);
	vec3 light_dir = normalize(light_pos - frag_pos);
	float diff = max(dot(norm, light_dir), 0.0);
	vec3 diffuse = diff * incoming_light_color;

	float specular_strength = 0.5;
	vec3 view_dir = normalize(view_pos - frag_pos);
	vec3 reflect_dir = reflect(-light_dir, norm);
	float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
	vec3 specular = specular_strength * spec * incoming_light_color;

	vec3 final_result = (ambient + diffuse + specular) * object_color_base;
	final_color = vec4(final_result, 1.0);
	//final_color = vec4(normalize(normal) * 0.5 + 0.5, 1.0);
	//final_color = vec4(normalize(frag_pos) * 0.5 + 0.5, 1.0);
	//final_color = vec4(light_dir * 0.5 + 0.5, 1.0);
}