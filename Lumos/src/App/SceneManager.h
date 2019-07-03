#pragma once

#include "LM.h"

namespace Lumos
{
	class Scene;

	class LUMOS_EXPORT SceneManager
    {
	public:
        SceneManager();
        virtual ~SceneManager();
		// Add Scene to list of possible scenes to switch to
		void EnqueueScene(Scene* scene);

		//Jump to the next scene in the list or first scene if at the end
		void SwitchScene();

		//Jump to scene index (stored in order they were originally added starting at zero)
		void SwitchScene(int idx);

		//Jump to scene name
		void SwitchScene(const std::string& name);
        
        void ApplySceneSwitch();
        
		//Get currently active scene (returns NULL if no scenes yet added)
		inline Scene* GetCurrentScene() const { return m_CurrentScene; }

		//Get currently active scene's index (return 0 if no scenes yet added)
		inline u32   GetCurrentSceneIndex() const { return m_SceneIdx; }

		//Get total number of enqueued scenes
		inline u32   SceneCount() const { return static_cast<u32>(m_vpAllScenes.size()); }

		std::vector<String> GetSceneNames();
        const std::vector<std::unique_ptr<Scene>>& GetScenes() const { return m_vpAllScenes; }
        
        void SetSwitchScene(bool switching) { m_SwitchingScenes = switching; }
        bool GetSwitchingScene() const { return m_SwitchingScenes; }

	protected:
		u32								m_SceneIdx;
		Scene*								m_CurrentScene;
		std::vector<std::unique_ptr<Scene>> m_vpAllScenes;
    private:
        bool m_SwitchingScenes = false;
        int m_QueuedSceneIndex = -1;
        SceneManager(SceneManager const&) = delete;
        SceneManager& operator=(SceneManager const&) = delete;
	};
}
