#include "LM.h"
#include "Entity.h"

#include "Graphics/Renderers/DebugRenderer.h"
#include "App/Scene.h"
#include "Graphics/API/Context.h"

#include <imgui/imgui.h>
#include <imgui/plugins/ImGuizmo.h>
#include "Graphics/Camera/Camera.h"
#include "App/Application.h"
#include "App/SceneManager.h"

namespace Lumos
{
	Entity::Entity(Scene* scene) : m_Name("Unnamed"), m_pScene(scene), m_pParent(nullptr), m_BoundingRadius(1), m_FrustumCullFlags(0)
	{
        Init();
	}

	Entity::Entity(const String& name,Scene* scene) : m_Name(name), m_pScene(scene), m_pParent(nullptr), m_BoundingRadius(1),
	                                     m_FrustumCullFlags(0)
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
        
        if(component->GetType() == ComponentType::Transform)
            m_DefaultTransformComponent = (TransformComponent*)component.get();
        
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
        if(m_DefaultTransformComponent && m_DefaultTransformComponent->m_Transform.HasUpdated())
        {
            if(!m_pParent)
                m_DefaultTransformComponent->m_Transform.SetWorldMatrix(maths::Matrix4());
			else
			{
				m_DefaultTransformComponent->m_Transform.SetWorldMatrix(m_pParent->GetTransform()->m_Transform.GetWorldMatrix());
			}
				
            
            for(auto child : m_vpChildren)
            {
                if(child && child->GetTransform())
                    child->GetTransform()->SetWorldMatrix(m_DefaultTransformComponent->m_Transform.GetWorldMatrix());
            }
            
            for (const auto& component : m_Components)
            {
                component.second->OnUpdateComponent(dt);
                
                if (m_DefaultTransformComponent->m_Transform.HasUpdated())
                {
                    component.second->OnUpdateTransform(m_DefaultTransformComponent->m_Transform.GetWorldMatrix());
                }
            }
            
            m_DefaultTransformComponent->m_Transform.SetHasUpdated(false);
        }
	}

	void Entity::AddChildObject(std::shared_ptr<Entity>& child)
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
		maths::Matrix4 view = Application::Instance()->GetSceneManager()->GetCurrentScene()->GetCamera()->GetViewMatrix();
		maths::Matrix4 proj = Application::Instance()->GetSceneManager()->GetCurrentScene()->GetCamera()->GetProjectionMatrix();
        
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
        
        auto parentMat = m_pParent ? m_pParent->GetTransform()->m_Transform.GetWorldMatrix() : maths::Matrix4();

        ImGuizmo::Manipulate(view.values, proj.values, static_cast<ImGuizmo::OPERATION>(mode),ImGuizmo::LOCAL, model.values, nullptr, nullptr);

		if (GetTransform() != nullptr)
        {
            auto mat = (maths::Matrix4::Inverse(parentMat) * model);
            mat.Transpose();
            m_DefaultTransformComponent->m_Transform.SetLocalTransform(mat);
        }
	}
    
    void Entity::OnIMGUI()
    {
		static char objName[INPUT_BUF_SIZE];
		strcpy(objName, m_Name.c_str());

		ImGuiInputTextFlags inputFlag = ImGuiInputTextFlags_EnterReturnsTrue;
		if (ImGui::InputText("Name", objName, IM_ARRAYSIZE(objName), inputFlag))
			m_Name = objName;
        
        ImGui::Text("%s", m_pParent ? m_pParent->GetName().c_str() : "No Parent");

        for(auto& component: m_Components)
        {
            component.second->OnIMGUI();
        }
    }
    
    void Entity::SetParent(Entity *parent)
    {
        m_pParent = parent;
        m_DefaultTransformComponent->SetWorldMatrix(m_pParent->GetTransform()->m_Transform.GetWorldMatrix());
    }
}
