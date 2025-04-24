#include "Engine.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include "nlohmann/json.hpp"

namespace BB3D
{
	// TEMP DEBUG FLYMODE CAMERA
	bool is_dbg = false;

	// Base scene implementation
	Scene::Scene(const char* filepath, std::function<void(SceneType)> trans_to_callback)
	{
		m_TransToCallback = trans_to_callback;

		m_SceneEntities.reserve(16);
		m_SceneElements.reserve(16);
		m_SceneTextfields.reserve(16);

		// Parse scene data
		std::ifstream scene_f(filepath);
		if (!scene_f)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to load scene json at: %s\n", filepath);
			std::abort();
		}

		std::stringstream json_buffer;
		json_buffer << scene_f.rdbuf();
		std::string json_string = json_buffer.str();

		nlohmann::json scene_data;
		scene_data = nlohmann::json::parse(json_string);

		if (!scene_data.is_object())
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to parse scene json at: %s\n", "my filepath");
			std::abort();
		}

		// Build entities and push to list
		for (const auto& loaded_entity : scene_data["entities"])
		{
			MeshType mesh_t = loaded_entity["mesh"];
			TextureType texture_t = loaded_entity["texture"];
			glm::vec3 pos = { loaded_entity["position"][0], loaded_entity["position"][1], loaded_entity["position"][2] };
			glm::vec3 rot = { loaded_entity["rotation"][0], loaded_entity["rotation"][1], loaded_entity["rotation"][2] };
			glm::vec3 scale = { loaded_entity["scale"][0], loaded_entity["scale"][1], loaded_entity["scale"][2] };
			bool is_shaded = loaded_entity["is_shaded"];

			// MeshType | TextureType | Transform Matrix | Pos | Rot | Scale | Velocity | Apply Shading?
			m_SceneEntities.push_back({ mesh_t, texture_t, glm::mat4(1.0f), pos, rot, scale, glm::vec3(0.0f), is_shaded });
		}

		// Build UI Elems and push to list

		// Build UI Textfields and push to list
		for (const auto& loaded_text : scene_data["textfields"])
		{
			std::string text = loaded_text["text"];
			glm::vec2 pos = { loaded_text["position"][0], loaded_text["position"][1] };
			glm::vec4 color = { loaded_text["color"][0], loaded_text["color"][1], loaded_text["color"][2], loaded_text["color"][3] };

			// MeshType | TextureType | Transform Matrix | Pos | Rot | Scale | Velocity | Apply Shading?
			m_SceneTextfields.push_back({ text, pos, color });
		}

		// Camera Setup
		m_SceneCam.pos = glm::vec3(0.0f, 3.0f, 7.0f);
		m_SceneCam.front = glm::vec3(0.0f, 0.0f, -1.0f);
		m_SceneCam.up = glm::vec3(0.0f, 1.0f, 0.0f);
		m_SceneCam.pitch = -30.0f;
		m_SceneCam.yaw = -90.0f;
	}

	std::vector<Entity>& Scene::GetSceneEntities()
	{
		return m_SceneEntities;
	}

	std::vector<UI_Element>& Scene::GetSceneUIElems()
	{
		return m_SceneElements;
	}

	std::vector<UI_TextField>& Scene::GetSceneUITextFields()
	{
		return m_SceneTextfields;
	}

	Camera Scene::GetSceneCamera()
	{
		return m_SceneCam;
	}

	// ________________________________ MenuScene ________________________________
	MenuScene::MenuScene(const char* filepath, std::function<void(SceneType)> trans_to_callback) : Scene(filepath, trans_to_callback)
	{

	}

	MenuScene::~MenuScene()
	{

	}

	void MenuScene::Update(InputState& input_state, float delta_time)
	{
		// Test scene transition
		if (input_state.current_keys[SDL_SCANCODE_S] && !input_state.prev_keys[SDL_SCANCODE_S])
		{
			m_TransToCallback(SceneType::GAMEPLAY);
		}
	}

	// ________________________________ GameScene ________________________________
	GameScene::GameScene(const char* filepath, std::function<void(SceneType)> trans_to_callback) : Scene(filepath, trans_to_callback)
	{

	}

	GameScene::~GameScene()
	{

	}

	void GameScene::Update(InputState& input_state, float delta_time)
	{
		// Main gameplay loop logic
		// ___________________________________
		if (input_state.current_keys[SDL_SCANCODE_LEFT])
		{
			m_SceneEntities[0].position.x -= 4.0f * delta_time;
		}

		if (input_state.current_keys[SDL_SCANCODE_RIGHT])
		{
			m_SceneEntities[0].position.x += 4.0f * delta_time;
		}

		if (input_state.current_keys[SDL_SCANCODE_UP])
		{
			m_SceneEntities[0].position.z -= 4.0f * delta_time;
		}

		if (input_state.current_keys[SDL_SCANCODE_DOWN])
		{
			m_SceneEntities[0].position.z += 4.0f * delta_time;
		}

		if (input_state.current_keys[SDL_SCANCODE_EQUALS] && !input_state.prev_keys[SDL_SCANCODE_EQUALS])
		{
			is_dbg = true;
		}

		if (input_state.current_keys[SDL_SCANCODE_MINUS] && !input_state.prev_keys[SDL_SCANCODE_MINUS])
		{
			is_dbg = false;
		}

		for (Entity& current_entity : m_SceneEntities)
		{
			current_entity.UpdateTransform();
		}

		// TEMP DEBUG FLYMODE
		// ___________________________________
		if (is_dbg)
		{
			m_SceneCam.yaw += input_state.relx;
			m_SceneCam.pitch -= input_state.rely;
		}

			if (m_SceneCam.pitch > 89.0f)
				m_SceneCam.pitch = 89.0f;
			if (m_SceneCam.pitch < -89.0f)
				m_SceneCam.pitch = -89.0f;

			glm::vec3 direction;
			direction.x = cos(glm::radians(m_SceneCam.yaw)) * cos(glm::radians(m_SceneCam.pitch));
			direction.y = sin(glm::radians(m_SceneCam.pitch));
			direction.z = sin(glm::radians(m_SceneCam.yaw)) * cos(glm::radians(m_SceneCam.pitch));
			m_SceneCam.front = glm::normalize(direction);

			const bool* keys = SDL_GetKeyboardState(0);

			const float camera_speed = 2.0f * delta_time;

			if (keys[SDL_SCANCODE_W])
				m_SceneCam.pos += camera_speed * m_SceneCam.front;
			if (keys[SDL_SCANCODE_A])
				m_SceneCam.pos -= glm::normalize(glm::cross(m_SceneCam.front, m_SceneCam.up)) * camera_speed;
			if (keys[SDL_SCANCODE_S])
				m_SceneCam.pos -= camera_speed * m_SceneCam.front;
			if (keys[SDL_SCANCODE_D])
				m_SceneCam.pos += glm::normalize(glm::cross(m_SceneCam.front, m_SceneCam.up)) * camera_speed;
		// ___________________________________
	}
}