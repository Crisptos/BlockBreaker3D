#include "Engine.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
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
			bool is_active = loaded_entity["is_active"];

			// MeshType | TextureType | Transform Matrix | Pos | Rot | Scale | Velocity | Apply Shading?
			m_SceneEntities.push_back({ mesh_t, texture_t, glm::mat4(1.0f), pos, rot, scale, glm::vec3(0.0f), is_shaded, is_active });
		}

		// Build UI Elems and push to list

		// Build UI Textfields and push to list
		for (const auto& loaded_text : scene_data["textfields"])
		{
			std::string text = loaded_text["text"];
			glm::vec2 pos = { loaded_text["position"][0], loaded_text["position"][1] };
			glm::vec4 color = { loaded_text["color"][0], loaded_text["color"][1], loaded_text["color"][2], loaded_text["color"][3] };
			bool is_visible = loaded_text["is_visible"];

			// MeshType | TextureType | Transform Matrix | Pos | Rot | Scale | Velocity | Apply Shading?
			m_SceneTextfields.push_back({ text, pos, color, is_visible });
		}
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
		m_SceneCam.pos = glm::vec3(0.0f, 1.0f, 4.0f);
		m_SceneCam.front = glm::vec3(0.0f, 0.0f, -1.0f);
		m_SceneCam.up = glm::vec3(0.0f, 1.0f, 0.0f);
		m_SceneCam.pitch = -30.0f;
		m_SceneCam.yaw = -90.0f;
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

		m_SceneEntities[0].rotation.x += 8.0f * delta_time;
		m_SceneEntities[0].rotation.y += 4.5f * delta_time;
		m_SceneEntities[0].rotation.z += 6.0f * delta_time;

		if(m_SceneEntities[0].rotation.x >= 360.0f)
			m_SceneEntities[0].rotation.x == 0.0f;
		if(m_SceneEntities[0].rotation.y >= 360.0f)
			m_SceneEntities[0].rotation.x == 0.0f;
		if(m_SceneEntities[0].rotation.z >= 360.0f)
			m_SceneEntities[0].rotation.x == 0.0f;

		for (Entity& current_entity : m_SceneEntities)
		{
			current_entity.UpdateTransform();
		}
	}

	// ________________________________ GameScene ________________________________
	GameScene::GameScene(const char* filepath, std::function<void(SceneType)> trans_to_callback) : Scene(filepath, trans_to_callback)
	{
		m_SceneCam.pos = glm::vec3(0.0f, 9.0f, 10.0f);
		m_SceneCam.front = glm::vec3(0.0f, 0.0f, -1.0f);
		m_SceneCam.up = glm::vec3(0.0f, 1.0f, 0.0f);
		m_SceneCam.pitch = -50.0f;
		m_SceneCam.yaw = -90.0f;

		m_SceneEntities[2].velocity.x =  1.5f;
		m_SceneEntities[2].velocity.z = -1.4f;

		// Initialize Block Locations
		// TODO Remove Test Map
		const Uint8 BLOCK_MAP[6 * 6] =
		{
			0x8, 0x9, 0x0, 0x0, 0x8, 0x9,
			0xB, 0xA, 0xB, 0xA, 0xB, 0xA,
			0x9, 0x8, 0x9, 0x8, 0x9, 0x8,
			0xA, 0xB, 0xA, 0xB, 0xA, 0xB,
			0xB, 0xC, 0x0, 0x0, 0xB, 0xC,
			0xC, 0xB, 0x0, 0x0, 0xC, 0xB
		};
		for (int z = 0; z < 6; z++)
		{
			for (int x = 0; x < 6; x++)
			{
				if (BLOCK_MAP[z * 6 + x] == 0x0)
					continue;
				
				float x_offset = -5.0f + (x * 2.0f);
				float z_offset = -5.0f + (z * 1.0f);
				Entity new_block = {};
				new_block.mesh_type = MeshType::BLOCK;
				new_block.texture_type = static_cast<TextureType>(BLOCK_MAP[z * 6 + x]);
				new_block.transform = glm::mat4(1.0f);
				new_block.position = glm::vec3(x_offset, 0.0f, z_offset);
				new_block.rotation = glm::vec3(0.0f);
				new_block.scale = glm::vec3(0.5f);
				new_block.velocity = glm::vec3(0.0f);
				new_block.is_shaded = true;

				m_SceneEntities.push_back(new_block);
			}
		}
		
		
	}

	GameScene::~GameScene()
	{

	}

	void GameScene::Update(InputState& input_state, float delta_time)
	{
		// Main gameplay loop logic
		// ___________________________________
		//	Input
		if (input_state.current_keys[SDL_SCANCODE_LEFT])
		{
			m_SceneEntities[0].position.x -= 4.0f * delta_time;
		}

		if (input_state.current_keys[SDL_SCANCODE_RIGHT])
		{
			m_SceneEntities[0].position.x += 4.0f * delta_time;
		}

		if (input_state.current_keys[SDL_SCANCODE_EQUALS] && !input_state.prev_keys[SDL_SCANCODE_EQUALS])
		{
			is_dbg = true;
			// TODO improve this
			m_SceneTextfields[1].is_visible = true;
		}

		if (input_state.current_keys[SDL_SCANCODE_MINUS] && !input_state.prev_keys[SDL_SCANCODE_MINUS])
		{
			is_dbg = false;
			m_SceneTextfields[1].is_visible = false;
		}

		// Not the cleanest solution but doing it for the sake of time
		// TODO Remove this
		Entity* ball_ref = m_SceneEntities.data() + 2;
		ball_ref->position += ball_ref->velocity * delta_time;

		//	Collision

		if (input_state.current_keys[SDL_SCANCODE_B] && !input_state.prev_keys[SDL_SCANCODE_B])
		{
			printf("Ball Pos: (%.2f, %.2f, %.2f)\n", ball_ref->position.x, ball_ref->position.y, ball_ref->position.z);
			printf("Camera Pos: (%.2f, %.2f, %.2f)\n", m_SceneCam.pos.x, m_SceneCam.pos.y, m_SceneCam.pos.z);
		}

		if (ball_ref->position.x > 7.0f || ball_ref->position.x < -7.0f)
		{
			ball_ref->velocity.x *= -1;
		}

		if (ball_ref->position.z > 7.0f || ball_ref->position.z < -7.0f)
		{
			ball_ref->velocity.z *= -1;
		}

		// Check collision on each block
		// Iterating through every block on the map should be fine for our purposes
		for (Entity& current_entity : m_SceneEntities)
		{
			if (current_entity.mesh_type != MeshType::BLOCK && !current_entity.is_active)
				continue;

			if (IsBallColliding(ball_ref->position, current_entity.position))
			{
				printf("HIT\n");
				current_entity.is_active = false;
			}
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

		if (is_dbg)
		{
			if (keys[SDL_SCANCODE_W])
				m_SceneCam.pos += camera_speed * m_SceneCam.front;
			if (keys[SDL_SCANCODE_A])
				m_SceneCam.pos -= glm::normalize(glm::cross(m_SceneCam.front, m_SceneCam.up)) * camera_speed;
			if (keys[SDL_SCANCODE_S])
				m_SceneCam.pos -= camera_speed * m_SceneCam.front;
			if (keys[SDL_SCANCODE_D])
				m_SceneCam.pos += glm::normalize(glm::cross(m_SceneCam.front, m_SceneCam.up)) * camera_speed;
		}
		// ___________________________________
	}

	bool GameScene::IsBallColliding(glm::vec3 ball_pos, glm::vec3 collider_pos)
	{
		// Closest point to the sphere within the AABB box of the paddle
		const float x = std::fmaxf(collider_pos.x - 3.0f, std::fminf(ball_pos.x, collider_pos.x + 3.0f));
		const float y = std::fmaxf(collider_pos.y, std::fminf(ball_pos.y, collider_pos.y));;
		const float z = std::fmaxf(collider_pos.z - 0.5f, std::fminf(ball_pos.z, collider_pos.z + 0.5f));;


		// Euclidean distance between Point - Sphere
		float distance = sqrtf(
			((x - ball_pos.x) * (x - ball_pos.x)) +
			((y - ball_pos.y) * (y - ball_pos.y)) +
			((z - ball_pos.z) * (z - ball_pos.z))
		);

		return distance <= 1.0f;


	}
}