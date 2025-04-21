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
	glm::mat4 proj_ui(1.0f);
	glm::mat4 model(1.0f);
	glm::mat4 model2(1.0f);
	glm::mat4 mvp(1.0f);

	FontAtlas test_font;

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

		InitFreeType();

		m_Timer.current_frame = SDL_GetTicks();
		m_Timer.last_frame = m_Timer.current_frame;
		m_Timer.elapsed_time = 0.0f;
	}

	void Engine::Run()
	{
		Setup();
		while (m_IsRunning)
		{
			if (!m_IsIdle)
			{
				Update();
				Render();
				Input();
				UpdateDeltaTime();
			}
		}
	}

	void Engine::Destroy()
	{
		// Freetype and Fonts
		DestroyFreeType();

		// Dispose of all textures and meshes
		SDL_ReleaseGPUGraphicsPipeline(m_Device, m_PipelineModelsNoPhong);
		SDL_ReleaseGPUGraphicsPipeline(m_Device, m_PipelineModelsPhong);
		SDL_ReleaseGPUGraphicsPipeline(m_Device, m_PipelineSkybox);
		SDL_ReleaseGPUGraphicsPipeline(m_Device, m_PipelineUI);

		for (Mesh& disposed_mesh : meshes)
		{
			SDL_ReleaseGPUBuffer(m_Device, disposed_mesh.vbo);
			SDL_ReleaseGPUBuffer(m_Device, disposed_mesh.ibo);
		}

		SDL_ReleaseGPUBuffer(m_Device, ui_buff);

		for (SDL_GPUTexture* disposed_texture : textures)
		{
			SDL_ReleaseGPUTexture(m_Device, disposed_texture);
		}
		SDL_ReleaseGPUTexture(m_Device, test_font.atlas_texture);
		SDL_ReleaseGPUSampler(m_Device, m_Sampler);

		SDL_ReleaseWindowFromGPUDevice(m_Device, m_Window);
		SDL_DestroyWindow(m_Window);
		SDL_DestroyGPUDevice(m_Device);
		SDL_Quit();
	}

	// ________________________________ Setup ________________________________

	void Engine::Setup()
	{

		// Allocate storage
		game_entities.reserve(16);
		meshes.reserve(16);
		textures.reserve(16);

		// Load Textures
		// DEPTH TEXTURE IS ALWAYS IDX 0, SKYBOX TEXTURE IS ALWAYS IDX 1
		textures.push_back(CreateDepthTestTexture(m_Device, 1280, 720));
		textures.push_back(CreateAndLoadCubeMapToGPU(
			m_Device,
			{
				"assets/skyboxes/space/space_right.png",
				"assets/skyboxes/space/space_left.png",
				"assets/skyboxes/space/space_up.png",
				"assets/skyboxes/space/space_down.png",
				"assets/skyboxes/space/space_front.png",
				"assets/skyboxes/space/space_back.png"
			}
		));
		textures.push_back(CreateAndLoadTextureToGPU(m_Device, "assets/gem_10.png"));
		textures.push_back(CreateAndLoadTextureToGPU(m_Device, "assets/gem_03.png"));
		textures.push_back(CreateAndLoadTextureToGPU(m_Device, "assets/metal_07.png"));

		test_font = CreateFontAtlasFromFile(m_Device, "assets/fonts/DejaVuSansMono.ttf");

		// Load Meshes
		meshes.push_back(LoadMeshFromFile(m_Device, "assets/ico.obj"));
		meshes.push_back(LoadMeshFromFile(m_Device, "assets/quad.obj"));
		meshes.push_back(LoadMeshFromFile(m_Device, "assets/sphere.obj"));

		// Load Shaders and Setup Pipelines
		SDL_GPUShader* phong_vert_shader_model = CreateShaderFromFile(m_Device, "Shaders/model-phong.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1, 0, 0);
		SDL_GPUShader* phong_frag_shader_model = CreateShaderFromFile(m_Device, "Shaders/model-phong.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1, 0, 0);
		SDL_GPUShader* no_phong_vert_shader_model = CreateShaderFromFile(m_Device, "Shaders/model-no-phong.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1, 0, 0);
		SDL_GPUShader* no_phong_frag_shader_model = CreateShaderFromFile(m_Device, "Shaders/model-no-phong.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0, 0, 0);
		SDL_GPUShader* skybox_vert_shader = CreateShaderFromFile(m_Device, "Shaders/skybox.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1, 0, 0);
		SDL_GPUShader* skybox_frag_shader = CreateShaderFromFile(m_Device, "Shaders/skybox.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0, 0, 0);
		SDL_GPUShader* ui_vert_shader = CreateShaderFromFile(m_Device, "Shaders/ui.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1, 0, 0);
		SDL_GPUShader* ui_frag_shader = CreateShaderFromFile(m_Device, "Shaders/ui.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0, 0, 0);

		m_PipelineModelsPhong = CreateGraphicsPipelineForModels(
			m_Device, 
			SDL_GetGPUSwapchainTextureFormat(m_Device, m_Window),
			phong_vert_shader_model,
			phong_frag_shader_model
		);

		m_PipelineModelsNoPhong = CreateGraphicsPipelineForModels(
			m_Device,
			SDL_GetGPUSwapchainTextureFormat(m_Device, m_Window),
			no_phong_vert_shader_model,
			no_phong_frag_shader_model
		);

		m_PipelineSkybox = CreateGraphicsPipelineForSkybox(
			m_Device,
			SDL_GetGPUSwapchainTextureFormat(m_Device, m_Window),
			skybox_vert_shader,
			skybox_frag_shader
		);

		m_PipelineUI = CreateGraphicsPipelineForUI(
			m_Device,
			SDL_GetGPUSwapchainTextureFormat(m_Device, m_Window),
			ui_vert_shader,
			ui_frag_shader
		);

		SDL_ReleaseGPUShader(m_Device, phong_vert_shader_model);
		SDL_ReleaseGPUShader(m_Device, phong_frag_shader_model);
		SDL_ReleaseGPUShader(m_Device, no_phong_frag_shader_model);
		SDL_ReleaseGPUShader(m_Device, no_phong_vert_shader_model);
		SDL_ReleaseGPUShader(m_Device, skybox_vert_shader);
		SDL_ReleaseGPUShader(m_Device, skybox_frag_shader);
		SDL_ReleaseGPUShader(m_Device, ui_vert_shader);
		SDL_ReleaseGPUShader(m_Device, ui_frag_shader);

		m_Sampler = CreateSampler(m_Device, SDL_GPU_FILTER_NEAREST);

		ui_buff = CreateUILayerBuffer(m_Device);

		// Entities TODO
		game_entities.push_back( {meshes[2], textures[2], glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, -4.0f), glm::vec3(0.0f), glm::vec3(1.0f), glm::vec3(0.0f) });
		game_entities.push_back( {meshes[1], textures[2], glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f), glm::vec3(0.0f), glm::vec3(5.0f, 1.0f, 5.0f), glm::vec3(0.0f) });
		
		// Uniform data
		proj = glm::perspective(glm::radians(60.0f), 1280.0f/720.0f, 0.0001f, 1000.0f);
		proj_ui = glm::ortho(0.0f, 1280.0f, 720.0f, 0.0f, -1.0f, 1.0f);

		m_StaticCamera.pos = glm::vec3(0.0f, 0.0f, 4.0f);
		m_StaticCamera.front = glm::vec3(0.0f, 0.0f, -1.0f);
		m_StaticCamera.up = glm::vec3(0.0f, 1.0f, 0.0f);
		m_StaticCamera.pitch = 0.0f;
		m_StaticCamera.yaw = -80.0f;
	}

	void Engine::Update()
	{
		for (Entity& current_entity : game_entities)
		{
			current_entity.UpdateTransform();
		}

	}

	// ________________________________ Runtime ________________________________
	void Engine::Render()
	{
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
		color_target_info.load_op = SDL_GPU_LOADOP_LOAD;
		color_target_info.clear_color = {1.0, 0.0, 1.0, 1.0};
		color_target_info.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPUDepthStencilTargetInfo depth_stencil_target_info = {};
		depth_stencil_target_info.texture = textures[DEPTH_TEXTURE_IDX];
		depth_stencil_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
		depth_stencil_target_info.clear_depth = 1;
		depth_stencil_target_info.store_op = SDL_GPU_STOREOP_DONT_CARE;

		SDL_GPURenderPass* render_pass_skybox = SDL_BeginGPURenderPass(
			cmd_buff,
			&color_target_info,
			1,
			nullptr
		);

		// Stage 1: Skybox
		SDL_BindGPUGraphicsPipeline(render_pass_skybox, m_PipelineSkybox);
		SDL_GPUTextureSamplerBinding skybox_bind = { textures[SKYBOX_TEXTURE_IDX], m_Sampler};
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
		SDL_GPUBufferBinding binding = { meshes[2].vbo, 0};
		SDL_BindGPUVertexBuffers(render_pass_models, 0, &binding, 1);
		SDL_GPUBufferBinding ind_bind = { meshes[2].ibo, 0 };
		SDL_BindGPUIndexBuffer(render_pass_models, &ind_bind, SDL_GPU_INDEXELEMENTSIZE_16BIT);
		SDL_GPUTextureSamplerBinding tex_bind = {textures[3], m_Sampler};
		SDL_BindGPUFragmentSamplers(render_pass_models, 0, &tex_bind, 1);

		// Stage 2: 3D Models
		SDL_BindGPUGraphicsPipeline(render_pass_models, m_PipelineModelsNoPhong);
		model2 = glm::mat4(1.0f);
		model2 = glm::translate(model2, glm::vec3(-2.0f, 1.0f, -2.0f));
		model2 = glm::scale(model2, glm::vec3(0.5, 0.5, 0.5));
		mvp = proj * m_StaticCamera.GetViewMatrix() * model2;
		SDL_PushGPUVertexUniformData(cmd_buff, 0, glm::value_ptr(mvp), sizeof(mvp));
		SDL_DrawGPUIndexedPrimitives(render_pass_models, meshes[2].ind_count, 1, 0, 0, 0);

		SDL_BindGPUGraphicsPipeline(render_pass_models, m_PipelineModelsPhong);
		glm::vec4 origin = {0.0f, 0.0f, 0.0f, 1.0f};
		glm::vec4 light_pos = origin * model2;

		for (Entity& current_entity : game_entities)
		{
			mvp = proj * m_StaticCamera.GetViewMatrix() * current_entity.GetTransformMatrix();
			glm::mat4 v_ubo[2] = { current_entity.GetTransformMatrix(), mvp};
			SDL_PushGPUVertexUniformData(cmd_buff, 0, glm::value_ptr(v_ubo[0]), sizeof(v_ubo));
			float f_ubo[16] = {
				0.97f, 0.64f, 0.12f, 0.0f,
				1.0f, 1.0f, 1.0f, 0.0f,
				light_pos.x, light_pos.y, light_pos.z, 0.0f,
				m_StaticCamera.pos.x, m_StaticCamera.pos.y, m_StaticCamera.pos.z, 0.0f
			};
			SDL_PushGPUFragmentUniformData(cmd_buff, 0, &f_ubo, sizeof(f_ubo));
			current_entity.Draw(render_pass_models, { current_entity.mesh.vbo, 0 }, { current_entity.mesh.ibo, 0 }, {current_entity.texture, m_Sampler});
		}

		SDL_EndGPURenderPass(render_pass_models);

		// Stage 3: UI Layer
		ui_layer.PushTextToUIBuff(m_Device, ui_buff, "LEVEL-01 YAY! :D", { 50.0f ,50.0f }, {0.98f, 0.37f, 0.87f, 1.0f}, test_font);

		SDL_GPURenderPass* render_pass_ui = SDL_BeginGPURenderPass(
			cmd_buff,
			&color_target_info,
			1,
			nullptr
		);
		if (!render_pass_ui) {
			SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to begin UI render pass: %s", SDL_GetError());
			std::abort();
		}
		SDL_BindGPUGraphicsPipeline(render_pass_ui, m_PipelineUI);

		SDL_GPUBufferBinding test_bind = { ui_buff, 0 };
		SDL_BindGPUVertexBuffers(render_pass_ui, 0, &test_bind, 1);
		SDL_GPUTextureSamplerBinding testtex_bind = { test_font.atlas_texture, m_Sampler };
		SDL_BindGPUFragmentSamplers(render_pass_models, 0, &testtex_bind, 1);

		SDL_PushGPUVertexUniformData(cmd_buff, 0, glm::value_ptr(proj_ui), sizeof(proj_ui));
		SDL_DrawGPUPrimitives(render_pass_ui, ui_layer.frame_offset / sizeof(Vertex), 1, 0, 0);


		SDL_EndGPURenderPass(render_pass_ui);
		ui_layer.FlushUIBuff(m_Device);

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
					break;
				}

				case SDL_EVENT_MOUSE_MOTION:
				{
					relx = ev.motion.xrel;
					rely = ev.motion.yrel;
					break;
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
		const float FRAME_TARGET_TIME = 1.0f / 60.0f;

		m_Timer.last_frame = m_Timer.current_frame;
		m_Timer.current_frame = SDL_GetTicks();
		m_Timer.elapsed_time = (m_Timer.current_frame - m_Timer.last_frame) / 1000.0f;

		if (m_Timer.elapsed_time < FRAME_TARGET_TIME) SDL_Delay((FRAME_TARGET_TIME - m_Timer.elapsed_time) * 1000.0f);
	}
}