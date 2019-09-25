#include "lmpch.h"
#include "MeshComponent.h"
#include "Maths/BoundingSphere.h"
#include "Physics3DComponent.h"

#include <imgui/imgui.h>

namespace Lumos
{
    MeshComponent::MeshComponent()
    {
		m_BoundingShape = nullptr;
    }
    
	MeshComponent::MeshComponent(const Ref<Graphics::Mesh>& model)
		: m_Mesh(model)
	{
		m_BoundingShape = m_Mesh->GetBoundingSphere();
	}

	MeshComponent::MeshComponent(Graphics::Mesh* mesh)
	{
        m_Mesh = Lumos::Ref<Graphics::Mesh>(mesh);
		m_BoundingShape = m_Mesh->GetBoundingSphere();
	}

	void MeshComponent::OnImGui()
	{
		
	}

}
