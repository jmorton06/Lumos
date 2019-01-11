#include "JM.h"
#include "SceneManager.h"

#include "Physics/B2PhysicsEngine/B2PhysicsEngine.h"
#include "Scene.h"
#include "App/Application.h"
#include "GraphicsPipeline.h"
#include "Scene.h"

namespace jm
{

	SceneManager::SceneManager()
		: m_SceneIdx(0), m_CurrentScene(nullptr)
	{
	}

	SceneManager::~SceneManager()
	{
		m_SceneIdx = 0;
		m_vpAllScenes.clear();
	}

	void SceneManager::EnqueueScene(Scene* scene)
	{
		if (scene == nullptr)
		{
			JM_CORE_ERROR("Attempting to enqueue nullptr scene", "");
			return;
		}

		JM_CORE_INFO("[SceneManager] - Enqueued scene: ", scene->GetSceneName().c_str());
		m_vpAllScenes.push_back(std::unique_ptr<Scene>(scene));

		scene->SetScreenWidth(Application::Instance()->GetWindow()->GetWidth());
		scene->SetScreenHeight(Application::Instance()->GetWindow()->GetHeight());

		//If this was the first scene, activate it immediately
		// if (m_vpAllScenes.size() == 1)
		// 	JumpToScene(0);
	}

	void SceneManager::JumpToScene()
	{
		JumpToScene((m_SceneIdx + 1) % m_vpAllScenes.size());
	}

	void SceneManager::JumpToScene(int idx)
	{
		if (idx < 0 || idx >= static_cast<int>(m_vpAllScenes.size()))
		{
			JM_CORE_ERROR("Invalid Scene Index: ", idx);
			return;
		}

		//Clear up old scene
		if (m_CurrentScene)
		{
			JM_CORE_INFO("[SceneManager] - Exiting scene - " , m_CurrentScene->GetSceneName());
			JMPhysicsEngine::Instance()->RemoveAllPhysicsObjects();
            JMPhysicsEngine::Instance()->SetPaused(true);
			m_CurrentScene->OnCleanupScene();
		}

		m_SceneIdx = idx;
		m_CurrentScene = m_vpAllScenes[idx].get();

		//Initialize new scene
		JMPhysicsEngine::Instance()->SetDefaults();
		B2PhysicsEngine::Instance()->SetDefaults();

        Application::Instance()->GetGraphicsPipeline()->SetScene(m_CurrentScene);
        Application::Instance()->GetGraphicsPipeline()->Reset();
		m_CurrentScene->OnInit();

		JM_CORE_INFO("[SceneManager] - Scene switched to: ", m_CurrentScene->GetSceneName().c_str());
	}

	void SceneManager::JumpToScene(const std::string& friendly_name)
	{
		bool found = false;
		uint idx = 0;
		for (uint i = 0; !found && i < m_vpAllScenes.size(); ++i)
		{
			if (m_vpAllScenes[i]->GetSceneName() == friendly_name)
			{
				found = true;
				idx = i;
				break;
			}
		}

		if (found)
		{
			JumpToScene(idx);
		}
		else
		{
			JM_CORE_ERROR("Unknown Scene Alias: ", friendly_name.c_str());
		}
	}

	std::vector<String> SceneManager::GetSceneNames()
	{
		std::vector<String> names;

		for(auto& scene : m_vpAllScenes)
		{
			names.push_back(scene->GetSceneName());
		}

		return names;
	}
}
