#include "LM.h"
#include "Entity.h"

#include "Graphics/Renderers/DebugRenderer.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/Camera/Camera.h"
#include "App/Application.h"
#include "App/Scene.h"
#include "App/SceneManager.h"
#include "Maths/MathsUtilities.h"

#include <imgui/imgui.h>
#include <imgui/plugins/ImGuizmo.h>

namespace Lumos
{
	Entity::Entity(const String& name) : m_Name(name),m_pParent(nullptr), m_BoundingRadius(1),
	                                     m_FrustumCullFlags(0), m_Active(true)
	{
        Init();
	}

	Entity::~Entity()
	{
	}
    
    void Entity::Init()
    {
        m_UUID = Maths::GenerateUUID();
    }

	void Entity::AddComponent(std::unique_ptr<LumosComponent> component)
	{
        component->SetEntity(this);
		component->Init();
        
        if(component->GetType() == ComponentType::Transform)
            m_DefaultTransformComponent = reinterpret_cast<TransformComponent*>(component.get());
        
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
        }
        
        if(m_DefaultTransformComponent && m_DefaultTransformComponent->GetTransform().HasUpdated())
        {
            if(!m_pParent)
                m_DefaultTransformComponent->SetWorldMatrix(Maths::Matrix4());
			else
			{
				m_DefaultTransformComponent->SetWorldMatrix(m_pParent->GetTransformComponent()->GetTransform().GetWorldMatrix());
			}
				
            
            for(auto child : m_vpChildren)
            {
                if(child && child->GetTransformComponent())
                    child->GetTransformComponent()->SetWorldMatrix(m_DefaultTransformComponent->GetTransform().GetWorldMatrix());
            }
            
            for (const auto& component : m_Components)
            {
                component.second->OnUpdateTransform(m_DefaultTransformComponent->GetTransform().GetWorldMatrix());
            }
            
            m_DefaultTransformComponent->GetTransform().SetHasUpdated(false);
        }
	}

	void Entity::AddChildObject(std::shared_ptr<Entity>& child)
	{
		m_vpChildren.push_back(child);
		child->m_pParent = this;
	}

	void Entity::DebugDraw(uint64 debugFlags)
	{
		if (debugFlags & DEBUGDRAW_FLAGS_BOUNDING_RADIUS)
		{
			Maths::Vector4 boundRadiusCol(0.3f, 0.6f, 0.4f, 0.8f);
			boundRadiusCol.SetW(0.2f);
			if (GetComponent<TransformComponent>())
				DebugRenderer::DrawPointNDT(GetComponent<TransformComponent>()->GetTransform().GetWorldMatrix().GetPositionVector(), m_BoundingRadius, boundRadiusCol);
		}
		
		for(auto& component: m_Components)
		{
			component.second->DebugDraw(debugFlags);
		}
	}

	TransformComponent* Entity::GetTransformComponent()
	{
		if (!m_DefaultTransformComponent)
		{
			m_DefaultTransformComponent = GetComponent<TransformComponent>();

			if(!m_DefaultTransformComponent)
				AddComponent<TransformComponent>();
		}

		return m_DefaultTransformComponent;
	}

	void Entity::OnGuizmo(uint mode)
	{
		Maths::Matrix4 view = Application::Instance()->GetSceneManager()->GetCurrentScene()->GetCamera()->GetViewMatrix();
		Maths::Matrix4 proj = Application::Instance()->GetSceneManager()->GetCurrentScene()->GetCamera()->GetProjectionMatrix();
        
#ifdef LUMOS_RENDER_API_VULKAN
		if(Graphics::GraphicsContext::GetRenderAPI() == Graphics::RenderAPI::VULKAN)
			proj[5] *= -1.0f;
#endif
		ImGuizmo::SetDrawlist();
        
        auto pos = ImGui::GetWindowPos();
        auto size = ImGui::GetWindowSize();
        ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y);

		Maths::Matrix4 model = Maths::Matrix4();
		if (this->GetComponent<TransformComponent>() != nullptr)
			model = GetComponent<TransformComponent>()->GetTransform().GetWorldMatrix();

        float delta[16];
        ImGuizmo::Manipulate(view.values, proj.values, static_cast<ImGuizmo::OPERATION>(mode),ImGuizmo::LOCAL, model.values, delta, nullptr);

		if (GetTransformComponent() != nullptr && ImGuizmo::IsUsing())
        {
            auto mat = Maths::Matrix4(delta) * m_DefaultTransformComponent->GetTransform().GetLocalMatrix();
            m_DefaultTransformComponent->GetTransform().SetLocalTransform(mat);
            m_DefaultTransformComponent->GetTransform().ApplyTransform();
            
        }
	}
    
    void Entity::OnIMGUI()
    {
		static char objName[INPUT_BUF_SIZE];
		strcpy(objName, m_Name.c_str());

        ImGuiInputTextFlags inputFlag = ImGuiInputTextFlags_EnterReturnsTrue;
        
        ImGui::Checkbox("##Active", &m_Active);
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("##Name", objName, IM_ARRAYSIZE(objName), inputFlag))
            m_Name = objName;
        
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2,2));
        ImGui::Columns(2);
        ImGui::Separator();
        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", "Parent");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::Text("%s", m_pParent ? m_pParent->GetName().c_str() : "No Parent");
        ImGui::PopItemWidth();
        ImGui::NextColumn();
        
        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
        
        for(auto& component: m_Components)
        {
			ImGui::Separator();
			if(ImGui::TreeNode(component.second->GetName().c_str()))
			{
				if (component.second->GetCanDisable())
				{
					ImGui::Checkbox("Active", &component.second->GetActive());
				}
				component.second->OnIMGUI();

				ImGui::TreePop();
			}
        }
    }
    
    void Entity::SetParent(Entity *parent)
    {
        m_pParent = parent;
        m_DefaultTransformComponent->SetWorldMatrix(m_pParent->GetTransformComponent()->GetTransform().GetWorldMatrix());
    }

	const bool Entity::ActiveInHierarchy() const
	{
		if (!Active())
			return false;

		if (m_pParent)
			return m_pParent->ActiveInHierarchy();
		else
			return true;
	}

	void Entity::SetActiveRecursive(bool active)
	{
		m_Active = active;

		for (auto child : m_vpChildren)
		{
			child->SetActive(active);
		}
	}
}
