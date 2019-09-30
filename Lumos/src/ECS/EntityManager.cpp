#include "lmpch.h"
#include "EntityManager.h"
#include "Utilities/RandomNumberGenerator.h"

#include <imgui/imgui.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>

namespace Lumos
{
    Entity::Entity(EntityManager* manager, const String& name) 
		: m_Name(name), 
		m_Parent(nullptr),
		m_FrustumCullFlags(0),
		m_Active(true), 
		m_Manager(manager)
    {
        Init();
    }
    
    Entity::~Entity()
    {
        ComponentManager::Instance()->EntityDestroyed(this);
    }
    
    void Entity::Init()
    {
        m_UUID = RandomNumberGenerator32::Rand(0, INT_MAX);
    }
    
    void Entity::OnUpdateObject(float dt)
    {
		auto transformComponent = this->GetTransformComponent();

        if (transformComponent && transformComponent->GetTransform()->HasUpdated())
        {
            if (!m_Parent)
				transformComponent->SetWorldMatrix(Maths::Matrix4());
            else
            {
				transformComponent->SetWorldMatrix(m_Parent->GetTransformComponent()->GetTransform()->GetWorldMatrix());
            }
            
            
            for (auto child : m_Children)
            {
                if (child && child->GetTransformComponent())
                    child->GetTransformComponent()->SetWorldMatrix(transformComponent->GetTransform()->GetWorldMatrix());
            }
            
			transformComponent->GetTransform()->SetHasUpdated(false);
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
		auto transformComponent = this->GetComponent<TransformComponent>();
        if (!transformComponent)
        {
            AddComponent<TransformComponent>();
			transformComponent = this->GetComponent<TransformComponent>();
        }
        
        return transformComponent;
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
            ImGui::OpenPopup("Add Component");
        }
        
        if (ImGui::BeginPopup("Add Component", 3))
        {
			const auto& componentArrays = ComponentManager::Instance()->GetComponentArrays();

			for(const auto& componentArray : componentArrays)
				if (ImGui::Selectable(componentArray.second->GetName().c_str())) ComponentManager::Instance()->CreateComponent(this, componentArray.second->GetID());

            ImGui::EndPopup();
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
        
		ComponentManager::Instance()->OnImGui(this);
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
    
    nlohmann::json Entity::Serialise()
    {
        nlohmann::json output;
        output["typeID"] = LUMOS_TYPENAME(Entity);
        output["instanceID"] = m_UUID;
        output["active"] = m_Active;
        output["name"] = m_Name;
        output["parentID"] = m_Parent ? m_Parent->GetUUID() : 0;
        output["prefabFilePath"] = m_PrefabFileLocation;
        
        nlohmann::json childrenIDs = nlohmann::json::array_t();
        for (int i = 0; i < m_Children.size(); i++)
            childrenIDs.push_back(m_Children[i]->GetUUID());
        
        output["children"] = childrenIDs;
        
        nlohmann::json serializedComponents = nlohmann::json::array_t();
      /*  auto components = GetAllComponents();
        
		for (int i = 0; i < components.size(); ++i)
		{
			auto sComp = components[i]->Serialise();
			if(sComp != nullptr)
				serializedComponents.push_back(sComp);
		}*/
            
        
        output["components"] = serializedComponents;
        
        return output;
    }
    
    void Entity::Deserialise(nlohmann::json& data)
    {
        m_Active = data["active"];
        m_UUID = data["instanceID"];
        m_Name = data["name"];
        m_PrefabFileLocation = data["prefabFilePath"];
        
        u32 parentID = data["parentID"];

		m_Parent = m_Manager->GetEntity(parentID);
        nlohmann::json::array_t children = data["children"];
        nlohmann::json::array_t components = data["components"];
        
        for (int i = 0; i < components.size(); i++)
        {
            size_t type = components[i]["typeID"];
			ComponentManager::Instance()->CreateComponent(this, type);
			//component->Deserialise(components[i]);
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

Lumos::Entity * Lumos::EntityManager::GetEntity(u32 uuid)
{
	for (auto entity : m_Entities)
	{
		if (entity->GetUUID() == uuid)
			return entity;
	}
	return nullptr;
}
