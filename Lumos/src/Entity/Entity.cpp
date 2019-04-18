#include "LM.h"
#include "Entity.h"

#include "Graphics/Renderers/DebugRenderer.h"
#include "App/Scene.h"
#include "Graphics/API/Context.h"

#include <imgui/imgui.h>
#include <imgui/plugins/ImGuizmo.h>
#include "Graphics/Camera/Camera.h"

namespace Lumos
{
	Entity::Entity(Scene* scene) : m_Name("Unnamed"), m_pScene(scene), m_pParent(nullptr), m_BoundingRadius(1), m_FrustumCullFlags(0), m_UpdateTransforms(false)
	{
        Init();
	}

	Entity::Entity(const String& name,Scene* scene) : m_Name(name), m_pScene(scene), m_pParent(nullptr), m_BoundingRadius(1),
	                                     m_FrustumCullFlags(0), m_UpdateTransforms(false)
	{
        Init();
	}

	Entity::~Entity()
	{
	}
    
    void Entity::Init()
    {
    }

	void Entity::AddComponent(std::unique_ptr<LumosComponent> component)
	{
        component->SetEntity(this);
		component->Init();
		m_Components[component->GetType()] = std::move(component);
	}

	void Entity::OnRenderObject()
	{
		for(const auto& component : m_Components)
		{
			component.second->OnRenderComponent();
		}
	}

	void Entity::OnUpdateObject(float dt)
	{
		for (const auto& component : m_Components)
		{
			component.second->OnUpdateComponent(dt);

			if (m_UpdateTransforms)
				component.second->OnUpdateTransform(maths::Matrix4());
		}

		m_UpdateTransforms = false;
	}

	void Entity::AddChildObject(Entity* child)
	{
		m_vpChildren.push_back(child);
		child->m_pParent = this;
		child->m_pScene = this->m_pScene;
	}
	void Entity::DebugDraw(uint64 debugFlags)
	{
		if (debugFlags & DEBUGDRAW_FLAGS_BOUNDING_RADIUS)
		{
			maths::Vector4 boundRadiusCol(0.3f, 0.6f, 0.4f, 0.8f);
			boundRadiusCol.SetW(0.2f);
			if (GetComponent<TransformComponent>())
				DebugRenderer::DrawPointNDT(GetComponent<TransformComponent>()->m_Transform.GetWorldMatrix().GetPositionVector(), m_BoundingRadius, boundRadiusCol);
		}
		
		for(auto& component: m_Components)
		{
			component.second->DebugDraw(debugFlags);
		}
	}

	TransformComponent* Entity::GetTransform()
	{
		auto transform = m_DefaultTransformComponent;

		if (!transform)
		{
			transform = GetComponent<TransformComponent>();

			if(!transform)
				AddComponent(std::make_unique<TransformComponent>(maths::Matrix4()));

			if (!m_DefaultTransformComponent)
				m_DefaultTransformComponent = GetComponent<TransformComponent>();
		}

		return m_DefaultTransformComponent;
	}

	void Entity::OnGuizmo(uint mode)
	{
		maths::Matrix4 view = m_pScene->GetCamera()->GetViewMatrix();
		maths::Matrix4 proj = m_pScene->GetCamera()->GetProjectionMatrix();
        
#ifdef LUMOS_RENDER_API_VULKAN
		if(graphics::Context::GetRenderAPI() == RenderAPI::VULKAN)
			proj[5] *= -1.0f;
#endif
		ImGuizmo::SetDrawlist();
        
        auto pos = ImGui::GetWindowPos();
        auto size = ImGui::GetWindowSize();
        ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y);

		maths::Matrix4 model = maths::Matrix4();
		if (this->GetComponent<TransformComponent>() != nullptr)
			model = GetComponent<TransformComponent>()->m_Transform.GetWorldMatrix();
        
		ImGuizmo::Manipulate(view.values, proj.values, static_cast<ImGuizmo::OPERATION>(mode), ImGuizmo::WORLD, model.values, nullptr, nullptr);

		if (this->GetComponent<TransformComponent>() != nullptr)
			GetComponent<TransformComponent>()->SetWorldMatrix(model);
	}
    
    void Entity::OnIMGUI()
    {
		static char objName[INPUT_BUF_SIZE];
		strcpy(objName, m_Name.c_str());

		ImGuiInputTextFlags inputFlag = ImGuiInputTextFlags_EnterReturnsTrue;
		if (ImGui::InputText("Name", objName, IM_ARRAYSIZE(objName), inputFlag))
			m_Name = objName;

        for(auto& component: m_Components)
        {
            component.second->OnIMGUI();
        }
    }
}
