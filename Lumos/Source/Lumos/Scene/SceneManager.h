#pragma once
#include "Core/DataStructures/TDArray.h"
#include "Core/String.h"

namespace Lumos
{
    class Scene;

    class LUMOS_EXPORT SceneManager
    {
    public:
        SceneManager();
        virtual ~SceneManager();

        // Jump to the next scene in the list or first scene if at the end
        void SwitchScene();

        // Jump to scene index (stored in order they were originally added starting at zero)
        void SwitchScene(int idx);

        // Jump to scene name
        void SwitchScene(const char* name);

        void ApplySceneSwitch();

        // Get currently active scene (returns NULL if no scenes yet added)
        inline Scene* GetCurrentScene() const
        {
            return m_CurrentScene;
        }

        // Get currently active scene's index (return 0 if no scenes yet added)
        inline uint32_t GetCurrentSceneIndex() const
        {
            return m_SceneIdx;
        }

        // Get total number of enqueued scenes
        inline uint32_t SceneCount() const
        {
            return static_cast<uint32_t>(m_vpAllScenes.Size());
        }

        TDArray<String8> GetSceneNames(Arena* arena);
        const TDArray<SharedPtr<Scene>>& GetScenes() const
        {
            return m_vpAllScenes;
        }

        void SetSwitchScene(bool switching)
        {
            m_SwitchingScenes = switching;
        }
        bool GetSwitchingScene() const
        {
            return m_SwitchingScenes;
        }

        int EnqueueSceneFromFile(const char* filePath);
        void EnqueueScene(Scene* scene);
        bool ContainsScene(const char* filePath);
        const TDArray<String8>& GetSceneFilePaths();
        void AddFileToLoadList(const char* filePath);
        void LoadCurrentList();

    protected:
        uint32_t m_SceneIdx;
        Scene* m_CurrentScene;
        TDArray<SharedPtr<Scene>> m_vpAllScenes;
        TDArray<String8> m_SceneFilePaths;
        TDArray<String8> m_SceneFilePathsToLoad;
        Arena* m_Arena;

    private:
        bool m_SwitchingScenes = false;
        int m_QueuedSceneIndex = -1;
        NONCOPYABLE(SceneManager)
    };
}
