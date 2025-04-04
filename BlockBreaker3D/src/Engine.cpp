#include "Engine.h"
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace BB3D
{
	// ________________________________ Globals (TEST) ________________________________
	glm::mat4 proj(1.0f);
	glm::mat4 model(1.0f);
	glm::mat4 mvp(1.0f);
	float rot = 0.0f;
	float height = 0.0f;
	bool dir = true;

	struct Vertex
	{
		float x, y, z, r, g, b;
	};
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
				UpdateDeltaTime();
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
		SDL_GPUShader* vert_shader = CreateShaderFromFile(m_Device, "Shaders/triangle.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1, 0, 0);
		SDL_GPUShader* frag_shader = CreateShaderFromFile(m_Device, "Shaders/triangle.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0, 0, 0);

		if (!vert_shader || !frag_shader)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create GPU Pipeline shaders: %s\n", SDL_GetError());
			std::abort();
		}

		// Describe vertex attributes and buffers in pipeline
		// create vertex data, create buffer, upload data to the buffer
		// bind buffer to draw call

		SDL_GPUColorTargetDescription color_target_dscr = {};
		color_target_dscr.format = SDL_GetGPUSwapchainTextureFormat(m_Device, m_Window);

		SDL_GPUGraphicsPipelineTargetInfo target_info_pipeline = {};
		target_info_pipeline.num_color_targets = 1;
		target_info_pipeline.color_target_descriptions = &color_target_dscr;

		SDL_GPUBufferCreateInfo vbo_info = {};
		vbo_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
		vbo_info.size = sizeof(Vertex) * 3;
		vbo = SDL_CreateGPUBuffer(m_Device, &vbo_info);

		// upload vertex data to vbo
		//	- create transfer buffer
		//	- map transfer buffer mem and copy from cpu
		//	- begin copy pass
		//	- invoke upload command
		//	- end copy pass and submit
		SDL_GPUTransferBufferCreateInfo transfer_create_info = {};
		transfer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		transfer_create_info.size = sizeof(Vertex) * 3;
		SDL_GPUTransferBuffer* trans_buff = SDL_CreateGPUTransferBuffer(m_Device, &transfer_create_info);
		
		void* trans_ptr = SDL_MapGPUTransferBuffer(m_Device, trans_buff, false);
		Vertex vertices[3] = {
			{0.0, 0.5, 0.0, 0.95, 0.0, 0.0},
			{0.5, -0.5, 0.0, 0.0, 0.95, 0.0},
			{-0.5, -0.5, 0.0, 0.0, 0.0, 0.95}
		};
		std::memcpy(trans_ptr, &vertices, sizeof(Vertex) * 3);
		SDL_UnmapGPUTransferBuffer(m_Device, trans_buff);

		SDL_GPUCommandBuffer* copy_cmd_buff = SDL_AcquireGPUCommandBuffer(m_Device);
		SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buff);

		SDL_GPUTransferBufferLocation trans_location = {};
		trans_location.transfer_buffer = trans_buff;
		trans_location.offset = 0;
		SDL_GPUBufferRegion vbo_region = {};
		vbo_region.buffer = vbo;
		vbo_region.offset = 0;
		vbo_region.size = sizeof(Vertex) * 3;

		SDL_UploadToGPUBuffer(copy_pass, &trans_location, &vbo_region, false);

		SDL_EndGPUCopyPass(copy_pass);
		if (!SDL_SubmitGPUCommandBuffer(copy_cmd_buff))
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to submit copy command buffer to GPU: %s\n", SDL_GetError());
			std::abort();
		}

		// Vertex Attribs
		// x, y, z, | r, g, b
		SDL_GPUVertexAttribute attribs[2] = {};
		attribs[0].location = 0;
		attribs[0].buffer_slot = 0;
		attribs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
		attribs[0].offset = 0;
		attribs[1].location = 1;
		attribs[1].buffer_slot = 0;
		attribs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
		attribs[1].offset = sizeof(float) * 3;

		SDL_GPUVertexBufferDescription vbo_descr = {};
		vbo_descr.slot = 0;
		vbo_descr.pitch = sizeof(Vertex);
		vbo_descr.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
		vbo_descr.instance_step_rate = 0;

		SDL_GPUVertexInputState vert_input_state = {};
		vert_input_state.num_vertex_attributes = 2;
		vert_input_state.num_vertex_buffers = 1;
		vert_input_state.vertex_buffer_descriptions = &vbo_descr;
		vert_input_state.vertex_attributes = attribs;

		SDL_GPUGraphicsPipelineCreateInfo create_info_pipeline = {};
		create_info_pipeline.vertex_shader = vert_shader;
		create_info_pipeline.fragment_shader = frag_shader;
		create_info_pipeline.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
		create_info_pipeline.target_info = target_info_pipeline;
		create_info_pipeline.vertex_input_state = vert_input_state;

		m_Pipeline = SDL_CreateGPUGraphicsPipeline(m_Device, &create_info_pipeline);

		SDL_ReleaseGPUShader(m_Device, vert_shader);
		SDL_ReleaseGPUShader(m_Device, frag_shader);

		// Delete Me After
		proj = glm::perspective(glm::radians(60.0f), 1280.0f/720.0f, 0.0001f, 1000.0f);
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
		color_target_info.clear_color = {0.87, 0.85, 0.88, 1.0};
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
		SDL_GPUBufferBinding binding = {vbo, 0};
		SDL_BindGPUVertexBuffers(render_pass, 0, &binding, 1);

		// Vertex Attributes - per vertex data
		// Uniform Data - pew draw call
		rot += 360.0f * m_Timer.elapsed_time;
		if (rot > 720.0f) rot = 0.0f;

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, height, -2.0f));
		model = glm::rotate(model, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
		mvp = proj * model;
		SDL_PushGPUVertexUniformData(cmd_buff, 0, glm::value_ptr(mvp), sizeof(mvp));

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

	void Engine::UpdateDeltaTime()
	{
		const float FRAME_TARGET_TIME = 1000.0f / 60.0f;

		m_Timer.last_frame = m_Timer.current_frame;
		m_Timer.current_frame = SDL_GetTicks();
		m_Timer.elapsed_time = (m_Timer.current_frame - m_Timer.last_frame) / 1000.0f;

		if (m_Timer.elapsed_time < FRAME_TARGET_TIME) SDL_Delay(FRAME_TARGET_TIME - m_Timer.elapsed_time);
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