#include "LM.h"
#include "MaterialComponent.h"
#include "Graphics/Material.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/API/Texture.h"

#include <imgui/imgui.h>

namespace Lumos
{
    MaterialComponent::MaterialComponent()
    {
        m_Name = "Material";
        m_Material = CreateRef<Material>();
    }
    
    MaterialComponent::MaterialComponent(Ref<Material>& material)
    : m_Material(material)
    {
		m_Name = "Material";
    }
    
    MaterialComponent::~MaterialComponent()
    {
    }
    
    void MaterialComponent::OnUpdateComponent(float dt)
    {
    }
    
    void MaterialComponent::OnIMGUI()
    {
		ImGui::Separator();

		if (m_Material)
		{
			m_Material->OnImGui();
		}
    }
}
