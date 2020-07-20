#pragma once
#include "lmpch.h"
#include "Scene/ISystem.h"

namespace Lumos
{
	class Camera;
	class SoundNode;

	class LUMOS_EXPORT AudioManager : public ISystem
	{
	public:
		static AudioManager* Create();

		virtual ~AudioManager() = default;
		virtual void OnInit() override = 0;
		virtual void OnUpdate(const TimeStep& dt, Scene* scene) override = 0;

		Camera* GetListener() const
		{
			return m_Listener;
		}

		void AddSoundNode(SoundNode* node)
		{
			m_SoundNodes.emplace_back(node);
		}
		void OnDebugDraw() override{};

		void ClearNodes()
		{
			m_SoundNodes.clear();
		}

	protected:
		Camera* m_Listener;
		std::vector<SoundNode*> m_SoundNodes;
	};
}
