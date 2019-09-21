#include "lmpch.h"
#include "EntityManager.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/Camera/Camera.h"
#include "App/Application.h"
#include "App/Scene.h"
#include "App/SceneManager.h"
#include "Maths/MathsUtilities.h"

#include <imgui/imgui.h>
#include <imgui/plugins/ImGuizmo.h>

#include <IconFontCppHeaders/IconsFontAwesome5.h>

namespace Lumos
{
    Entity::Entity(EntityManager* manager, const String& name) : m_Name(name), m_Parent(nullptr), m_BoundingRadius(1),
    m_FrustumCullFlags(0), m_Active(true), m_Manager(manager)
    {
        Init();
    }
    
    Entity::~Entity()
    {
        ComponentManager::Instance()->EntityDestroyed(this);
    }
    
    void Entity::Init()
    {
        m_UUID = Maths::GenerateUUID();
    }
    
    void Entity::OnUpdateObject(float dt)
    {
        if (m_DefaultTransformComponent && m_DefaultTransformComponent->GetTransform()->HasUpdated())
        {
            if (!m_Parent)
                m_DefaultTransformComponent->SetWorldMatrix(Maths::Matrix4());
            else
            {
                m_DefaultTransformComponent->SetWorldMatrix(m_Parent->GetTransformComponent()->GetTransform()->GetWorldMatrix());
            }
            
            
            for (auto child : m_Children)
            {
                if (child && child->GetTransformComponent())
                    child->GetTransformComponent()->SetWorldMatrix(m_DefaultTransformComponent->GetTransform()->GetWorldMatrix());
            }
            
            m_DefaultTransformComponent->GetTransform()->SetHasUpdated(false);
        }
    }
    
    void Entity::AddChild(Entity* child)
    {
        if (child->m_Parent)
            child->m_Parent->RemoveChild(child);
        
        child->GetTransformComponent()->GetTransform()->SetHasUpdated(true);
        
        child->m_Parent = this;
        m_Children.push_back(child);
    }
    
    void Entity::RemoveChild(Entity* child)
    {
        for (size_t i = 0; i < m_Children.size(); i++)
        {
            if (m_Children[i] == child)
            {
                m_Children.erase(m_Children.begin() + i);
            }
        }
    }
    
    TransformComponent* Entity::GetTransformComponent()
    {
        if (!m_DefaultTransformComponent)
        {
            m_DefaultTransformComponent = GetComponent<TransformComponent>();
            
            if (!m_DefaultTransformComponent)
                AddComponent<TransformComponent>();
        }
        
        return m_DefaultTransformComponent;
    }
    
    void Entity::OnGuizmo(u32 mode)
    {
        Maths::Matrix4 view = Application::Instance()->GetSceneManager()->GetCurrentScene()->GetCamera()->GetViewMatrix();
        Maths::Matrix4 proj = Application::Instance()->GetSceneManager()->GetCurrentScene()->GetCamera()->GetProjectionMatrix();
        
#ifdef LUMOS_RENDER_API_VULKAN
        if (Graphics::GraphicsContext::GetRenderAPI() == Graphics::RenderAPI::VULKAN)
            proj[5] *= -1.0f;
#endif
        ImGuizmo::SetDrawlist();
        
        Maths::Matrix4 model = Maths::Matrix4();
        if (this->GetComponent<TransformComponent>() != nullptr)
            model = GetComponent<TransformComponent>()->GetTransform()->GetWorldMatrix();
        
        float delta[16];
        ImGuizmo::Manipulate(view.values, proj.values, static_cast<ImGuizmo::OPERATION>(mode), ImGuizmo::LOCAL, model.values, delta, nullptr);
        
        if (GetTransformComponent() != nullptr && ImGuizmo::IsUsing())
        {
            auto mat = Maths::Matrix4(delta) * m_DefaultTransformComponent->GetTransform()->GetLocalMatrix();
            m_DefaultTransformComponent->GetTransform()->SetLocalTransform(mat);
            m_DefaultTransformComponent->GetTransform()->ApplyTransform();
            
        }
    }
    
    void Entity::OnImGui()
    {
        static char objName[INPUT_BUF_SIZE];
        strcpy(objName, m_Name.c_str());
        
        ImGuiInputTextFlags inputFlag = ImGuiInputTextFlags_EnterReturnsTrue;
        
        ImGui::Checkbox("##Active", &m_Active);
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_PLUS))
        {
            static float value = 0.5f;
            if (ImGui::BeginPopup("item context menu", 3))
            {
                if (ImGui::Selectable("Copy")) value = 0.0f;
                if (ImGui::Selectable("Paste")) value = 3.1415f;
                ImGui::EndPopup();
            }
        }
        ImGui::SameLine();
        
        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("##Name", objName, IM_ARRAYSIZE(objName), inputFlag))
            m_Name = objName;
        
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();
        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", "Parent");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::Text("%s", m_Parent ? m_Parent->GetName().c_str() : "No Parent");
        ImGui::PopItemWidth();
        
        ImGui::NextColumn();
        
        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
        
        auto components = GetAllComponents();
        for (auto& component : components)
        {
            ImGui::Separator();
            bool open = ImGui::CollapsingHeader(component->GetName().c_str());
            
            if (open)
            {
                if (component->GetCanDisable())
                {
                    ImGui::Checkbox("Active", &component->GetActive());
                }
                component->OnImGui();
            }
        }
    }
    
    void Entity::SetParent(Entity *parent)
    {
        if (m_Parent != nullptr)
            m_Parent->RemoveChild(this);
        
        m_Parent = parent;
        m_DefaultTransformComponent->SetWorldMatrix(m_Parent->GetTransformComponent()->GetTransform()->GetWorldMatrix());
    }
    
    const bool Entity::ActiveInHierarchy() const
    {
        if (!Active())
            return false;
        
        if (m_Parent)
            return m_Parent->ActiveInHierarchy();
        else
            return true;
    }
    
    void Entity::SetActiveRecursive(bool active)
    {
        m_Active = active;
        
        for (auto child : m_Children)
        {
            child->SetActive(active);
        }
    }
    
	const std::vector<LumosComponent*> Entity::GetAllComponents()
    {
        return ComponentManager::Instance()->GetAllComponents(this);
    }
    
    nlohmann::json Entity::Serialise()
    {
        nlohmann::json output;
        output["typeID"] = LUMOS_TYPENAME(Entity);
        output["instanceID"] = m_UUID;
        output["active"] = m_Active;
        output["name"] = m_Name;
        output["parentID"] = m_Parent ? m_Parent->GetUUID() : "";
        output["prefabFilePath"] = m_PrefabFileLocation;
        
        nlohmann::json childrenIDs = nlohmann::json::array_t();
        for (int i = 0; i < m_Children.size(); i++)
            childrenIDs.push_back(m_Children[i]->GetUUID());
        
        output["children"] = childrenIDs;
        
        nlohmann::json serializedComponents = nlohmann::json::array_t();
        auto components = GetAllComponents();
        
        for (int i = 0; i < components.size(); ++i)
            serializedComponents.push_back(components[i]->Serialise());
        
        output["components"] = serializedComponents;
        
        return output;
    }
    
    void Entity::Deserialise(nlohmann::json& data)
    {
        m_Active = data["active"];
        m_UUID = data["instanceID"];
        m_Name = data["name"];
        m_PrefabFileLocation = data["prefabFilePath"];
        
        auto parentID = data["parentID"];
        nlohmann::json::array_t children = data["children"];
        nlohmann::json::array_t components = data["components"];
        
        for (int i = 0; i < components.size(); i++)
        {
            auto type = components[i]["typeID"];
            //auto component = new <typeID to class>();
        }
    }
}

Lumos::EntityManager::~EntityManager()
{
    Clear();
}

void Lumos::EntityManager::Clear()
{
	for (auto entity : m_Entities)
		delete entity;

	m_Entities.clear();
}

Lumos::Entity* Lumos::EntityManager::CreateEntity(const String& name)
{
	auto entity = lmnew Entity(this, name);
	m_Entities.emplace_back(entity);
	return entity;
}

void Lumos::EntityManager::DeleteEntity(Entity* entity)
{
	for (int i = 0; i < m_Entities.size(); i++)
	{
		if (m_Entities[i] == entity)
		{
            if(entity != nullptr)
                delete entity;
			m_Entities.erase(m_Entities.begin() + i);
		}
	}
}
