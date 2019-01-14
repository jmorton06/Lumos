#pragma once
#include "JM.h"
#include "JMComponent.h"
#include "Maths/Vector3.h"

namespace  jm
{
    class ParticleEmitter;

    class JM_EXPORT ParticleComponent : public JMComponent
    {
    public:
        std::shared_ptr<ParticleEmitter> m_ParticleEmitter;
        maths::Vector3 m_PositionOffset;
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
    };
}
