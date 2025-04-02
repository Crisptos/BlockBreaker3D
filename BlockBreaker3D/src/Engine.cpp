#include "Engine.h"
#include <iostream>

namespace BB3D
{
	// ________________________________ Engine Lifetime ________________________________

	void Engine::Init()
	{
		if (!SDL_Init(SDL_INIT_VIDEO))
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to initialize SDL3 and subsystems: %s\n", SDL_GetError());
			std::abort();
		}

		m_Window = SDL_CreateWindow(
			"Block Breaker 3D",
			1280,
			720,
			SDL_WINDOW_VULKAN
		);

		if (!m_Window)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to initialize window: %s\n", SDL_GetError());
			std::abort();
		}

		SDL_SetWindowPosition(m_Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		SDL_SetWindowResizable(m_Window, false);

		m_Device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);

		if (!m_Device)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to acquire a handle to the GPU: %s\n", SDL_GetError());
			std::abort();
		}

		SDL_Log("OK: Created GPU handle with driver: %s\n", SDL_GetGPUDeviceDriver(m_Device));

		if (!SDL_ClaimWindowForGPUDevice(m_Device, m_Window))
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to claim window for device: %s\n", SDL_GetError());
			std::abort();
		}
	}

	void Engine::Run()
	{
		while (m_IsRunning)
		{
			if (!m_IsIdle)
			{
				Render();
				Input();
			}
		}
	}

	void Engine::Destroy()
	{
		SDL_ReleaseWindowFromGPUDevice(m_Device, m_Window);
		SDL_DestroyGPUDevice(m_Device);
		SDL_DestroyWindow(m_Window);
		SDL_Quit();
	}

	// ________________________________ Setup ________________________________

	void Engine::Setup()
	{

	}

	// ________________________________ Runtime ________________________________
	void Engine::Render()
	{
		/*
			Acquire Command Buffer
			Acquire Swapchain Texture
			Begin Render Pass
			Draw
			End Render Pass
			More Render Passes
			Submit Command Buffer
		*/
		SDL_GPUCommandBuffer* cmd_buff = SDL_AcquireGPUCommandBuffer(m_Device);

		SDL_GPUTexture* swapchain_tex;
		if (!SDL_WaitAndAcquireGPUSwapchainTexture(
			cmd_buff, 
			m_Window, 
			&swapchain_tex, 
			nullptr, 
			nullptr
		))
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to acquire swapchain texture: %s\n", SDL_GetError());
			std::abort();
		}

		SDL_GPUColorTargetInfo color_target_info = {};
		color_target_info.texture = swapchain_tex;
		color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
		color_target_info.clear_color = {0.44, 0.44, 0.64, 1.0};
		color_target_info.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(
			cmd_buff,
			&color_target_info,
			1,
			nullptr
		);

		SDL_EndGPURenderPass(render_pass);


		if (!SDL_SubmitGPUCommandBuffer(cmd_buff))
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to submit command buffer to GPU: %s\n", SDL_GetError());
			std::abort();
		}
	}

	void Engine::Input()
	{
		SDL_Event ev;
		while (SDL_PollEvent(&ev))
		{
			switch (ev.type)
			{
				case SDL_EVENT_QUIT:
				{
					m_IsRunning = false;
				}
			}
		}
	}

	// Utility Functions
	SDL_GPUShader* CreateShaderFromFile(
		SDL_GPUDevice* device,
		const char* file_path,
		SDL_GPUShaderStage shader_stage,
		Uint32 sampler_count,
		Uint32 uniform_buffer_count,
		Uint32 storage_buffer_count,
		Uint32 storage_texture_count
	)
	{
		SDL_GPUShader* new_shader;

		SDL_GPUShaderFormat supported_formats = SDL_GetGPUShaderFormats(device);
		if (!(supported_formats & SDL_GPU_SHADERFORMAT_SPIRV))
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Device context does not support the target shader format (SPIR-V)");
			std::abort();
		}

		size_t source_size = 0;
		void* shader_source = SDL_LoadFile(file_path, &source_size);

		SDL_GPUShaderCreateInfo shader_create_info = {};
		shader_create_info.code = static_cast<Uint8*>(shader_source);

		return new_shader;
	}
}