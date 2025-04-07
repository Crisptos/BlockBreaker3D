#pragma once
#include <SDL3/SDL.h>

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

		struct Image
		{
			int x, y, channels;
		} m_Img;

		// SDL Context
		SDL_Window* m_Window;
		SDL_GPUDevice* m_Device;

		SDL_GPUGraphicsPipeline* m_Pipeline;
		SDL_GPUBuffer* vbo;
		SDL_GPUBuffer* ibo;

		SDL_GPUTexture* m_TestTex;
		SDL_GPUSampler* m_Sampler;
	};

	// Utility Functions
	SDL_GPUShader* CreateShaderFromFile(
		SDL_GPUDevice* device, 
		const char* file_path, 
		SDL_GPUShaderStage shader_stage, 
		Uint32 sampler_count,
		Uint32 uniform_buffer_count,
		Uint32 storage_buffer_count,
		Uint32 storage_texture_count
	);
}