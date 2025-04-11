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
	Mesh ico;

	float last_x = 0.0f;
	float last_y = 0.0f;

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
		SDL_HideCursor();
		SDL_SetWindowRelativeMouseMode(m_Window, true);

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
		SDL_ReleaseGPUGraphicsPipeline(m_Device, m_PipelineSkybox);

		SDL_ReleaseGPUBuffer(m_Device, ico.vbo);
		SDL_ReleaseGPUBuffer(m_Device, ico.ibo);
		SDL_ReleaseGPUTexture(m_Device, m_IcoTex);
		SDL_ReleaseGPUTexture(m_Device, m_DepthTex);
		SDL_ReleaseGPUTexture(m_Device, m_Cubemap);
		SDL_ReleaseGPUSampler(m_Device, m_Sampler);

		SDL_ReleaseWindowFromGPUDevice(m_Device, m_Window);
		SDL_DestroyWindow(m_Window);
		SDL_DestroyGPUDevice(m_Device);
		SDL_Quit();
	}

	// ________________________________ Setup ________________________________

	void Engine::Setup()
	{

		// Setup for model pipeline

		SDL_GPUShader* vert_shader_model = CreateShaderFromFile(m_Device, "Shaders/quad.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1, 0, 0);
		SDL_GPUShader* frag_shader_model = CreateShaderFromFile(m_Device, "Shaders/quad.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0, 0, 0);

		m_IcoTex = CreateAndLoadTextureToGPU(m_Device, "assets/gem_10.png");
		m_Sampler = CreateSampler(m_Device, SDL_GPU_FILTER_NEAREST);
		m_DepthTex = CreateDepthTestTexture(m_Device, 1280, 720);
		ico = LoadMeshFromFile(m_Device, "assets/ico.obj");

		m_Pipeline = CreateGraphicsPipelineForModels(
			m_Device, 
			SDL_GetGPUSwapchainTextureFormat(m_Device, m_Window),
			vert_shader_model,
			frag_shader_model
		);

		SDL_ReleaseGPUShader(m_Device, vert_shader_model);
		SDL_ReleaseGPUShader(m_Device, frag_shader_model);

		// Setup for skybox pipeline

		SDL_GPUShader* vert_shader_skybox = CreateShaderFromFile(m_Device, "Shaders/skybox.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1, 0, 0);
		SDL_GPUShader* frag_shader_skybox = CreateShaderFromFile(m_Device, "Shaders/skybox.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0, 0, 0);

		m_Cubemap = CreateAndLoadCubeMapToGPU(
			m_Device, 
			{
				"assets/skyboxes/ocean/ocean_right.png",
				"assets/skyboxes/ocean/ocean_left.png",
				"assets/skyboxes/ocean/ocean_up.png",
				"assets/skyboxes/ocean/ocean_down.png",
				"assets/skyboxes/ocean/ocean_front.png",
				"assets/skyboxes/ocean/ocean_back.png"
			}
		);

		m_PipelineSkybox = CreateGraphicsPipelineForSkybox(
			m_Device, 
			SDL_GetGPUSwapchainTextureFormat(m_Device, m_Window),
			vert_shader_skybox,
			frag_shader_skybox
		);

		SDL_ReleaseGPUShader(m_Device, vert_shader_skybox);
		SDL_ReleaseGPUShader(m_Device, frag_shader_skybox);
		
		// Uniform data
		proj = glm::perspective(glm::radians(60.0f), 1280.0f/720.0f, 0.0001f, 1000.0f);

		m_StaticCamera.pos = glm::vec3(0.0f, 0.0f, 3.0f);
		m_StaticCamera.front = glm::vec3(0.0f, 0.0f, -1.0f);
		m_StaticCamera.up = glm::vec3(0.0f, 1.0f, 0.0f);
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

		// Per Renderpass
		//	Bind Pipeline
		//	Bind Vertex Data
		//	Bind Uniform
		//	Draw Call

		SDL_GPUColorTargetInfo color_target_info = {};
		color_target_info.texture = swapchain_tex;
		color_target_info.load_op = SDL_GPU_LOADOP_LOAD;
		color_target_info.clear_color = {1.0, 0.0, 1.0, 1.0};
		color_target_info.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPUDepthStencilTargetInfo depth_stencil_target_info = {};
		depth_stencil_target_info.texture = m_DepthTex;
		depth_stencil_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
		depth_stencil_target_info.clear_depth = 1;
		depth_stencil_target_info.store_op = SDL_GPU_STOREOP_DONT_CARE;

		SDL_GPURenderPass* render_pass_skybox = SDL_BeginGPURenderPass(
			cmd_buff,
			&color_target_info,
			1,
			nullptr
		);

		SDL_BindGPUGraphicsPipeline(render_pass_skybox, m_PipelineSkybox);
		SDL_GPUTextureSamplerBinding skybox_bind = { m_Cubemap, m_Sampler };
		SDL_BindGPUFragmentSamplers(render_pass_skybox, 0, &skybox_bind, 1);

		glm::mat4 vp_sky(1.0f);
		glm::mat4 view_no_transform = glm::mat4(glm::mat3(m_StaticCamera.GetViewMatrix()));
		vp_sky = proj * view_no_transform;
		SDL_PushGPUVertexUniformData(cmd_buff, 0, glm::value_ptr(vp_sky), sizeof(vp_sky));

		SDL_DrawGPUPrimitives(render_pass_skybox, 36, 1, 0, 0);

		SDL_EndGPURenderPass(render_pass_skybox);

		SDL_GPURenderPass* render_pass_models = SDL_BeginGPURenderPass(
			cmd_buff,
			&color_target_info,
			1,
			&depth_stencil_target_info
		);

		SDL_BindGPUGraphicsPipeline(render_pass_models, m_Pipeline);
		SDL_GPUBufferBinding binding = {ico.vbo, 0};
		SDL_BindGPUVertexBuffers(render_pass_models, 0, &binding, 1);
		SDL_GPUBufferBinding ind_bind = { ico.ibo, 0 };
		SDL_BindGPUIndexBuffer(render_pass_models, &ind_bind, SDL_GPU_INDEXELEMENTSIZE_16BIT);
		SDL_GPUTextureSamplerBinding tex_bind = {m_IcoTex, m_Sampler};
		SDL_BindGPUFragmentSamplers(render_pass_models, 0, &tex_bind, 1);

		// Vertex Attributes - per vertex data
		// Uniform Data - pew draw call
		rot += 90.0f * m_Timer.elapsed_time;
		if (rot > 720.0f) rot = 0.0f;

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, -0.5f, -4.0f));
		model = glm::rotate(model, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
		mvp = proj * m_StaticCamera.GetViewMatrix() * model;

		SDL_PushGPUVertexUniformData(cmd_buff, 0, glm::value_ptr(mvp), sizeof(mvp));

		SDL_DrawGPUIndexedPrimitives(render_pass_models, ico.ind_count, 1, 0, 0, 0);

		SDL_EndGPURenderPass(render_pass_models);


		if (!SDL_SubmitGPUCommandBuffer(cmd_buff))
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to submit command buffer to GPU: %s\n", SDL_GetError());
			std::abort();
		}
	}

	void Engine::Input()
	{
		// TODO Clean up Camera Code

		float mouse_sensitivity = 0.3f;
		float relx = 0.0f;
		float rely = 0.0f;

		SDL_Event ev;
		while (SDL_PollEvent(&ev))
		{
			switch (ev.type)
			{
				case SDL_EVENT_QUIT:
				{
					m_IsRunning = false;
				}

				case SDL_EVENT_MOUSE_MOTION:
				{
					relx = ev.motion.xrel;
					rely = ev.motion.yrel;
				}
			}
		}

		relx *= mouse_sensitivity;
		rely *= mouse_sensitivity;

		m_StaticCamera.yaw += relx;
		m_StaticCamera.pitch -= rely;

		if (m_StaticCamera.pitch > 89.0f)
			m_StaticCamera.pitch = 89.0f;
		if (m_StaticCamera.pitch < -89.0f)
			m_StaticCamera.pitch = -89.0f;

		glm::vec3 direction;
		direction.x = cos(glm::radians(m_StaticCamera.yaw)) * cos(glm::radians(m_StaticCamera.pitch));
		direction.y = sin(glm::radians(m_StaticCamera.pitch));
		direction.z = sin(glm::radians(m_StaticCamera.yaw)) * cos(glm::radians(m_StaticCamera.pitch));
		m_StaticCamera.front = glm::normalize(direction);

		const bool* keys = SDL_GetKeyboardState(0);

		const float camera_speed = 2.0f * m_Timer.elapsed_time;

		if (keys[SDL_SCANCODE_W])
			m_StaticCamera.pos += camera_speed * m_StaticCamera.front;
		if(keys[SDL_SCANCODE_A])
			m_StaticCamera.pos -= glm::normalize(glm::cross(m_StaticCamera.front, m_StaticCamera.up)) * camera_speed;
		if(keys[SDL_SCANCODE_S])
			m_StaticCamera.pos -= camera_speed * m_StaticCamera.front;
		if(keys[SDL_SCANCODE_D])
			m_StaticCamera.pos += glm::normalize(glm::cross(m_StaticCamera.front, m_StaticCamera.up)) * camera_speed;
		if (keys[SDL_SCANCODE_ESCAPE])
			m_IsRunning = false;
	}

	void Engine::UpdateDeltaTime()
	{
		const float FRAME_TARGET_TIME = 1000.0f / 60.0f;

		m_Timer.last_frame = m_Timer.current_frame;
		m_Timer.current_frame = SDL_GetTicks();
		m_Timer.elapsed_time = (m_Timer.current_frame - m_Timer.last_frame) / 1000.0f;

		if (m_Timer.elapsed_time < FRAME_TARGET_TIME) SDL_Delay(FRAME_TARGET_TIME - m_Timer.elapsed_time);
	}
}