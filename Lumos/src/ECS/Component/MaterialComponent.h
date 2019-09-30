#pragma once
#include "lmpch.h"
#include "Graphics/Material.h"

namespace Lumos
{
    class LUMOS_EXPORT MaterialComponent
    {
    public:
        MaterialComponent();
        explicit MaterialComponent(Ref<Material>& material);
        virtual ~MaterialComponent();

        void OnImGui();
        
        const Ref<Material>& GetMaterial() const { return m_Material; }

    private:
        Ref<Material> m_Material;
    };
}
