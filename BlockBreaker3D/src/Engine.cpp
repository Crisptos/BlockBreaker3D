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
		Setup();
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
		SDL_ReleaseGPUGraphicsPipeline(m_Device, m_Pipeline);
		SDL_ReleaseWindowFromGPUDevice(m_Device, m_Window);
		SDL_DestroyWindow(m_Window);
		SDL_DestroyGPUDevice(m_Device);
		SDL_Quit();
	}

	// ________________________________ Setup ________________________________

	void Engine::Setup()
	{
		SDL_GPUShader* vert_shader = CreateShaderFromFile(m_Device, "Shaders/triangle.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0, 0, 0);
		SDL_GPUShader* frag_shader = CreateShaderFromFile(m_Device, "Shaders/triangle.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0, 0, 0);

		if (!vert_shader || !frag_shader)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create GPU Pipeline shaders: %s\n", SDL_GetError());
			std::abort();
		}

		SDL_GPUColorTargetDescription color_target_dscr = {};
		color_target_dscr.format = SDL_GetGPUSwapchainTextureFormat(m_Device, m_Window);

		SDL_GPUGraphicsPipelineTargetInfo target_info_pipeline = {};
		target_info_pipeline.num_color_targets = 1;
		target_info_pipeline.color_target_descriptions = &color_target_dscr;

		SDL_GPUGraphicsPipelineCreateInfo create_info_pipeline = {};
		create_info_pipeline.vertex_shader = vert_shader;
		create_info_pipeline.fragment_shader = frag_shader;
		create_info_pipeline.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
		create_info_pipeline.target_info = target_info_pipeline;

		m_Pipeline = SDL_CreateGPUGraphicsPipeline(m_Device, &create_info_pipeline);

		SDL_ReleaseGPUShader(m_Device, vert_shader);
		SDL_ReleaseGPUShader(m_Device, frag_shader);
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

		// Bind Pipeline
		// Bind Vertex Data
		// Bind Uniform
		// Draw Call

		SDL_BindGPUGraphicsPipeline(render_pass, m_Pipeline);
		SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);

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
		if (!shader_source)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Couldn't locate file at: %s\n", file_path);
			std::abort();
		}

		SDL_GPUShaderCreateInfo shader_create_info = {};
		shader_create_info.code = static_cast<Uint8*>(shader_source);
		shader_create_info.code_size = source_size;
		shader_create_info.entrypoint = "main";
		shader_create_info.format = supported_formats;
		shader_create_info.stage = shader_stage;
		shader_create_info.num_storage_textures = storage_texture_count;
		shader_create_info.num_storage_buffers = storage_buffer_count;
		shader_create_info.num_uniform_buffers = uniform_buffer_count;

		new_shader = SDL_CreateGPUShader(
			device,
			&shader_create_info
		);

		SDL_free(shader_source);

		return new_shader;
	}
}