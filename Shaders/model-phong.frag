#version 450

// GLSL pads vec3 to 16 bytes per std140, caused a fun lighting bug
layout(set=3, binding = 0)uniform UBO {
	vec3 object_color_base; float pad0;					     // 16 bytes
	vec3 incoming_light_color; float pad1;                   // 16 bytes
	vec3 view_pos; float pad2;							     // 16 bytes
	float num_of_lights; float pad3; float pad4; float pad5; // 16 bytes
	vec4 light_positions[32];							     // 512 bytes
};

layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 frag_uv;
layout(location = 2) in vec3 frag_pos;

layout(location = 0) out vec4 final_color;

layout(set=2, binding=0) uniform sampler2D tex_sampler;

vec3 calc_point_light(vec3 light_pos, vec3 normal, vec3 frag_pos, vec3 view_dir)
{
	const float AMBIENT_STRENGTH = 0.2;
	const float SPECULAR_STRENGTH = 0.5;
	const float CONSTANT = 1.0;
	const float LINEAR = 0.07;
	const float QUADRATIC = 0.017;

	vec3 light_dir = normalize(light_pos - frag_pos);

	// diffuse -> specular -> attenuate
	float diff = max(dot(normal, light_dir), 0.0);

	vec3 reflect_dir = reflect(-light_dir, normal);
	float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);

	float dist = length(light_pos - frag_pos);
	float attenuation = 1.0 / (CONSTANT + LINEAR * dist + QUADRATIC * (dist * dist));

	// combine
	vec3 ambient = AMBIENT_STRENGTH * vec3(texture(tex_sampler, frag_uv));
	vec3 diffuse = diff * vec3(texture(tex_sampler, frag_uv)) * incoming_light_color;
	vec3 specular = SPECULAR_STRENGTH * spec * incoming_light_color;

	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;

	return (ambient + diffuse + specular);
}

void main()
{
	vec3 view_dir = normalize(view_pos - frag_pos);
	int num_of_lights_for = int(num_of_lights);

	vec3 result = vec3(0.0f);
	for(int i = 0; i < num_of_lights_for; i+=1)
	{
		vec3 light_pos = vec3(light_positions[i].x, light_positions[i].y, light_positions[i].z);
		result += calc_point_light(light_pos, normalize(normal), frag_pos, view_dir);
	}

	final_color = vec4(result, 1.0);
	//final_color = vec4(normalize(normal) * 0.5 + 0.5, 1.0);
	//final_color = vec4(normalize(frag_pos) * 0.5 + 0.5, 1.0);
	//final_color = vec4(light_dir * 0.5 + 0.5, 1.0);
}