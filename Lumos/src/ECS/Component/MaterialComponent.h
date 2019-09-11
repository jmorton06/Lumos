#pragma once
#include "lmpch.h"
#include "LumosComponent.h"
#include "Graphics/Material.h"

namespace Lumos
{
    class LUMOS_EXPORT MaterialComponent : public LumosComponent
    {
    public:
        MaterialComponent();
        explicit MaterialComponent(Ref<Material>& material);
        virtual ~MaterialComponent();

        void OnImGui() override;
        
        const Ref<Material>& GetMaterial() const { return m_Material; }

		nlohmann::json Serialise() override { return nullptr; };
		void Deserialise(nlohmann::json& data) override {};
    private:
        Ref<Material> m_Material;
    };
}
