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
        std::shared_ptr<ParticleEmitter> m_ParticleEmitter;
        Maths::Vector3 m_PositionOffset;
    public:
        explicit ParticleComponent(std::shared_ptr<ParticleEmitter>& emitter);

        static ComponentType GetStaticType()
        {
            static ComponentType type(ComponentType::Particle);
            return type;
        }

        void Init() override;
		void DebugDraw(uint64 debugFlags) override;
        void OnUpdateComponent(float dt) override;

        inline virtual ComponentType GetType() const override { return GetStaticType(); }

		void OnIMGUI() override;
    };
}
