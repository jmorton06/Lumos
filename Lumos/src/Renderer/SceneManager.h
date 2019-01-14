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
		void JumpToScene();

		//Jump to scene index (stored in order they were originally added starting at zero)
		void JumpToScene(int idx);

		//Jump to scene name
		void JumpToScene(const std::string& friendly_name);

		//Get currently active scene (returns NULL if no scenes yet added)
		inline Scene* GetCurrentScene() const { return m_CurrentScene; }

		//Get currently active scene's index (return 0 if no scenes yet added)
		inline uint   GetCurrentSceneIndex() const { return m_SceneIdx; }

		//Get total number of enqueued scenes
		inline uint   SceneCount() const { return static_cast<uint>(m_vpAllScenes.size()); }

		std::vector<String> GetSceneNames();

	protected:
		uint								m_SceneIdx;
		Scene*								m_CurrentScene;
		std::vector<std::unique_ptr<Scene>> m_vpAllScenes;
        
    private:
        SceneManager(SceneManager const&) = delete;
        SceneManager& operator=(SceneManager const&) = delete;
	};
}
