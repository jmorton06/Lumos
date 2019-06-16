#include "LM.h"
#include "MaterialComponent.h"
#include "Graphics/Material.h"

namespace Lumos
{
    MaterialComponent::MaterialComponent(std::shared_ptr<Material>& material)
    : m_Material(material)
    {
        m_Name = "Material";
    }
    
    void MaterialComponent::OnUpdateComponent(float dt)
    {
    }
    
    void MaterialComponent::OnIMGUI()
    {
        
    }
}
