#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace BB3D
{
	struct Camera
	{
		glm::vec3 pos;
		glm::vec3 front;
		glm::vec3 up;

		float yaw = -90.0;
		float pitch;

	public:
		glm::mat4 GetViewMatrix();
	};
}