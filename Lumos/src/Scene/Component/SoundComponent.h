#pragma once

#include "Audio/SoundNode.h"

namespace Lumos
{
	class LUMOS_EXPORT SoundComponent
	{
	public:
        SoundComponent();
		explicit SoundComponent(Ref<SoundNode>& sound);

		void Init();

		void OnImGui();
        
        SoundNode* GetSoundNode() const { return m_SoundNode.get(); }

    private:
        Ref<SoundNode> m_SoundNode;
	};
}
