#include "LM.h"

namespace Lumos
{
	class Camera;
	class SoundNode;

    class LUMOS_EXPORT AudioManager
    {
    public:
        static AudioManager* Create();

        virtual ~AudioManager() = default;
        virtual void OnInit() = 0;
        virtual void OnUpdate() = 0;

		virtual void SetListener(Camera* camera) { m_Listener = camera; }
		Camera* GetListener() const { return m_Listener; }

		void AddSoundNode(SoundNode* node) { m_SoundNodes.emplace_back(node); }

		void ClearNodes() { m_SoundNodes.clear(); }
	protected:
		Camera*	m_Listener;
		std::vector<SoundNode*> m_SoundNodes;
	};
}