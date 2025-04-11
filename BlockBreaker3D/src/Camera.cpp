#include "Camera.h"

namespace BB3D
{
	glm::mat4 Camera::GetViewMatrix()
	{
		return glm::lookAt(pos, pos + front, up);
	}
}