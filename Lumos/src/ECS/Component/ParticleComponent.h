#pragma once
#include "lmpch.h"
#include "Maths/Vector3.h"
#include "Graphics/ParticleEmitter.h"

namespace Lumos
{
    class LUMOS_EXPORT ParticleComponent
    {
    public:
        ParticleComponent();
        explicit ParticleComponent(Ref<ParticleEmitter>& emitter);

        void Init();

		void OnImGui();

    private:
        Ref<ParticleEmitter> m_ParticleEmitter;
        Maths::Vector3 m_PositionOffset;
    };
}
