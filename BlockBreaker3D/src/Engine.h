#pragma once
#include <SDL3/SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <vector>
#include <array>
#include <string>
#include "Camera.h"

#define DEPTH_TEXTURE_IDX 0
#define SKYBOX_TEXTURE_IDX 1

namespace BB3D
{
	// ENGINE DEFINES
	enum TextureType : Uint8
	{
		GEM10 = 0x2,
		GEM03 = 0x3,
		METAL07 = 0x4
	};

	enum MeshType : Uint8
	{
		ICO = 0x0,
		QUAD = 0x1,
		SPHERE = 0x2
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
		SDL_GPUTexture* atlas_texture;
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
	};

	struct UI
	{
		unsigned int frame_offset = 0; // 1 vertex + 32 bytes
		
		void PushTextToUIBuff(SDL_GPUDevice* device, SDL_GPUBuffer* ui_buff, UI_TextField text_field, FontAtlas& atlas);
		void PushElementToUIBuff(SDL_GPUDevice* device, SDL_GPUBuffer* ui_buff, UI_Element& elem);
		void FlushUIBuff(SDL_GPUDevice* device);
	};

	SDL_GPUBuffer* CreateUILayerBuffer(SDL_GPUDevice* device);

	// ________________________________ Scenes.cpp ________________________________
	enum SceneType : Uint8
	{
		MAIN_MENU,
		OPTIONS,
		GAMEPLAY
	};

	class IScene
	{
	public:
		virtual void Update(InputState& input_state) = 0;
		virtual std::vector<Entity>& GetSceneEntities() = 0;
		virtual const std::vector<UI_Element>& GetSceneUIElems() const = 0;
		virtual const std::vector<UI_TextField>& GetSceneUITextFields() const = 0;
	};

	class GameScene : public IScene
	{
	public:
		GameScene();
		~GameScene();

		void Update(InputState& input_state) override;
		std::vector<Entity>& GetSceneEntities() override;
		const std::vector<UI_Element>& GetSceneUIElems() const override;
		const std::vector<UI_TextField>& GetSceneUITextFields() const override;

	private:
		

	private:
		std::vector<Entity> m_GameEntities;
		std::vector<UI_Element> m_GameElements;
		std::vector<UI_TextField> m_GameTextfields;
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
		void RecordKeyState(SDL_Keycode keycode, bool is_keydown);
		void CopyPrevKeys();
		void Render();

		// Utility
		void UpdateDeltaTime();
		
	private:

		// State
		bool m_IsRunning = true;
		bool m_IsIdle = false;
		Timer m_Timer;
		InputState m_InputState;

		Camera m_StaticCamera;
		GameScene TEST; // TODO REMOVE
		UI ui_layer;

		// SDL Context
		SDL_Window* m_Window;
		SDL_GPUDevice* m_Device;

		//	Renderer State
		SDL_GPUGraphicsPipeline* m_PipelineSkybox;
		SDL_GPUGraphicsPipeline* m_PipelineModelsPhong;
		SDL_GPUGraphicsPipeline* m_PipelineModelsNoPhong;
		SDL_GPUGraphicsPipeline* m_PipelineUI;
		SDL_GPUBuffer* ui_buff;
		std::vector<Mesh> meshes;
		std::vector<SDL_GPUTexture*> textures;

		//	Global texture sampler
		SDL_GPUSampler* m_Sampler;
	};
}