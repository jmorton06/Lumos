#include "LM.h"
#include "MeshComponent.h"
#include "Maths/BoundingSphere.h"
#include "Physics3DComponent.h"

#include <imgui/imgui.h>

namespace Lumos
{
    MeshComponent::MeshComponent()
    {
        m_Name = "Mesh";
        m_BoundingShape = CreateScope<Maths::BoundingSphere>(Maths::Vector3(0.0f),1.0f);
    }
    
	MeshComponent::MeshComponent(const Ref<Graphics::Mesh>& model)
		: m_Mesh(model)
	{
		m_Name = "Mesh";
		m_BoundingShape = CreateScope<Maths::BoundingSphere>(Maths::Vector3(0.0f),1.0f);
	}

	MeshComponent::MeshComponent(Graphics::Mesh* mesh)
	{
        m_Mesh = Lumos::Ref<Graphics::Mesh>(mesh);
		m_BoundingShape = CreateScope<Maths::BoundingSphere>(Maths::Vector3(0.0f), 1.0f);
	}

	void MeshComponent::OnImGui()
	{
		
	}

}
