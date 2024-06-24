#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "SceneManager.h"

#include "Physics/B2PhysicsEngine/B2PhysicsEngine.h"
#include "Scene.h"
#include "Core/Application.h"
#include "Scene.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Core/OS/FileSystem.h"
#include "Core/OS/FileSystem.h"
#include "Utilities/StringUtilities.h"

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
            LUMOS_LOG_INFO("[SceneManager] - Exiting scene : {0}", m_CurrentScene->GetSceneName());
            m_CurrentScene->OnCleanupScene();
        }

        m_vpAllScenes.Clear();
    }

    void SceneManager::SwitchScene()
    {
        SwitchScene((m_SceneIdx + 1) % m_vpAllScenes.Size());
    }

    void SceneManager::SwitchScene(int idx)
    {
        m_QueuedSceneIndex = idx;
        m_SwitchingScenes  = true;
    }

    void SceneManager::SwitchScene(const std::string& name)
    {
        bool found        = false;
        m_SwitchingScenes = true;
        uint32_t idx      = 0;
        for(uint32_t i = 0; !found && i < m_vpAllScenes.Size(); ++i)
        {
            if(m_vpAllScenes[i]->GetSceneName() == name)
            {
                found = true;
                idx   = i;
                break;
            }
        }

        if(found)
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
        {
            if(m_CurrentScene)
                return;

            if(m_vpAllScenes.Empty())
                m_vpAllScenes.PushBack(CreateSharedPtr<Scene>("NewScene"));

            m_QueuedSceneIndex = 0;
        }

        if(m_QueuedSceneIndex < 0 || m_QueuedSceneIndex >= static_cast<int>(m_vpAllScenes.Size()))
        {
            LUMOS_LOG_ERROR("[SceneManager] - Invalid Scene Index : {0}", m_QueuedSceneIndex);
            m_QueuedSceneIndex = 0;
        }

        auto& app = Application::Get();

        // Clear up old scene
        if(m_CurrentScene)
        {
            LUMOS_LOG_INFO("[SceneManager] - Exiting scene : {0}", m_CurrentScene->GetSceneName());
            app.GetSystem<LumosPhysicsEngine>()->SetPaused(true);

            m_CurrentScene->OnCleanupScene();
            app.OnExitScene();
        }

        m_SceneIdx     = m_QueuedSceneIndex;
        m_CurrentScene = m_vpAllScenes[m_QueuedSceneIndex].get();

        // Initialise new scene
        app.GetSystem<LumosPhysicsEngine>()->SetDefaults();
        app.GetSystem<B2PhysicsEngine>()->SetDefaults();
        app.GetSystem<LumosPhysicsEngine>()->SetPaused(false);

        std::string physicalPath;
        if(Lumos::FileSystem::Get().ResolvePhysicalPath("//Assets/Scenes/" + m_CurrentScene->GetSceneName() + ".lsn", physicalPath))
        {
            auto newPath = StringUtilities::RemoveName(physicalPath);
            m_CurrentScene->Deserialise(newPath, false);
        }

        auto screenSize = app.GetWindowSize();
        m_CurrentScene->SetScreenSize(static_cast<uint32_t>(screenSize.x), static_cast<uint32_t>(screenSize.y));

        if(app.GetEditorState() == EditorState::Play)
            m_CurrentScene->OnInit();

        Application::Get().OnNewScene(m_CurrentScene);

        LUMOS_LOG_INFO("[SceneManager] - Scene switched to : {0}", m_CurrentScene->GetSceneName().c_str());

        m_SwitchingScenes = false;
    }

    TDArray<std::string> SceneManager::GetSceneNames()
    {
        TDArray<std::string> names;

        for(auto& scene : m_vpAllScenes)
        {
            names.PushBack(scene->GetSceneName());
        }

        return names;
    }

    int SceneManager::EnqueueSceneFromFile(const std::string& filePath)
    {
        /*    auto found = std::find(m_SceneFilePaths.begin(), m_SceneFilePaths.end(), filePath);
            if(found != m_SceneFilePaths.end())
                return int(found - m_SceneFilePaths.begin());*/

        for(uint32_t i = 0; i < m_SceneFilePaths.Size(); ++i)
        {
            if(m_SceneFilePaths[i] == filePath)
            {
                return i;
            }
        }

        m_SceneFilePaths.PushBack(filePath);

        auto name  = StringUtilities::RemoveFilePathExtension(StringUtilities::GetFileName(filePath));
        auto scene = new Scene(name);
        EnqueueScene(scene);
        return int(m_vpAllScenes.Size()) - 1;
    }

    void SceneManager::EnqueueScene(Scene* scene)
    {
        m_vpAllScenes.PushBack(SharedPtr<Scene>(scene));
        LUMOS_LOG_INFO("[SceneManager] - Enqueued scene : {0}", scene->GetSceneName().c_str());
    }

    bool SceneManager::ContainsScene(const std::string& filePath)
    {
        for(uint32_t i = 0; i < m_SceneFilePaths.Size(); ++i)
        {
            if(m_SceneFilePaths[i] == filePath)
            {
                return true;
            }
        }

        return false;
    }

    void SceneManager::LoadCurrentList()
    {
        for(auto& filePath : m_SceneFilePathsToLoad)
        {
            std::string newPath;
            FileSystem::Get().AbsolutePathToFileSystem(filePath, newPath);
            EnqueueSceneFromFile(filePath);
        }

        m_SceneFilePathsToLoad.Clear();
    }

    const TDArray<std::string>& SceneManager::GetSceneFilePaths()
    {
        m_SceneFilePaths.Clear();
        for(auto scene : m_vpAllScenes)
            m_SceneFilePaths.PushBack("//Assets/Scenes/" + scene->GetSceneName());
        return m_SceneFilePaths;
    }
}
