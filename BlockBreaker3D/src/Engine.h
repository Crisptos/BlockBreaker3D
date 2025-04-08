#pragma once
#include <SDL3/SDL.h>
#include <vector>

namespace BB3D
{
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

		// SDL Context
		SDL_Window* m_Window;
		SDL_GPUDevice* m_Device;

		SDL_GPUGraphicsPipeline* m_Pipeline;

		SDL_GPUTexture* m_TestTex;
		SDL_GPUSampler* m_Sampler;
	};

	// ________________________________ Shader.cpp ________________________________
	//     Utility Functions
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
	//     Utility Functions
	struct Mesh
	{
		SDL_GPUBuffer* vbo;
		SDL_GPUBuffer* ibo;
		int vert_count;
		int ind_count;
	};

	struct Vertex
	{
		float x, y, z, r, g, b, u, v;
	};

	Mesh CreateMesh(SDL_GPUDevice* device, std::vector<Vertex> vertices, std::vector<Uint16> indices);

	// ________________________________ Texture.cpp ________________________________
	struct Image
	{
		int x, y, channels;
	};

	SDL_GPUTexture* CreateAndLoadTextureToGPU(SDL_GPUDevice* device, const char* filepath);
	SDL_GPUSampler* CreateSampler(SDL_GPUDevice* device, SDL_GPUFilter texture_filter);
}