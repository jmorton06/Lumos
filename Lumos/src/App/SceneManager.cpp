#include "lmpch.h"
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

		if (m_CurrentScene)
		{
			LUMOS_LOG_INFO("[SceneManager] - Exiting scene : {0}", m_CurrentScene->GetSceneName());
			m_CurrentScene->OnCleanupScene();
		}

		m_vpAllScenes.clear();
	}

	void SceneManager::SwitchScene()
	{
		SwitchScene((m_SceneIdx + 1) % m_vpAllScenes.size());
	}

	void SceneManager::SwitchScene(int idx)
	{
        m_QueuedSceneIndex = idx;
        m_SwitchingScenes = true;
	}

	void SceneManager::SwitchScene(const std::string& name)
	{
		bool found = false;
        m_SwitchingScenes = true;
		u32 idx = 0;
		for (u32 i = 0; !found && i < m_vpAllScenes.size(); ++i)
		{
			if (m_vpAllScenes[i]->GetSceneName() == name)
			{
				found = true;
				idx = i;
				break;
			}
		}

		if (found)
		{
			SwitchScene(idx);
		}
		else
		{
			LUMOS_LOG_ERROR("[SceneManager] - Unknown Scene Alias : {0}", name.c_str());
		}
	}
    
    void SceneManager::ApplySceneSwitch()
    {
        if(m_SwitchingScenes == false)
            return;
        
        if (m_QueuedSceneIndex < 0 || m_QueuedSceneIndex >= static_cast<int>(m_vpAllScenes.size()))
        {
            LUMOS_LOG_ERROR("[SceneManager] - Invalid Scene Index : {0}", m_QueuedSceneIndex);
            return;
        }
        
        //Clear up old scene
        if (m_CurrentScene)
        {
            LUMOS_LOG_INFO("[SceneManager] - Exiting scene : {0}" , m_CurrentScene->GetSceneName());
            Application::Instance()->GetSystem<LumosPhysicsEngine>()->SetPaused(true);
            m_CurrentScene->OnCleanupScene();
			Application::Instance()->OnExitScene();
        }
        
        m_SceneIdx = m_QueuedSceneIndex;
        m_CurrentScene = m_vpAllScenes[m_QueuedSceneIndex].get();
        
        //Initialize new scene
        Application::Instance()->GetSystem<LumosPhysicsEngine>()->SetDefaults();
        Application::Instance()->GetSystem<B2PhysicsEngine>()->SetDefaults();
        
        auto screenSize = Application::Instance()->GetWindowSize();
        m_CurrentScene->SetScreenWidth(static_cast<u32>(screenSize.x));
        m_CurrentScene->SetScreenHeight(static_cast<u32>(screenSize.y));
        m_CurrentScene->OnInit();
        
        Application::Instance()->OnNewScene(m_CurrentScene);
        
        LUMOS_LOG_INFO("[SceneManager] - Scene switched to : {0}", m_CurrentScene->GetSceneName().c_str());
        
        m_SwitchingScenes = false;
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
