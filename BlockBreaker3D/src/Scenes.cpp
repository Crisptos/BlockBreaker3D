#include "Engine.h"

namespace BB3D
{

	// Implementation of all scenes

	// ________________________________ GameScene ________________________________
	GameScene::GameScene()
	{
		m_GameEntities.reserve(16);
		m_GameElements.reserve(16);
		m_GameTextfields.reserve(16);

		// Initialize game scene entities
		// TODO Possibly load this as serialized JSON data
		m_GameEntities.push_back({ MeshType::SPHERE, TextureType::GEM10, glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, -4.0f), glm::vec3(0.0f), glm::vec3(1.0f), glm::vec3(0.0f) });
		m_GameEntities.push_back({ MeshType::QUAD, TextureType::GEM10, glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f), glm::vec3(0.0f), glm::vec3(5.0f, 1.0f, 5.0f), glm::vec3(0.0f) });

	}

	GameScene::~GameScene()
	{

	}

	// Main gameplay loop logic
	void GameScene::Update(InputState& input_state)
	{

	}

	std::vector<Entity>& GameScene::GetSceneEntities()
	{
		return m_GameEntities;
	}

	const std::vector<UI_Element>& GameScene::GetSceneUIElems() const
	{
		return m_GameElements;
	}

	const std::vector<UI_TextField>& GameScene::GetSceneUITextFields() const
	{
		return m_GameTextfields;
	}

}