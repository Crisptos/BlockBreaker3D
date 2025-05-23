#pragma once
#include <SDL3/SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <vector>
#include <stack>
#include <array>
#include <string>
#include <memory>
#include <functional>
#include "Camera.h"

#define DEPTH_TEXTURE_IDX 0
#define SKYBOX_TEXTURE_IDX 0xC

namespace BB3D
{
	// ENGINE DEFINES
	enum TextureType : Uint8
	{
		GEM10 = 0x1,
		GEM03 = 0x2,
		METAL07 = 0x3,
		PADDLE01 = 0x4,
		GEM13 = 0x5,
		METAL21 = 0x6,
		BLOCK1 = 0x7,
		BLOCK2 = 0x8,
		BLOCK3 = 0x9,
		BLOCK4 = 0xA,
		BLOCK5 = 0xB,
		SPACE_SKYBOX = 0xC,
		TECHNO_SKYBOX = 0xD,
		SINISTER_SKYBOX = 0xE,
		NETHER_SKYBOX = 0xF,
		CLASSIC_SKYBOX = 0x10,
		TEXTURETYPE_MAX = 0x11,
	};

	enum MeshType : Uint8
	{
		ICO = 0x0,
		QUAD = 0x1,
		SPHERE = 0x2,
		PADDLE = 0x3,
		BLOCK = 0x4
	};

	struct Timer
	{
		Uint64 last_frame;
		Uint64 current_frame;
		float elapsed_time;
	};

	struct InputState
	{
		bool current_keys[128];
		bool prev_keys[128];
		bool current_mousebtn[12];
		bool prev_mousebtn[12];
		float relx, rely;
		float current_mouse_x, current_mouse_y;
		float prev_mouse_x, prev_mouse_y;
	};

	struct Resolution
	{
		unsigned int w;
		unsigned int h;
	};

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
		float v[8];

		float& operator[](size_t i)
		{
			return v[i];
		}
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
	SDL_GPUGraphicsPipeline* CreateGraphicsPipelineForUI(SDL_GPUDevice* device, SDL_GPUTextureFormat color_target_format, SDL_GPUShader* vert_shader, SDL_GPUShader* frag_shader);
	SDL_GPUTexture* CreateAndLoadFontAtlasTextureToGPU(SDL_GPUDevice* device, const Uint32* atlas_buffer, Image atlas_props);

	// ________________________________ Fonts.cpp ________________________________
	struct Glyph {
		glm::ivec2   size;
		glm::ivec2   bearing;
		unsigned int advance;
	};

	struct FontAtlas
	{
		SDL_GPUTexture* atlas_texture = nullptr;
		std::vector<Glyph> glyph_metadata;
	};

	void InitFreeType();
	FontAtlas CreateFontAtlasFromFile(SDL_GPUDevice* device, const char* filepath);
	void DestroyFreeType();

	// ________________________________ Entity.cpp ________________________________
	struct Entity
	{
		MeshType mesh_type;
		TextureType texture_type;

		glm::mat4 transform;

		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;

		glm::vec3 velocity;

		bool is_shaded;
		bool is_active;

		void UpdateTransform();
		glm::mat4 GetTransformMatrix();
		void Draw(SDL_GPURenderPass* render_pass, SDL_GPUBufferBinding vbo_bind, SDL_GPUBufferBinding ibo_bind, SDL_GPUTextureSamplerBinding tex_bind, int ind_count);
	};

	// ________________________________ UI.cpp ________________________________
	struct UI_Element
	{
		SDL_GPUTexture* texture;

		glm::vec2 pos;
		glm::vec4 color;
		int width, height;
	};

	struct UI_TextField
	{
		std::string text;
		glm::vec2 pos;
		glm::vec4 color;
		bool is_visible = true;
	};

	struct UI
	{
		unsigned int frame_offset = 0; // 1 vertex + 32 bytes
		
		void PushTextToUIBuff(SDL_GPUDevice* device, SDL_GPUBuffer* ui_buff, UI_TextField text_field, FontAtlas& atlas, Resolution screen_res);
		void PushElementToUIBuff(SDL_GPUDevice* device, SDL_GPUBuffer* ui_buff, UI_Element elem);
		void FlushUIBuff(SDL_GPUDevice* device);
	};

	SDL_GPUBuffer* CreateUILayerBuffer(SDL_GPUDevice* device);

	// ________________________________ Scenes.cpp ________________________________
	enum SceneType : Uint8
	{
		QUIT,
		MAIN_MENU,
		GAMEPLAY,
		OPTIONS
	};

	class Scene
	{
	public:
		Scene(const char* filepath, std::function<void(SceneType)> trans_to_callback);

		virtual void Update(InputState& input_state, float delta_time) = 0;
		std::vector<Entity>& GetSceneEntities();
		std::vector<UI_Element>& GetSceneUIElems();
		std::vector<UI_TextField>& GetSceneUITextFields();
		Camera GetSceneCamera();

	protected:
		Camera m_SceneCam;

		std::vector<Entity> m_SceneEntities;
		std::vector<UI_Element> m_SceneElements;
		std::vector<UI_TextField> m_SceneTextfields;

		std::function<void(SceneType)> m_TransToCallback;
	};

	class MenuScene : public Scene
	{
	private:
		bool m_IsButtonsDown[3];

	public:
		MenuScene(const char* filepath, std::function<void(SceneType)> trans_to_callback);
		~MenuScene();

		void Update(InputState& input_state, float delta_time) override;

	private:
		void CheckMouseInput(InputState& input_state, float delta_time);
		bool IsInBox(float mouse_x, float mouse_y, glm::vec2 box_pos, float w, float h);
	};

	class OptionsScene : public Scene
	{
	private:
		bool m_IsButtonsDown[4];
		std::function<void()> m_ToggleSkyboxCallback;

	public:
		OptionsScene(const char* filepath, std::function<void(SceneType)> trans_to_callback, std::function<void()> toggle_skybox_callback);
		~OptionsScene();

		void Update(InputState& input_state, float delta_time) override;

	private:
		void CheckMouseInput(InputState& input_state, float delta_time);
		bool IsInBox(float mouse_x, float mouse_y, glm::vec2 box_pos, float w, float h);
	};

	class GameScene : public Scene
	{
	private:
		enum VelocityDir
		{
			DOWN,
			LEFT,
			UP,
			RIGHT
		};

		struct CollisionResult
		{
			bool is_colliding;
			VelocityDir collision_dir;
			glm::vec3 difference_vector;
		};

		struct
		{
			float radius;
			bool is_stuck;
		} m_BallState;

		int m_PaddleHitCount;

	public:
		GameScene(const char* filepath, std::function<void(SceneType)> trans_to_callback);
		~GameScene();

		void Update(InputState& input_state, float delta_time) override;
		CollisionResult IsBallColliding(glm::vec3 ball_pos, glm::vec3 collider_pos, float collider_width);

	private:
		void CheckCollisions();
		void UpdatePaddle(InputState& input_state, float delta_time);
		void UpdateBall(InputState& input_state, float delta_time);
		void ResetBall();
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
		static void SceneTransToCallback(SceneType type);
		static void OptionsToggleSkyboxCallback();
		void RecordKeyState(SDL_Keycode keycode, bool is_keydown);
		void RecordMouseBtnState(Uint8 mousebtn_idx, bool is_btndown);
		void CopyPrevInput();
		void UpdateDeltaTime();
		void ParseSettingsJSON();
		
	private:
		// State
		static bool s_IsRunning;
		bool m_IsIdle = false;
		Timer m_Timer;
		InputState m_InputState;
		static std::stack<std::unique_ptr<Scene>> s_SceneStack;
		UI ui_layer;

		// Options
		static TextureType s_SelectedTex;
		static Resolution s_Resolution;

		// SDL Context
		static SDL_Window* s_Window;
		static SDL_GPUDevice* s_Device;

		//	Renderer State
		SDL_GPUGraphicsPipeline* m_PipelineSkybox;
		SDL_GPUGraphicsPipeline* m_PipelineModelsPhong;
		SDL_GPUGraphicsPipeline* m_PipelineModelsNoPhong;
		SDL_GPUGraphicsPipeline* m_PipelineUI;
		SDL_GPUBuffer* m_UIBuff;
		std::vector<Mesh> m_Meshes;
		std::vector<SDL_GPUTexture*> m_Textures;

		//	Global texture sampler
		SDL_GPUSampler* m_Sampler;
	};
}