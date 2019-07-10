#pragma once
#include "LM.h"
#include "LumosComponent.h"
#include "Maths/Vector3.h"

namespace Lumos
{
    class ParticleEmitter;

    class LUMOS_EXPORT ParticleComponent : public LumosComponent
    {
    public:
        ParticleComponent();
        explicit ParticleComponent(std::shared_ptr<ParticleEmitter>& emitter);

        void Init() override;
		void DebugDraw(uint64 debugFlags) override;
        void OnUpdateComponent(float dt) override;

		void OnIMGUI() override;
    private:
        std::shared_ptr<ParticleEmitter> m_ParticleEmitter;
        Maths::Vector3 m_PositionOffset;
    };
}
