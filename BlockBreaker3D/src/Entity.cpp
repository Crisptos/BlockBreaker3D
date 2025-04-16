#include "Engine.h"

namespace BB3D
{
	void Entity::UpdateTransform()
	{
		transform = glm::mat4(1.0f);
		// Scale -> Rotate -> Transform
		transform = glm::translate(transform, position);

		// Rotation for each axis
		transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		transform = glm::scale(transform, scale);
	}
}