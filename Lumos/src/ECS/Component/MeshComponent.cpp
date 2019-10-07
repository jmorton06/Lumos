#include "lmpch.h"
#include "MeshComponent.h"
#include "Maths/BoundingSphere.h"
#include "Physics3DComponent.h"

#include <imgui/imgui.h>

namespace Lumos
{
    MeshComponent::MeshComponent()
    {
    }
    
	MeshComponent::MeshComponent(const Ref<Graphics::Mesh>& model)
		: m_Mesh(model)
	{
	}

	MeshComponent::MeshComponent(Graphics::Mesh* mesh)
	{
        m_Mesh = Lumos::Ref<Graphics::Mesh>(mesh);
	}

	void MeshComponent::OnImGui()
	{
		
	}

}
