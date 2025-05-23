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
	glm::mat4 mvp(1.0f);

	glm::mat4 model2(1.0f);
	glm::mat4 model3(1.0f);

	FontAtlas test_font;

	// Engine Static Globals
	SDL_Window* Engine::s_Window;
	SDL_GPUDevice* Engine::s_Device;
	std::stack<std::unique_ptr<Scene>> Engine::s_SceneStack;
	bool Engine::s_IsRunning = true;
	TextureType Engine::s_SelectedTex = TextureType::SPACE_SKYBOX;
	Resolution Engine::s_Resolution = {1280, 720};

	// ________________________________ Engine Lifetime ________________________________

	void Engine::Init()
	{
		if (!SDL_Init(SDL_INIT_VIDEO))
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to initialize SDL3 and subsystems: %s\n", SDL_GetError());
			std::abort();
		}

		s_Window = SDL_CreateWindow(
			"Block Breaker 3D",
			s_Resolution.w,
			s_Resolution.h,
			SDL_WINDOW_VULKAN
		);

		if (!s_Window)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to initialize window: %s\n", SDL_GetError());
			std::abort();
		}

		SDL_SetWindowPosition(s_Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		SDL_SetWindowResizable(s_Window, false);

		s_Device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);

		if (!s_Device)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to acquire a handle to the GPU: %s\n", SDL_GetError());
			std::abort();
		}

		SDL_Log("OK: Created GPU handle with driver: %s\n", SDL_GetGPUDeviceDriver(s_Device));

		if (!SDL_ClaimWindowForGPUDevice(s_Device, s_Window))
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to claim window for device: %s\n", SDL_GetError());
			std::abort();
		}

		InitFreeType();
		ParseSettingsJSON();

		m_Timer.current_frame = SDL_GetTicks();
		m_Timer.last_frame = m_Timer.current_frame;
		m_Timer.elapsed_time = 0.0f;
	}

	void Engine::Run()
	{
		Setup();
		while (s_IsRunning)
		{
			if (!m_IsIdle)
			{
				Input();
				Update();
				Render();
				UpdateDeltaTime();
				CopyPrevInput();
			}
		}
	}

	void Engine::Destroy()
	{
		// Freetype and Fonts
		DestroyFreeType();

		// Dispose of all textures and meshes
		SDL_ReleaseGPUGraphicsPipeline(s_Device, m_PipelineModelsNoPhong);
		SDL_ReleaseGPUGraphicsPipeline(s_Device, m_PipelineModelsPhong);
		SDL_ReleaseGPUGraphicsPipeline(s_Device, m_PipelineSkybox);
		SDL_ReleaseGPUGraphicsPipeline(s_Device, m_PipelineUI);

		for (Mesh& disposed_mesh : m_Meshes)
		{
			SDL_ReleaseGPUBuffer(s_Device, disposed_mesh.vbo);
			SDL_ReleaseGPUBuffer(s_Device, disposed_mesh.ibo);
		}

		SDL_ReleaseGPUBuffer(s_Device, m_UIBuff);

		for (SDL_GPUTexture* disposed_texture : m_Textures)
		{
			SDL_ReleaseGPUTexture(s_Device, disposed_texture);
		}
		SDL_ReleaseGPUTexture(s_Device, test_font.atlas_texture);
		SDL_ReleaseGPUSampler(s_Device, m_Sampler);

		SDL_ReleaseWindowFromGPUDevice(s_Device, s_Window);
		SDL_DestroyWindow(s_Window);
		SDL_DestroyGPUDevice(s_Device);
		SDL_Quit();
	}

	// ________________________________ Setup ________________________________

	void Engine::Setup()
	{
		// Reset Input State
		std::memset(m_InputState.current_keys, 0, sizeof(m_InputState.current_keys));
		std::memset(m_InputState.prev_keys, 0, sizeof(m_InputState.prev_keys));
		std::memset(m_InputState.current_mousebtn, 0, sizeof(m_InputState.current_mousebtn));
		std::memset(m_InputState.prev_mousebtn, 0, sizeof(m_InputState.prev_mousebtn));
		m_InputState.relx = 0;
		m_InputState.rely = 0;

		// Allocate storage
		m_Meshes.reserve(16);
		m_Textures.reserve(16);

		// Load Textures
		// DEPTH TEXTURE IS ALWAYS IDX 0, SKYBOX TEXTURE IS ALWAYS IDX 1
		m_Textures.push_back(CreateDepthTestTexture(s_Device, s_Resolution.w, s_Resolution.h));
		m_Textures.push_back(CreateAndLoadTextureToGPU(s_Device, "assets/textures/gem_10.png"));
		m_Textures.push_back(CreateAndLoadTextureToGPU(s_Device, "assets/textures/gem_03.png"));
		m_Textures.push_back(CreateAndLoadTextureToGPU(s_Device, "assets/textures/metal_07.png"));
		m_Textures.push_back(CreateAndLoadTextureToGPU(s_Device, "assets/textures/paddle.png"));
		m_Textures.push_back(CreateAndLoadTextureToGPU(s_Device, "assets/textures/gem_13.png"));
		m_Textures.push_back(CreateAndLoadTextureToGPU(s_Device, "assets/textures/metal_21.png"));
		m_Textures.push_back(CreateAndLoadTextureToGPU(s_Device, "assets/textures/block_1.png"));
		m_Textures.push_back(CreateAndLoadTextureToGPU(s_Device, "assets/textures/block_2.png"));
		m_Textures.push_back(CreateAndLoadTextureToGPU(s_Device, "assets/textures/block_3.png"));
		m_Textures.push_back(CreateAndLoadTextureToGPU(s_Device, "assets/textures/block_4.png"));
		m_Textures.push_back(CreateAndLoadTextureToGPU(s_Device, "assets/textures/block_5.png"));
		m_Textures.push_back(CreateAndLoadCubeMapToGPU(
			s_Device,
			{
				"assets/skyboxes/space/space_right.png",
				"assets/skyboxes/space/space_left.png",
				"assets/skyboxes/space/space_up.png",
				"assets/skyboxes/space/space_down.png",
				"assets/skyboxes/space/space_front.png",
				"assets/skyboxes/space/space_back.png"
			}
		));
		m_Textures.push_back(CreateAndLoadCubeMapToGPU(
			s_Device,
			{
				"assets/skyboxes/techno/techno_right.png",
				"assets/skyboxes/techno/techno_left.png",
				"assets/skyboxes/techno/techno_up.png",
				"assets/skyboxes/techno/techno_down.png",
				"assets/skyboxes/techno/techno_front.png",
				"assets/skyboxes/techno/techno_back.png"
			}
		));
		m_Textures.push_back(CreateAndLoadCubeMapToGPU(
			s_Device,
			{
				"assets/skyboxes/sinister/sinister_right.png",
				"assets/skyboxes/sinister/sinister_left.png",
				"assets/skyboxes/sinister/sinister_up.png",
				"assets/skyboxes/sinister/sinister_down.png",
				"assets/skyboxes/sinister/sinister_front.png",
				"assets/skyboxes/sinister/sinister_back.png"
			}
		));
		m_Textures.push_back(CreateAndLoadCubeMapToGPU(
			s_Device,
			{
				"assets/skyboxes/nether/nether_right.png",
				"assets/skyboxes/nether/nether_left.png",
				"assets/skyboxes/nether/nether_up.png",
				"assets/skyboxes/nether/nether_down.png",
				"assets/skyboxes/nether/nether_front.png",
				"assets/skyboxes/nether/nether_back.png"
			}
		));
		m_Textures.push_back(CreateAndLoadCubeMapToGPU(
			s_Device,
			{
				"assets/skyboxes/classic/classic_right.png",
				"assets/skyboxes/classic/classic_left.png",
				"assets/skyboxes/classic/classic_up.png",
				"assets/skyboxes/classic/classic_down.png",
				"assets/skyboxes/classic/classic_front.png",
				"assets/skyboxes/classic/classic_back.png"
			}
		));

		test_font = CreateFontAtlasFromFile(s_Device, "assets/fonts/DejaVuSansMono.ttf");

		// Load Meshes
		m_Meshes.push_back(LoadMeshFromFile(s_Device, "assets/meshes/ico.obj"));
		m_Meshes.push_back(LoadMeshFromFile(s_Device, "assets/meshes/quad.obj"));
		m_Meshes.push_back(LoadMeshFromFile(s_Device, "assets/meshes/sphere.obj"));
		m_Meshes.push_back(LoadMeshFromFile(s_Device, "assets/meshes/paddle.obj"));
		m_Meshes.push_back(LoadMeshFromFile(s_Device, "assets/meshes/block.obj"));

		// Load Shaders and Setup Pipelines
		SDL_GPUShader* phong_vert_shader_model = CreateShaderFromFile(s_Device, "Shaders/model-phong.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1, 0, 0);
		SDL_GPUShader* phong_frag_shader_model = CreateShaderFromFile(s_Device, "Shaders/model-phong.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1, 0, 0);
		SDL_GPUShader* no_phong_vert_shader_model = CreateShaderFromFile(s_Device, "Shaders/model-no-phong.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1, 0, 0);
		SDL_GPUShader* no_phong_frag_shader_model = CreateShaderFromFile(s_Device, "Shaders/model-no-phong.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0, 0, 0);
		SDL_GPUShader* skybox_vert_shader = CreateShaderFromFile(s_Device, "Shaders/skybox.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1, 0, 0);
		SDL_GPUShader* skybox_frag_shader = CreateShaderFromFile(s_Device, "Shaders/skybox.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0, 0, 0);
		SDL_GPUShader* ui_vert_shader = CreateShaderFromFile(s_Device, "Shaders/ui.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1, 0, 0);
		SDL_GPUShader* ui_frag_shader = CreateShaderFromFile(s_Device, "Shaders/ui.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0, 0, 0);

		m_PipelineModelsPhong = CreateGraphicsPipelineForModels(
			s_Device, 
			SDL_GetGPUSwapchainTextureFormat(s_Device, s_Window),
			phong_vert_shader_model,
			phong_frag_shader_model
		);

		m_PipelineModelsNoPhong = CreateGraphicsPipelineForModels(
			s_Device,
			SDL_GetGPUSwapchainTextureFormat(s_Device, s_Window),
			no_phong_vert_shader_model,
			no_phong_frag_shader_model
		);

		m_PipelineSkybox = CreateGraphicsPipelineForSkybox(
			s_Device,
			SDL_GetGPUSwapchainTextureFormat(s_Device, s_Window),
			skybox_vert_shader,
			skybox_frag_shader
		);

		m_PipelineUI = CreateGraphicsPipelineForUI(
			s_Device,
			SDL_GetGPUSwapchainTextureFormat(s_Device, s_Window),
			ui_vert_shader,
			ui_frag_shader
		);

		SDL_ReleaseGPUShader(s_Device, phong_vert_shader_model);
		SDL_ReleaseGPUShader(s_Device, phong_frag_shader_model);
		SDL_ReleaseGPUShader(s_Device, no_phong_frag_shader_model);
		SDL_ReleaseGPUShader(s_Device, no_phong_vert_shader_model);
		SDL_ReleaseGPUShader(s_Device, skybox_vert_shader);
		SDL_ReleaseGPUShader(s_Device, skybox_frag_shader);
		SDL_ReleaseGPUShader(s_Device, ui_vert_shader);
		SDL_ReleaseGPUShader(s_Device, ui_frag_shader);

		m_Sampler = CreateSampler(s_Device, SDL_GPU_FILTER_NEAREST);

		m_UIBuff = CreateUILayerBuffer(s_Device);

		// Scene Initialization
		// TODO harcode gamescene as idx 0
		s_SceneStack.push(std::make_unique<MenuScene>("assets/scenes/mainmenu.json", SceneTransToCallback));
		
		// Uniform data
		float f_w = static_cast<float>(s_Resolution.w);
		float f_h = static_cast<float>(s_Resolution.h);
		proj = glm::perspective(glm::radians(60.0f), (f_w/f_h), 0.0001f, 1000.0f);
		proj_ui = glm::ortho(0.0f, 16.0f, 9.0f, 0.0f, -1.0f, 1.0f);
	}

	void Engine::Update()
	{
		s_SceneStack.top()->Update(m_InputState, m_Timer.elapsed_time);
	}

	// ________________________________ Runtime ________________________________
	void Engine::Render()
	{
		SDL_GPUCommandBuffer* cmd_buff = SDL_AcquireGPUCommandBuffer(s_Device);

		SDL_GPUTexture* swapchain_tex;
		if (!SDL_WaitAndAcquireGPUSwapchainTexture(
			cmd_buff, 
			s_Window, 
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
		depth_stencil_target_info.texture = m_Textures[DEPTH_TEXTURE_IDX];
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
		SDL_GPUTextureSamplerBinding skybox_bind = { m_Textures[s_SelectedTex], m_Sampler};
		SDL_BindGPUFragmentSamplers(render_pass_skybox, 0, &skybox_bind, 1);
		glm::mat4 vp_sky(1.0f);
		glm::mat4 view_no_transform = glm::mat4(glm::mat3(s_SceneStack.top()->GetSceneCamera().GetViewMatrix()));
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
		SDL_GPUBufferBinding binding = { m_Meshes[2].vbo, 0};
		SDL_BindGPUVertexBuffers(render_pass_models, 0, &binding, 1);
		SDL_GPUBufferBinding ind_bind = { m_Meshes[2].ibo, 0 };
		SDL_BindGPUIndexBuffer(render_pass_models, &ind_bind, SDL_GPU_INDEXELEMENTSIZE_16BIT);
		SDL_GPUTextureSamplerBinding tex_bind = {m_Textures[3], m_Sampler};
		SDL_BindGPUFragmentSamplers(render_pass_models, 0, &tex_bind, 1);

		// Stage 2: 3D Models

		// Light Sources
		glm::vec4 light_positions[32] = { glm::vec4(0.0f) };
		glm::vec4 origin = { 0.0f, 0.0f, 0.0f, 1.0f };
		int light_count = 0;
		SDL_BindGPUGraphicsPipeline(render_pass_models, m_PipelineModelsNoPhong);
		for (Entity& current_entity : s_SceneStack.top()->GetSceneEntities())
		{
			if (current_entity.is_shaded)
				continue;
			if (!current_entity.is_active)
				continue;

			mvp = proj * s_SceneStack.top()->GetSceneCamera().GetViewMatrix() * current_entity.GetTransformMatrix();
			SDL_PushGPUVertexUniformData(cmd_buff, 0, glm::value_ptr(mvp), sizeof(mvp));
			current_entity.Draw(render_pass_models, { m_Meshes[current_entity.mesh_type].vbo, 0 }, { m_Meshes[current_entity.mesh_type].ibo, 0 }, { m_Textures[current_entity.texture_type], m_Sampler }, m_Meshes[current_entity.mesh_type].ind_count);
			light_positions[light_count] = current_entity.GetTransformMatrix() * origin;
			light_count++;
		}

		SDL_BindGPUGraphicsPipeline(render_pass_models, m_PipelineModelsPhong);

		// Shaded Objects
		for (Entity& current_entity : s_SceneStack.top()->GetSceneEntities())
		{
			if (!current_entity.is_shaded)
				continue;
			if (!current_entity.is_active)
				continue;

			mvp = proj * s_SceneStack.top()->GetSceneCamera().GetViewMatrix() * current_entity.GetTransformMatrix();
			glm::mat4 v_ubo[2] = { current_entity.GetTransformMatrix(), mvp};
			SDL_PushGPUVertexUniformData(cmd_buff, 0, glm::value_ptr(v_ubo[0]), sizeof(v_ubo));
			// object color | pad
			// light color | pad
			// view pos | pad
			// num of lights | 3xpad
			// light pos array of vec4
			float f_ubo[144] = {
				0.97f, 0.64f, 0.12f, 0.0f,
				1.0f, 1.0f, 1.0f, 0.0f,
				s_SceneStack.top()->GetSceneCamera().pos.x, s_SceneStack.top()->GetSceneCamera().pos.y, s_SceneStack.top()->GetSceneCamera().pos.z, 0.0f,
				static_cast<float>(light_count), 0.0f, 0.0f, 0.0f
			};
			std::memcpy(f_ubo + 16, light_positions, sizeof(light_positions));

			SDL_PushGPUFragmentUniformData(cmd_buff, 0, &f_ubo, sizeof(f_ubo));
			current_entity.Draw(render_pass_models, { m_Meshes[current_entity.mesh_type].vbo, 0}, { m_Meshes[current_entity.mesh_type].ibo, 0}, { m_Textures[current_entity.texture_type], m_Sampler}, m_Meshes[current_entity.mesh_type].ind_count);
		}

		SDL_EndGPURenderPass(render_pass_models);

		// Stage 3: UI Layer
		for (UI_TextField text_field : s_SceneStack.top()->GetSceneUITextFields())
		{
			if(text_field.is_visible)
				ui_layer.PushTextToUIBuff(s_Device, m_UIBuff, text_field, test_font, s_Resolution);
		}

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

		SDL_GPUBufferBinding test_bind = { m_UIBuff, 0 };
		SDL_BindGPUVertexBuffers(render_pass_ui, 0, &test_bind, 1);
		SDL_GPUTextureSamplerBinding testtex_bind = { test_font.atlas_texture, m_Sampler };
		SDL_BindGPUFragmentSamplers(render_pass_models, 0, &testtex_bind, 1);

		SDL_PushGPUVertexUniformData(cmd_buff, 0, glm::value_ptr(proj_ui), sizeof(proj_ui));
		SDL_DrawGPUPrimitives(render_pass_ui, ui_layer.frame_offset / sizeof(Vertex), 1, 0, 0);


		SDL_EndGPURenderPass(render_pass_ui);
		ui_layer.FlushUIBuff(s_Device);

		if (!SDL_SubmitGPUCommandBuffer(cmd_buff))
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to submit command buffer to GPU: %s\n", SDL_GetError());
			std::abort();
		}
	}

	void Engine::Input()
	{
		// Free Camera code is debug only and will be gone at some point
		// TODO Clean up Camera Code
		float mouse_sensitivity = 0.3f;
		m_InputState.relx = 0;
		m_InputState.rely = 0;

		SDL_Event ev;
		while (SDL_PollEvent(&ev))
		{
			switch (ev.type)
			{
				case SDL_EVENT_QUIT:
				{
					s_IsRunning = false;
					break;
				}

				case SDL_EVENT_MOUSE_MOTION:
				{
					m_InputState.relx = ev.motion.xrel * mouse_sensitivity;
					m_InputState.rely = ev.motion.yrel * mouse_sensitivity;
					m_InputState.current_mouse_x = (ev.motion.x/s_Resolution.w) * 16;
					m_InputState.current_mouse_y = (ev.motion.y/s_Resolution.h) * 9;
					break;
				}

				case SDL_EVENT_KEY_DOWN:
				{
					RecordKeyState(ev.key.scancode, true);
					break;
				}

				case SDL_EVENT_KEY_UP:
				{
					RecordKeyState(ev.key.scancode, false);
					break;
				}

				case SDL_EVENT_MOUSE_BUTTON_DOWN:
				{
					RecordMouseBtnState(ev.button.button, true);
					break;
				}

				case SDL_EVENT_MOUSE_BUTTON_UP:
				{
					RecordMouseBtnState(ev.button.button, false);
					break;
				}
			}
		}
	}

	void Engine::SceneTransToCallback(SceneType type)
	{
		switch (type)
		{
			case SceneType::QUIT:
			{
				s_IsRunning = false;
				break;
			}

			case SceneType::MAIN_MENU:
			{
				SDL_ShowCursor();
				SDL_SetWindowRelativeMouseMode(s_Window, false);
				s_SceneStack.pop();
				break;
			}

			case SceneType::GAMEPLAY:
			{
				SDL_HideCursor();
				SDL_SetWindowRelativeMouseMode(s_Window, true);
				s_SceneStack.push(std::make_unique<GameScene>("assets/scenes/gameplay.json", SceneTransToCallback));
				break;
			}

			case SceneType::OPTIONS:
			{
				s_SceneStack.push(std::make_unique<OptionsScene>("assets/scenes/optionsmenu.json", SceneTransToCallback, OptionsToggleSkyboxCallback));
				break;
			}
		}
	}

	void Engine::OptionsToggleSkyboxCallback()
	{
		int tex_idx = static_cast<int>(s_SelectedTex);

		tex_idx += 1;
		if (tex_idx >= TextureType::TEXTURETYPE_MAX) tex_idx = 12;

		s_SelectedTex = static_cast<TextureType>(tex_idx);
	}

	void Engine::RecordKeyState(SDL_Keycode keycode, bool is_keydown)
	{
		if (keycode < 0 || keycode >= 128) 
			return;

		if (m_InputState.current_keys[keycode] != is_keydown)
			m_InputState.current_keys[keycode] = is_keydown;
	}

	void Engine::RecordMouseBtnState(Uint8 mousebtn_idx, bool is_btndown)
	{
		if (mousebtn_idx < 0 || mousebtn_idx >= 12)
			return;

		if (m_InputState.current_mousebtn[mousebtn_idx] != is_btndown)
			m_InputState.current_mousebtn[mousebtn_idx] = is_btndown;
	}

	void Engine::CopyPrevInput()
	{
		std::memcpy(m_InputState.prev_keys, m_InputState.current_keys, sizeof(m_InputState.current_keys));
		std::memcpy(m_InputState.prev_mousebtn, m_InputState.current_mousebtn, sizeof(m_InputState.current_mousebtn));
		m_InputState.prev_mouse_x = m_InputState.current_mouse_x;
		m_InputState.prev_mouse_y = m_InputState.current_mouse_y;
	}

	void Engine::UpdateDeltaTime()
	{
		const float FRAME_TARGET_TIME = 1.0f / 60.0f;

		m_Timer.last_frame = m_Timer.current_frame;
		m_Timer.current_frame = SDL_GetTicks();
		m_Timer.elapsed_time = (m_Timer.current_frame - m_Timer.last_frame) / 1000.0f;

		if (m_Timer.elapsed_time < FRAME_TARGET_TIME) SDL_Delay((FRAME_TARGET_TIME - m_Timer.elapsed_time) * 1000.0f);
	}

	void Engine::ParseSettingsJSON()
	{
		const char* SETTINGS_PATH = "bb3d_settings.json";
	}
}