#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace BB3D
{
	struct Camera
	{
		glm::vec3 pos = glm::vec3(0.0f);
		glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

		float yaw = -90.0;
		float pitch;

	public:
		glm::mat4 GetViewMatrix();
	};
}