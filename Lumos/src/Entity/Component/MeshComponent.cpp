#include "LM.h"
#include "MeshComponent.h"
#include "Graphics/Mesh.h"
#include "Maths/BoundingSphere.h"
#include "Physics3DComponent.h"

#include <imgui/imgui.h>

namespace Lumos
{
	MeshComponent::MeshComponent(std::shared_ptr<Graphics::Mesh>& model)
		: m_Model(model)
	{
		m_Name = "Mesh";
		m_BoundingShape = std::make_unique<Maths::BoundingSphere>(Maths::Vector3(0.0f),1.0f);
	}

	MeshComponent::MeshComponent(Graphics::Mesh* mesh)
	{
		m_Model = std::shared_ptr<Graphics::Mesh>(mesh);
		m_BoundingShape = std::make_unique<Maths::BoundingSphere>(Maths::Vector3(0.0f), 1.0f);
	}

	void MeshComponent::OnUpdateComponent(float dt)
	{
	}

	void MeshComponent::OnIMGUI()
	{
		
	}

}
