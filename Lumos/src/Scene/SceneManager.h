#pragma once

namespace Lumos
{
	class Scene;

	class LUMOS_EXPORT SceneManager
	{
	public:
		SceneManager();
		virtual ~SceneManager();

		//Jump to the next scene in the list or first scene if at the end
		void SwitchScene();

		//Jump to scene index (stored in order they were originally added starting at zero)
		void SwitchScene(int idx);

		//Jump to scene name
		void SwitchScene(const std::string& name);

		void ApplySceneSwitch();

		//Get currently active scene (returns NULL if no scenes yet added)
		_FORCE_INLINE_ Scene* GetCurrentScene() const
		{
			return m_CurrentScene;
		}

		//Get currently active scene's index (return 0 if no scenes yet added)
		_FORCE_INLINE_ u32 GetCurrentSceneIndex() const
		{
			return m_SceneIdx;
		}

		//Get total number of enqueued scenes
		_FORCE_INLINE_ u32 SceneCount() const
		{
			return static_cast<u32>(m_vpAllScenes.size());
		}

		std::vector<std::string> GetSceneNames();
		const std::vector<Ref<Scene>>& GetScenes() const
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

		void EnqueueSceneFromFile(const std::string& filePath);
		void EnqueueScene(Scene* scene);

		template<class T>
		void EnqueueScene(const std::string& name)
		{
			//T* scene = new T(name);
			m_vpAllScenes.emplace_back(CreateRef<T>(name));
			LUMOS_LOG_INFO("[SceneManager] - Enqueued scene : {0}", name.c_str());
		}
		
		const std::vector<std::string>& GetSceneFilePaths() const 
		{
			return m_SceneFilePaths;
		}

	protected:
		u32 m_SceneIdx;
		Scene* m_CurrentScene;
		std::vector<Ref<Scene>> m_vpAllScenes;
		std::vector<std::string> m_SceneFilePaths;
	private:
		bool m_SwitchingScenes = false;
		int m_QueuedSceneIndex = -1;
		NONCOPYABLE(SceneManager)
	};
}
