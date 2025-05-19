#include "Engine.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "nlohmann/json.hpp"

namespace BB3D
{
	// TEMP DEBUG FLYMODE CAMERA
	const glm::vec4 SELECTED_ELEM_COLOR = { 0.98, 0.37, 0.87, 1.0 };
	const glm::vec4 NOT_SELECTED_ELEM_COLOR = { 0.96, 0.96, 0.96, 1.0 };
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

		std::memset(m_IsButtonsDown, 0, sizeof(m_IsButtonsDown));
	}

	MenuScene::~MenuScene()
	{

	}

	void MenuScene::Update(InputState& input_state, float delta_time)
	{
		// Input
		CheckMouseInput(input_state, delta_time);

		m_SceneEntities[0].rotation.x += 8.0f * delta_time;
		m_SceneEntities[0].rotation.y += 4.5f * delta_time;
		m_SceneEntities[0].rotation.z += 6.0f * delta_time;

		if(m_SceneEntities[0].rotation.x >= 360.0f)
			m_SceneEntities[0].rotation.x = 0.0f;
		if(m_SceneEntities[0].rotation.y >= 360.0f)
			m_SceneEntities[0].rotation.x = 0.0f;
		if(m_SceneEntities[0].rotation.z >= 360.0f)
			m_SceneEntities[0].rotation.x = 0.0f;

		for (Entity& current_entity : m_SceneEntities)
		{
			current_entity.UpdateTransform();
		}
	}

	void MenuScene::CheckMouseInput(InputState& input_state, float delta_time)
	{
		bool in_play = IsInBox(input_state.current_mouse_x, input_state.current_mouse_y, m_SceneTextfields[2].pos, 110, 40);
		bool in_options = IsInBox(input_state.current_mouse_x, input_state.current_mouse_y, m_SceneTextfields[3].pos, 180, 40);
		bool in_quit = IsInBox(input_state.current_mouse_x, input_state.current_mouse_y, m_SceneTextfields[4].pos, 100, 40);

		if (!in_play)
		{
			m_IsButtonsDown[0] = false;
			m_SceneTextfields[2].color = NOT_SELECTED_ELEM_COLOR;
		}

		if (!in_options)
		{
			m_IsButtonsDown[1] = false;
			m_SceneTextfields[3].color = NOT_SELECTED_ELEM_COLOR;
		}

		if (!in_quit)
		{
			m_IsButtonsDown[2] = false;
			m_SceneTextfields[4].color = NOT_SELECTED_ELEM_COLOR;
		}

		// TODO Clean this up a bit
		// Play Button
		if (
			input_state.current_mousebtn[1] && !input_state.prev_mousebtn[1] &&
			in_play &&
			!m_IsButtonsDown[0])
		{
			printf("Pressed Play Button Down!\n");
			m_IsButtonsDown[0] = true;
			m_SceneTextfields[2].color = SELECTED_ELEM_COLOR;
		}

		if (
			!input_state.current_mousebtn[1] && input_state.prev_mousebtn[1] &&
			in_play &&
			m_IsButtonsDown[0])
		{
			printf("Released Play Button Up!\n");
			m_IsButtonsDown[0] = false;
			m_SceneTextfields[2].color = NOT_SELECTED_ELEM_COLOR;
			m_TransToCallback(SceneType::GAMEPLAY);
			return;
		}

		// Options Button
		if (
			input_state.current_mousebtn[1] && !input_state.prev_mousebtn[1] &&
			in_options &&
			!m_IsButtonsDown[1])
		{
			printf("Pressed Options Button Down!\n");
			m_IsButtonsDown[1] = true;
			m_SceneTextfields[3].color = SELECTED_ELEM_COLOR;
		}

		if (
			!input_state.current_mousebtn[1] && input_state.prev_mousebtn[1] &&
			in_options &&
			m_IsButtonsDown[1])
		{
			printf("Released Options Button Up!\n");
			m_IsButtonsDown[1] = false;
			m_SceneTextfields[3].color = NOT_SELECTED_ELEM_COLOR;
		}

		// Quit Button
		if (
			input_state.current_mousebtn[1] && !input_state.prev_mousebtn[1] &&
			in_quit &&
			!m_IsButtonsDown[2])
		{
			printf("Pressed Quit Button Down!\n");
			m_IsButtonsDown[2] = true;
			m_SceneTextfields[4].color = SELECTED_ELEM_COLOR;
		}

		if (
			!input_state.current_mousebtn[1] && input_state.prev_mousebtn[1] &&
			in_quit &&
			m_IsButtonsDown[2])
		{
			printf("Released Quit Button Up!\n");
			m_IsButtonsDown[2] = false;
			m_SceneTextfields[4].color = NOT_SELECTED_ELEM_COLOR;
			m_TransToCallback(SceneType::QUIT);
			return;
		}
			
	}

	bool MenuScene::IsInBox(int mouse_x, int mouse_y, glm::vec2 box_pos, int w, int h)
	{
		if (mouse_x < box_pos.x + w && mouse_x > box_pos.x && mouse_y < box_pos.y + h && mouse_y > box_pos.y)
			return true;

		return false;
	}

	// ________________________________ GameScene ________________________________
	GameScene::GameScene(const char* filepath, std::function<void(SceneType)> trans_to_callback) : Scene(filepath, trans_to_callback)
	{
		m_SceneCam.pos = glm::vec3(0.0f, 9.0f, 10.0f);
		m_SceneCam.front = glm::vec3(0.0f, 0.0f, -1.0f);
		m_SceneCam.up = glm::vec3(0.0f, 1.0f, 0.0f);
		m_SceneCam.pitch = -50.0f;
		m_SceneCam.yaw = -90.0f;

		ResetBall();
		m_BallState.radius = 1.0f;

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
				new_block.is_active = true;

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
		if (input_state.current_keys[SDL_SCANCODE_ESCAPE] && !input_state.prev_keys[SDL_SCANCODE_ESCAPE])
		{
			m_TransToCallback(SceneType::MAIN_MENU);
			return;
		}

		if (input_state.current_keys[SDL_SCANCODE_SPACE] && !input_state.prev_keys[SDL_SCANCODE_SPACE])
		{
			m_BallState.is_stuck = false;
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

		UpdatePaddle(input_state, delta_time);
		UpdateBall(input_state, delta_time);
		CheckCollisions();

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

	void GameScene::CheckCollisions()
	{
		// Blocks
		for (Entity& current_entity : m_SceneEntities)
		{
			if (current_entity.mesh_type != MeshType::BLOCK)
				continue;
			if (!current_entity.is_active)
				continue;

			CollisionResult result = IsBallColliding(m_SceneEntities[2].position, current_entity.position, 0.5f);
			if (result.is_colliding)
			{
				printf("HIT BLOCK\n");
				printf("Collision Dir: %d\n", result.collision_dir);
				current_entity.is_active = false;

				if (result.collision_dir == VelocityDir::LEFT || result.collision_dir == VelocityDir::RIGHT)
				{
					m_SceneEntities[2].velocity.x *= -1.0f;

					// relocate ball
					float penetration = m_BallState.radius - std::abs(result.difference_vector.x);
					if (result.collision_dir == VelocityDir::LEFT)
						m_SceneEntities[2].position.x -= penetration;
					else
						m_SceneEntities[2].position.x += penetration;
				}

				if (result.collision_dir == VelocityDir::UP || result.collision_dir == VelocityDir::DOWN)
				{
					m_SceneEntities[2].velocity.z *= -1.0f;

					// relocate ball
					float penetration = m_BallState.radius - std::abs(result.difference_vector.z);
					if (result.collision_dir == VelocityDir::UP)
						m_SceneEntities[2].position.z -= penetration;
					else
						m_SceneEntities[2].position.z += penetration;
				}
			}
		}

		// Paddle
		if(!m_BallState.is_stuck)
		{
			CollisionResult result = IsBallColliding(m_SceneEntities[2].position, m_SceneEntities[0].position, 1.0f);
			if (result.is_colliding)
			{
				printf("HIT PADDLE\n");
				float distance = m_SceneEntities[2].position.x - m_SceneEntities[0].position.x;
				float strength = 2.0f;
				glm::vec3 old_velocity = m_SceneEntities[2].velocity;
				m_SceneEntities[2].velocity.x = distance * strength;
				m_SceneEntities[2].velocity.z = -1.0f * std::abs(m_SceneEntities[2].velocity.z);
				m_SceneEntities[2].velocity = glm::normalize(m_SceneEntities[2].velocity) * glm::length(old_velocity);

			}
		}
	}

	void GameScene::UpdatePaddle(InputState& input_state, float delta_time)
	{
		if (input_state.current_keys[SDL_SCANCODE_LEFT])
		{
			m_SceneEntities[0].position.x -= 4.0f * delta_time;
		}

		if (input_state.current_keys[SDL_SCANCODE_RIGHT])
		{
			m_SceneEntities[0].position.x += 4.0f * delta_time;
		}

		if (m_SceneEntities[0].position.x > 5.5f)
		{
			m_SceneEntities[0].position.x = 5.5f;
		}

		if (m_SceneEntities[0].position.x < -5.5f)
		{
			m_SceneEntities[0].position.x = -5.5f;
		}

	}

	void GameScene::UpdateBall(InputState& input_state, float delta_time)
	{
		// Update Position
		if (m_BallState.is_stuck)
		{
			m_SceneEntities[2].position = { m_SceneEntities[0].position.x, m_SceneEntities[0].position.y, m_SceneEntities[0].position.z - m_BallState.radius };
		}

		if (!m_BallState.is_stuck)
		{
			m_SceneEntities[2].position += m_SceneEntities[2].velocity * delta_time;
			// Bounds Checking

			if (m_SceneEntities[2].position.x > 6.0f || m_SceneEntities[2].position.x < -6.0f)
			{
				m_SceneEntities[2].velocity.x *= -1;
			}

			if (m_SceneEntities[2].position.z < -6.0f)
			{
				m_SceneEntities[2].velocity.z *= -1;
			}

			if (m_SceneEntities[2].position.z > 7.5f)
			{
				ResetBall();
			}
		}

		if (input_state.current_keys[SDL_SCANCODE_B] && !input_state.prev_keys[SDL_SCANCODE_B])
		{
			printf("Ball Pos: (%.2f, %.2f, %.2f)\n", m_SceneEntities[2].position.x, m_SceneEntities[2].position.y, m_SceneEntities[2].position.z);
			printf("Ball Velocity: (%.2f, %.2f)\n", m_SceneEntities[2].velocity.x, m_SceneEntities[2].velocity.z);
			printf("Paddle Pos: (%.2f, %.2f, %.2f)\n", m_SceneEntities[0].position.x, m_SceneEntities[0].position.y, m_SceneEntities[0].position.z);
			printf("Camera Pos: (%.2f, %.2f, %.2f)\n", m_SceneCam.pos.x, m_SceneCam.pos.y, m_SceneCam.pos.z);
		}
	}

	void GameScene::ResetBall()
	{
		m_BallState.is_stuck = true;
		m_SceneEntities[2].position = { m_SceneEntities[0].position.x, m_SceneEntities[0].position.y, m_SceneEntities[0].position.z - m_BallState.radius };
		m_SceneEntities[2].velocity.x = 3.5f;
		m_SceneEntities[2].velocity.z = -3.4f;
	}

	GameScene::CollisionResult GameScene::IsBallColliding(glm::vec3 ball_pos, glm::vec3 collider_pos, float collider_width)
	{
		// Closest point to the sphere within the AABB box of the paddle
		glm::vec3 closest_point(0.0f);

		closest_point.x = std::fmaxf(collider_pos.x - collider_width, std::fminf(ball_pos.x, collider_pos.x + collider_width));
		closest_point.y = std::fmaxf(collider_pos.y, std::fminf(ball_pos.y, collider_pos.y));
		closest_point.z = std::fmaxf(collider_pos.z, std::fminf(ball_pos.z, collider_pos.z));


		// Euclidean distance between Point - Sphere
		float distance = sqrtf(
			((closest_point.x - ball_pos.x) * (closest_point.x - ball_pos.x)) +
			((closest_point.y - ball_pos.y) * (closest_point.y - ball_pos.y)) +
			((closest_point.z - ball_pos.z) * (closest_point.z - ball_pos.z))
		);

		bool is_colliding = distance <= m_BallState.radius;
		glm::vec3 difference_vector = closest_point - ball_pos;
		VelocityDir dir = VelocityDir::DOWN;

		// Determine the direction the ball should move based on the collision
		if (is_colliding)
		{
			glm::vec3 directions[] = {
				glm::vec3(0.0f, 0.0f, -1.0f),	// down
				glm::vec3(1.0f, 0.0f, 0.0f),	// left
				glm::vec3(0.0f, 0.0f, 1.0f),	// up
				glm::vec3(-1.0f, 0.0f, 0.0f)	// right
			};

			float max = 0.0f;
			unsigned int best_match = -1;

			for (unsigned int i = 0; i < 4; i++)
			{
				float dot_product = glm::dot(glm::normalize(difference_vector), directions[i]);
				if (dot_product > max)
				{
					max = dot_product;
					best_match = i;
				}
			}

			dir = static_cast<VelocityDir>(best_match);
		}

		return { is_colliding, dir, difference_vector };
	}
}