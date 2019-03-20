#include "LM.h"
#include "SceneManager.h"

#include "Physics/B2PhysicsEngine/B2PhysicsEngine.h"
#include "Scene.h"
#include "App/Application.h"
#include "Scene.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"

namespace Lumos
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
			LUMOS_CORE_ERROR("Attempting to enqueue nullptr scene", "");
			return;
		}

        LUMOS_CORE_INFO("[SceneManager] - Enqueued scene : {0}", scene->GetSceneName().c_str());
		m_vpAllScenes.push_back(std::unique_ptr<Scene>(scene));

		auto screenSize = Application::Instance()->GetWindowSize();
		scene->SetScreenWidth(static_cast<uint>(screenSize.GetX()));
		scene->SetScreenHeight(static_cast<uint>(screenSize.GetY()));

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
			LUMOS_CORE_ERROR("[SceneManager] - Invalid Scene Index : {0}", idx);
			return;
		}

		//Clear up old scene
		if (m_CurrentScene)
		{
            LUMOS_CORE_INFO("[SceneManager] - Exiting scene : {0}" , m_CurrentScene->GetSceneName());
			LumosPhysicsEngine::Instance()->RemoveAllPhysicsObjects();
            LumosPhysicsEngine::Instance()->SetPaused(true);
			m_CurrentScene->OnCleanupScene();
		}

		m_SceneIdx = idx;
		m_CurrentScene = m_vpAllScenes[idx].get();

		//Initialize new scene
		LumosPhysicsEngine::Instance()->SetDefaults();
		B2PhysicsEngine::Instance()->SetDefaults();

		auto screenSize = Application::Instance()->GetWindowSize();
		m_CurrentScene->SetScreenWidth(static_cast<uint>(screenSize.GetX()));
		m_CurrentScene->SetScreenHeight(static_cast<uint>(screenSize.GetY()));
		m_CurrentScene->OnInit();

		LUMOS_CORE_INFO("[SceneManager] - Scene switched to : {0}", m_CurrentScene->GetSceneName().c_str());
	}

	void SceneManager::JumpToScene(const std::string& friendly_name)
	{
		bool found = false;
        m_SwitchingScenes = true;
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
			LUMOS_CORE_ERROR("[SceneManager] - Unknown Scene Alias : {0}", friendly_name.c_str());
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
