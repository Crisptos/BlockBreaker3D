#pragma once
#include <SDL3/SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <vector>
#include <array>
#include <string>
#include "Camera.h"

#define DEPTH_TEXTURE_IDX 0
#define SKYBOX_TEXTURE_IDX 1

namespace BB3D
{
	// OUTSIDE SOURCE FILES
// ________________________________ Shader.cpp ________________________________
	SDL_GPUShader* CreateShaderFromFile(
		SDL_GPUDevice* device,
		const char* file_path,
		SDL_GPUShaderStage shader_stage,
		Uint32 sampler_count,
		Uint32 uniform_buffer_count,
		Uint32 storage_buffer_count,
		Uint32 storage_texture_count
	);

	// ________________________________ Mesh.cpp ________________________________
	struct Mesh
	{
		SDL_GPUBuffer* vbo;
		SDL_GPUBuffer* ibo;
		int vert_count;
		int ind_count;
	};

	struct Vertex
	{
		float x, y, z, nx, ny, nz, u, v;
	};

	Mesh LoadMeshFromFile(SDL_GPUDevice* device, const char* filepath);
	Mesh CreateMesh(SDL_GPUDevice* device, std::vector<Vertex> vertices, std::vector<Uint16> indices);

	// ________________________________ Texture.cpp ________________________________
	struct Image
	{
		int x, y, channels;
	};

	SDL_GPUTexture* CreateAndLoadCubeMapToGPU(SDL_GPUDevice* device, std::array<std::string, 6> filepaths);
	SDL_GPUTexture* CreateDepthTestTexture(SDL_GPUDevice* device, int render_target_w, int render_target_h);
	SDL_GPUTexture* CreateAndLoadTextureToGPU(SDL_GPUDevice* device, const char* filepath);
	SDL_GPUSampler* CreateSampler(SDL_GPUDevice* device, SDL_GPUFilter texture_filter);

	// ________________________________ GraphicsPipeline.cpp ________________________________
	SDL_GPUGraphicsPipeline* CreateGraphicsPipelineForModels(SDL_GPUDevice* device, SDL_GPUTextureFormat color_target_format, SDL_GPUShader* vert_shader, SDL_GPUShader* frag_shader);
	SDL_GPUGraphicsPipeline* CreateGraphicsPipelineForSkybox(SDL_GPUDevice* device, SDL_GPUTextureFormat color_target_format, SDL_GPUShader* vert_shader, SDL_GPUShader* frag_shader);

	// ________________________________ Fonts.cpp ________________________________
	struct Glyph {
		unsigned int texture;
		glm::ivec2   size;
		glm::ivec2   bearing;
		unsigned int advance;
	};

	// ________________________________ Entity.cpp ________________________________
	struct Entity
	{
		Mesh mesh;
		SDL_GPUTexture* texture;

		glm::mat4 transform;

		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;

		glm::vec3 velocity;

		void UpdateTransform();
		void Draw(SDL_GPURenderPass* render_pass, SDL_GPUBufferBinding vbo_bind, SDL_GPUBufferBinding ibo_bind);
	};

	// ________________________________ Main Engine Class ________________________________

	class Engine
	{
	public:
		// Lifetime
		void Init();
		void Run();
		void Destroy();

	private:
		// Setup
		void Setup();

		// Runtime
		void Update();
		void Input();
		void Render();

		// Utility
		void UpdateDeltaTime();
		
	private:

		// State
		bool m_IsRunning = true;
		bool m_IsIdle = false;
		struct Timer
		{
			Uint64 last_frame;
			Uint64 current_frame;
			float elapsed_time;
		} m_Timer;

		Camera m_StaticCamera;
		std::vector<Entity> game_entities;

		// SDL Context
		SDL_Window* m_Window;
		SDL_GPUDevice* m_Device;

		//	Renderer State
		SDL_GPUGraphicsPipeline* m_PipelineSkybox;
		SDL_GPUGraphicsPipeline* m_PipelineModelsPhong;
		SDL_GPUGraphicsPipeline* m_PipelineModelsNoPhong;
		std::vector<Mesh> meshes;
		std::vector<SDL_GPUTexture*> textures;

		//	Global texture sampler
		SDL_GPUSampler* m_Sampler;
	};
}