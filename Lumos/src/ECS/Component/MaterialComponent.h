#pragma once
#include "LM.h"
#include "LumosComponent.h"

namespace Lumos
{
    class Material;
    
    class LUMOS_EXPORT MaterialComponent : public LumosComponent
    {
    public:
        MaterialComponent();
        explicit MaterialComponent(Ref<Material>& material);
        virtual ~MaterialComponent();
        
        void OnUpdateComponent(float dt) override;
        
        void OnIMGUI() override;
        
        const Ref<Material>& GetMaterial() const { return m_Material; }

		nlohmann::json Serialise() override { return nullptr; };
		void Deserialise(nlohmann::json& data) override {};
    private:
        Ref<Material> m_Material;
    };
}
