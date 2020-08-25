#include "lmpch.h"
#include "SceneManager.h"

#include "Physics/B2PhysicsEngine/B2PhysicsEngine.h"
#include "Scene.h"
#include "Core/Application.h"
#include "Scene.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Core/OS/FileSystem.h"

namespace Lumos
{

	SceneManager::SceneManager()
		: m_SceneIdx(0)
		, m_CurrentScene(nullptr)
	{
	}

	SceneManager::~SceneManager()
	{
		m_SceneIdx = 0;

		if(m_CurrentScene)
		{
			Debug::Log::Info("[SceneManager] - Exiting scene : {0}", m_CurrentScene->GetSceneName());
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
		for(u32 i = 0; !found && i < m_vpAllScenes.size(); ++i)
		{
			if(m_vpAllScenes[i]->GetSceneName() == name)
			{
				found = true;
				idx = i;
				break;
			}
		}

		if(found)
		{
			SwitchScene(idx);
		}
		else
		{
			Debug::Log::Error("[SceneManager] - Unknown Scene Alias : {0}", name.c_str());
		}
	}

	void SceneManager::ApplySceneSwitch()
	{
		if(m_SwitchingScenes == false)
        {
            if(m_CurrentScene)
                return;
            
            if(m_vpAllScenes.empty())
                m_vpAllScenes.push_back(CreateRef<Scene>("NewScene"));
            
            m_QueuedSceneIndex = 0;
        }

		if(m_QueuedSceneIndex < 0 || m_QueuedSceneIndex >= static_cast<int>(m_vpAllScenes.size()))
		{
			Debug::Log::Error("[SceneManager] - Invalid Scene Index : {0}", m_QueuedSceneIndex);
			return;
		}

		auto& app = Application::Get();

		//Clear up old scene
		if(m_CurrentScene)
		{
			Debug::Log::Info("[SceneManager] - Exiting scene : {0}", m_CurrentScene->GetSceneName());
			app.GetSystem<LumosPhysicsEngine>()->SetPaused(true);
            app.GetSystem<LumosPhysicsEngine>()->ClearConstraints();

			m_CurrentScene->OnCleanupScene();
			app.OnExitScene();
		}

		m_SceneIdx = m_QueuedSceneIndex;
		m_CurrentScene = m_vpAllScenes[m_QueuedSceneIndex].get();

		//Initialize new scene
		app.GetSystem<LumosPhysicsEngine>()->SetDefaults();
		app.GetSystem<B2PhysicsEngine>()->SetDefaults();

		auto screenSize = app.GetWindowSize();
		m_CurrentScene->SetScreenWidth(static_cast<u32>(screenSize.x));
		m_CurrentScene->SetScreenHeight(static_cast<u32>(screenSize.y));
        
        if(FileSystem::FileExists(ROOT_DIR "/Sandbox/res/scenes/" + m_CurrentScene->GetSceneName() + ".lsn"))
        {
            m_CurrentScene->Deserialise(ROOT_DIR "/Sandbox/res/scenes/", false);
        }
        
	#ifdef LUMOS_EDITOR
        if(app.GetEditorState() == EditorState::Play)
	#endif
            m_CurrentScene->OnInit();

		Application::Get().OnNewScene(m_CurrentScene);

		Debug::Log::Info("[SceneManager] - Scene switched to : {0}", m_CurrentScene->GetSceneName().c_str());

		m_SwitchingScenes = false;
	}

	std::vector<std::string> SceneManager::GetSceneNames()
	{
		std::vector<std::string> names;

		for(auto& scene : m_vpAllScenes)
		{
			names.push_back(scene->GetSceneName());
		}

		return names;
	}

	void SceneManager::EnqueueSceneFromFile(const std::string& filePath)
	{
		Scene* scene = new Scene("scene");
		scene->Deserialise(filePath, false);
		EnqueueScene(scene);
	}

	void SceneManager::EnqueueScene(Scene* scene)
	{
		m_vpAllScenes.push_back(Ref<Scene>(scene));
		LUMOS_LOG_INFO("[SceneManager] - Enqueued scene : {0}", scene->GetSceneName().c_str());
	}
}
