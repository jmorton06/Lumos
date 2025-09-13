#include "Precompiled.h"
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
        m_Arena = ArenaAlloc(Kilobytes(64));
    }

    SceneManager::~SceneManager()
    {
        m_SceneIdx = 0;

        if(m_CurrentScene)
        {
            LINFO("[SceneManager] - Exiting scene : %s", m_CurrentScene->GetSceneName().c_str());
            m_CurrentScene->OnCleanupScene();
        }

        m_vpAllScenes.Clear();

        ArenaRelease(m_Arena);
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

    void SceneManager::SwitchScene(const char* name)
    {
        bool found        = false;
        m_SwitchingScenes = true;
        uint32_t idx      = 0;
        for(uint32_t i = 0; !found && i < m_vpAllScenes.Size(); ++i)
        {
            if(Str8StdS(m_vpAllScenes[i]->GetSceneName()) == Str8C((char*)name))
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
            LERROR("[SceneManager] - Unknown Scene Alias : %s", name);
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
            LERROR("[SceneManager] - Invalid Scene Index : %i", m_QueuedSceneIndex);
            m_QueuedSceneIndex = 0;
        }

        auto& app = Application::Get();

        // Clear up old scene
        if(m_CurrentScene)
        {
            LINFO("[SceneManager] - Exiting scene : %s", m_CurrentScene->GetSceneName().c_str());
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

        String8 physicalPath;
        std::string path = "//Assets/Scenes/" + m_CurrentScene->GetSceneName() + ".lsn";
        if(Lumos::FileSystem::Get().ResolvePhysicalPath(Application::Get().GetFrameArena(), Str8StdS(path), &physicalPath))
        {
            auto newPath = StringUtilities::RemoveName(ToStdString(physicalPath));
            m_CurrentScene->Deserialise(newPath, false);
        }

        auto screenSize = app.GetWindowSize();
        m_CurrentScene->SetScreenSize(static_cast<uint32_t>(screenSize.x), static_cast<uint32_t>(screenSize.y));

        if(app.GetEditorState() == EditorState::Play)
            m_CurrentScene->OnInit();

        Application::Get().OnNewScene(m_CurrentScene);

        LINFO("[SceneManager] - Scene switched to : %s", m_CurrentScene->GetSceneName().c_str());

        m_SwitchingScenes = false;
    }

    TDArray<String8> SceneManager::GetSceneNames(Arena* arena)
    {
        TDArray<String8> names;

        for(auto& scene : m_vpAllScenes)
        {
            names.PushBack(PushStr8Copy(arena, scene->GetSceneName().c_str()));
        }

        return names;
    }

    int SceneManager::EnqueueSceneFromFile(const char* filePath)
    {
        /*    auto found = std::find(m_SceneFilePaths.begin(), m_SceneFilePaths.end(), filePath);
            if(found != m_SceneFilePaths.end())
                return int(found - m_SceneFilePaths.begin());*/

        for(uint32_t i = 0; i < m_SceneFilePaths.Size(); ++i)
        {
            if(m_SceneFilePaths[i] == Str8C((char*)filePath))
            {
                return i;
            }
        }

        m_SceneFilePaths.PushBack(PushStr8Copy(m_Arena, filePath));

        auto name  = StringUtilities::RemoveFilePathExtension(StringUtilities::GetFileName(filePath));
        auto scene = new Scene(name);
        EnqueueScene(scene);
        return int(m_vpAllScenes.Size()) - 1;
    }

    void SceneManager::EnqueueScene(Scene* scene)
    {
        m_vpAllScenes.PushBack(SharedPtr<Scene>(scene));
        LINFO("[SceneManager] - Enqueued scene : %s", scene->GetSceneName().c_str());
    }

    bool SceneManager::ContainsScene(const char* filePath)
    {
        for(uint32_t i = 0; i < m_SceneFilePaths.Size(); ++i)
        {
            if(m_SceneFilePaths[i] == Str8C((char*)filePath))
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
            String8 newPath;
            FileSystem::Get().AbsolutePathToFileSystem(Application::Get().GetFrameArena(), filePath, newPath);
            EnqueueSceneFromFile((const char*)newPath.str);
        }

        m_SceneFilePathsToLoad.Clear();
    }

    const TDArray<String8>& SceneManager::GetSceneFilePaths()
    {
        m_SceneFilePaths.Clear();
        for(Scene* scene : m_vpAllScenes)
            m_SceneFilePaths.PushBack(PushStr8F(m_Arena, "//Assets/Scenes/%s", scene->GetSceneName().c_str()));
        return m_SceneFilePaths;
    }

    void SceneManager::AddFileToLoadList(const char* filePath)
    {
        m_SceneFilePathsToLoad.PushBack(PushStr8Copy(m_Arena, filePath));
    }

}
