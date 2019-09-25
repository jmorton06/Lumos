#pragma once
#include "lmpch.h"
#include "LumosComponent.h"
#include "Maths/Vector3.h"
#include "Graphics/ParticleEmitter.h"

namespace Lumos
{
    class LUMOS_EXPORT ParticleComponent : public LumosComponent
    {
    public:
        ParticleComponent();
        explicit ParticleComponent(Ref<ParticleEmitter>& emitter);

        void Init();

		void OnImGui() override;
		nlohmann::json Serialise() override { return nullptr; };
		void Deserialise(nlohmann::json& data) override {};

		SETUPCOMPOMENT(ParticleComponent);

    private:
        Ref<ParticleEmitter> m_ParticleEmitter;
        Maths::Vector3 m_PositionOffset;
    };
}
