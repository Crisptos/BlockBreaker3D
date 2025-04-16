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

	void Entity::Draw(SDL_GPURenderPass* render_pass, SDL_GPUBufferBinding vbo_bind, SDL_GPUBufferBinding ibo_bind)
	{
		SDL_BindGPUVertexBuffers(render_pass, 0, &vbo_bind, 1);
		SDL_BindGPUIndexBuffer(render_pass, &ibo_bind, SDL_GPU_INDEXELEMENTSIZE_16BIT);

		SDL_DrawGPUIndexedPrimitives(render_pass, mesh.ind_count, 1, 0, 0, 0);
	}
}