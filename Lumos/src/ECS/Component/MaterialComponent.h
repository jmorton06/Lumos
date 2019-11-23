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
		bool& GetActive() { return m_Active; }
		const bool& GetActive() const { return m_Active; }

    private:
        Ref<Material> m_Material;
		bool m_Active = true;
    };
}
