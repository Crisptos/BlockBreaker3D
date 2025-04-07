#include "Engine.h"
#include <iostream>
#include <stb_image.h>
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

	struct Vertex
	{
		float x, y, z, r, g, b, u, v;
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

		SDL_ReleaseGPUBuffer(m_Device, vbo);
		SDL_ReleaseGPUBuffer(m_Device, ibo);
		SDL_ReleaseGPUTexture(m_Device, m_TestTex);
		SDL_ReleaseGPUSampler(m_Device, m_Sampler);

		SDL_ReleaseWindowFromGPUDevice(m_Device, m_Window);
		SDL_DestroyWindow(m_Window);
		SDL_DestroyGPUDevice(m_Device);
		SDL_Quit();
	}

	// ________________________________ Setup ________________________________

	void Engine::Setup()
	{
		SDL_GPUShader* vert_shader = CreateShaderFromFile(m_Device, "Shaders/triangle.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1, 0, 0);
		SDL_GPUShader* frag_shader = CreateShaderFromFile(m_Device, "Shaders/triangle.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0, 0, 0);

		// Load texture
		//  Load the pixels
		//  Create texture on the GPU
		//  Upload pixels to GPU texture
		//  Assign texture UV coordinates to vertices
		//  Create sampler
		//  Sample using UV in fragment shader
		stbi_set_flip_vertically_on_load(true);
		unsigned char* image_data = stbi_load("assets/metal_07.png", &m_Img.x, &m_Img.y, &m_Img.channels, 4);
		if (!image_data) std::abort();

		SDL_GPUTextureCreateInfo tex_info = {};
		tex_info.type = SDL_GPU_TEXTURETYPE_2D;
		tex_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
		tex_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
		tex_info.width = m_Img.x;
		tex_info.height = m_Img.y;
		tex_info.layer_count_or_depth = 1;
		tex_info.num_levels = 1;
		m_TestTex = SDL_CreateGPUTexture(m_Device, &tex_info);

		SDL_GPUTransferBufferCreateInfo tex_transfer_create_info = {};
		tex_transfer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		tex_transfer_create_info.size = 4 * (m_Img.x * m_Img.y);
		SDL_GPUTransferBuffer* tex_trans_buff = SDL_CreateGPUTransferBuffer(m_Device, &tex_transfer_create_info);

		void* tex_trans_ptr = SDL_MapGPUTransferBuffer(m_Device, tex_trans_buff, false);
		std::memcpy(tex_trans_ptr, image_data, 4 * (m_Img.x * m_Img.y));
		SDL_UnmapGPUTransferBuffer(m_Device, tex_trans_buff);

		SDL_GPUCommandBuffer* tex_copy_cmd_buff = SDL_AcquireGPUCommandBuffer(m_Device);
		SDL_GPUCopyPass* tex_copy_pass = SDL_BeginGPUCopyPass(tex_copy_cmd_buff);

		SDL_GPUTextureTransferInfo tex_trans_info = {};
		tex_trans_info.offset = 0;
		tex_trans_info.transfer_buffer = tex_trans_buff;
		SDL_GPUTextureRegion tex_trans_region = {};
		tex_trans_region.texture = m_TestTex;
		tex_trans_region.w = m_Img.x;
		tex_trans_region.h = m_Img.y;
		tex_trans_region.d = 1;
		SDL_UploadToGPUTexture(tex_copy_pass, &tex_trans_info, &tex_trans_region, false);
		SDL_EndGPUCopyPass(tex_copy_pass);
		if (!SDL_SubmitGPUCommandBuffer(tex_copy_cmd_buff))
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to submit copy command buffer to GPU Texture: %s\n", SDL_GetError());
			std::abort();
		}
		stbi_image_free(image_data);
		SDL_ReleaseGPUTransferBuffer(m_Device, tex_trans_buff);

		SDL_GPUSamplerCreateInfo sampler_info = {};
		sampler_info.min_filter = SDL_GPU_FILTER_NEAREST;
		sampler_info.mag_filter = SDL_GPU_FILTER_NEAREST;
		m_Sampler = SDL_CreateGPUSampler(m_Device, &sampler_info);

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
		vbo_info.size = sizeof(Vertex) * 4;
		vbo = SDL_CreateGPUBuffer(m_Device, &vbo_info);

		SDL_GPUBufferCreateInfo ibo_info = {};
		ibo_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
		ibo_info.size = sizeof(Uint16) * 6;
		ibo = SDL_CreateGPUBuffer(m_Device, &ibo_info);

		// upload vertex data to vbo
		//	- create transfer buffer
		//	- map transfer buffer mem and copy from cpu
		//	- begin copy pass
		//	- invoke upload command
		//	- end copy pass and submit
		SDL_GPUTransferBufferCreateInfo transfer_create_info = {};
		transfer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		transfer_create_info.size = (sizeof(Vertex) * 4) + (sizeof(Uint16) * 6);
		SDL_GPUTransferBuffer* trans_buff = SDL_CreateGPUTransferBuffer(m_Device, &transfer_create_info);
		
		void* trans_ptr = SDL_MapGPUTransferBuffer(m_Device, trans_buff, false);
		Vertex vertices[4] = {
			// XYZ RGB UV
			{1.0, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0},  // tr 0
			{1.0, -1.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0}, // br 1
			{-1.0, -1.0, 0.0, 1.0, 1.0, 1.0, 0.0, 1.0},// bl 2
			{-1.0, 1.0, 0.0, 1.0, 1.0, 1.0, 0.0, 0.0}  // tl 3
		};
		Uint16 indices[6] = {
			3, 0, 2,
			2, 0, 1
		};
		
		std::memcpy(trans_ptr, &vertices, sizeof(Vertex) * 4);
		std::memcpy(reinterpret_cast<Vertex*>(trans_ptr) + 4, &indices, sizeof(Uint16) * 6); // Map the index data right after the vertex data | 4 Vertices 96bytes | 6 Uint16s 12bytes | 108 bytes
		SDL_UnmapGPUTransferBuffer(m_Device, trans_buff);

		SDL_GPUCommandBuffer* copy_cmd_buff = SDL_AcquireGPUCommandBuffer(m_Device);
		SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buff);

		SDL_GPUTransferBufferLocation trans_location = {};
		trans_location.transfer_buffer = trans_buff;
		trans_location.offset = 0;
		SDL_GPUBufferRegion vbo_region = {};
		vbo_region.buffer = vbo;
		vbo_region.offset = 0;
		vbo_region.size = sizeof(Vertex) * 4;
		SDL_GPUBufferRegion ibo_region = {};
		ibo_region.buffer = ibo;
		ibo_region.offset = 0;
		ibo_region.size = sizeof(Uint16) * 6;

		SDL_UploadToGPUBuffer(copy_pass, &trans_location, &vbo_region, false);
		trans_location.offset = sizeof(Vertex) * 4;
		SDL_UploadToGPUBuffer(copy_pass, &trans_location, &ibo_region, false);

		SDL_EndGPUCopyPass(copy_pass);
		SDL_ReleaseGPUTransferBuffer(m_Device, trans_buff);
		if (!SDL_SubmitGPUCommandBuffer(copy_cmd_buff))
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to submit copy command buffer to GPU: %s\n", SDL_GetError());
			std::abort();
		}

		// Vertex Attribs
		// x, y, z, | r, g, b
		SDL_GPUVertexAttribute attribs[3] = {};
		attribs[0].location = 0;
		attribs[0].buffer_slot = 0;
		attribs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
		attribs[0].offset = 0;
		attribs[1].location = 1;
		attribs[1].buffer_slot = 0;
		attribs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
		attribs[1].offset = sizeof(float) * 3;
		attribs[2].location = 2;
		attribs[2].buffer_slot = 0;
		attribs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
		attribs[2].offset = sizeof(float) * 6;

		SDL_GPUVertexBufferDescription vbo_descr = {};
		vbo_descr.slot = 0;
		vbo_descr.pitch = sizeof(Vertex);
		vbo_descr.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
		vbo_descr.instance_step_rate = 0;

		SDL_GPUVertexInputState vert_input_state = {};
		vert_input_state.num_vertex_attributes = 3;
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
		SDL_GPUBufferBinding ind_bind = { ibo, 0 };
		SDL_BindGPUIndexBuffer(render_pass, &ind_bind, SDL_GPU_INDEXELEMENTSIZE_16BIT);
		SDL_GPUTextureSamplerBinding tex_bind = {m_TestTex, m_Sampler};
		SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_bind, 1);

		// Vertex Attributes - per vertex data
		// Uniform Data - pew draw call
		rot += 90.0f * m_Timer.elapsed_time;
		if (rot > 720.0f) rot = 0.0f;

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, -3.0f));
		model = glm::rotate(model, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
		mvp = proj * model;
		SDL_PushGPUVertexUniformData(cmd_buff, 0, glm::value_ptr(mvp), sizeof(mvp));

		SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);

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
		shader_create_info.num_samplers = sampler_count;
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