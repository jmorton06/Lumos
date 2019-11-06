#include "lmpch.h"
#include "HierarchyWindow.h"
#include "Editor.h"
#include "ECS/EntityManager.h"
#include "App/Application.h"
#include "App/SceneManager.h"
#include "ImGui/ImGuiHelpers.h"
#include <IconFontCppHeaders/IconsFontAwesome5.h>

namespace Lumos
{
	HierarchyWindow::HierarchyWindow()
	{
		m_Name = ICON_FA_LIST_ALT" Hierarchy###hierarchy";
		m_SimpleName = "Hierarchy";
	}

	void HierarchyWindow::DrawNode(Entity* node, bool defaultOpen)
	{
//		if (node == nullptr)
//			return;
//
//		bool show = true;
//
//		if (m_HierarchyFilter.IsActive())
//		{
//			if (!m_HierarchyFilter.PassFilter((node->GetName().c_str())))
//			{
//				show = false;
//			}
//		}
//
//		if (show)
//		{
//			bool noChildren = node->GetChildren().empty();
//
//			ImGuiTreeNodeFlags nodeFlags = ((m_Editor->GetSelected() == node) ? ImGuiTreeNodeFlags_Selected : 0);
//
//			nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding;
//
//			if (noChildren)
//			{
//				nodeFlags |= ImGuiTreeNodeFlags_Leaf;
//			}
//
//			String icon(ICON_FA_CUBE);
//
//            std::stringstream ss;
//			ss << "##";
//			ss << node->GetUUID();
//
//			bool active = node->ActiveInHierarchy();
//			if(!active)
//				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
//
//			bool doubleClicked = false;
//			if (node == m_DoubleClicked)
//			{
//				doubleClicked = true;
//			}
//
//			if(doubleClicked)
//				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 1.0f,2.0f });
//
//			if (m_HadRecentDroppedEntity == node)
//			{
//				ImGui::SetNextItemOpen(true);
//				m_HadRecentDroppedEntity = nullptr;
//			}
//
//            bool nodeOpen = ImGui::TreeNodeEx(ss.str().c_str(), nodeFlags, doubleClicked ? (icon).c_str() :(icon + " " + node->GetName()).c_str(), defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
//
//			if (doubleClicked)
//			{
//				ImGui::SameLine();
//				static char objName[INPUT_BUF_SIZE];
//				strcpy(objName, node->GetName().c_str());
//
//				ImGui::PushItemWidth(-1);
//				if (ImGui::InputText("##Name", objName, IM_ARRAYSIZE(objName), 0))
//					node->SetName(objName);
//				ImGui::PopStyleVar();
//			}
//
//			if (!active)
//				ImGui::PopStyleColor();
//
//			bool deleteEntity = false;
//			if (ImGui::BeginPopupContextItem("Entity context menu"))
//			{
//				if (ImGui::Selectable("Copy")) m_CopiedEntity = node;
//
//				if (m_CopiedEntity)
//				{
//					if (ImGui::Selectable("Paste"))
//					{
//						auto e = EntityManager::Instance()->DuplicateEntity(m_CopiedEntity);
//						node->AddChild(e);
//						m_CopiedEntity = nullptr;
//					}
//				}
//				else
//				{
//					ImGui::TextDisabled("Paste");
//				}
//
//				ImGui::Separator();
//				if (ImGui::Selectable("Duplicate")) EntityManager::Instance()->DuplicateEntity(node);
//				if (ImGui::Selectable("Remove")) deleteEntity = true; if (m_Editor->GetSelected() == node) m_Editor->SetSelected(nullptr);
//				ImGui::Separator();
//				if (ImGui::Selectable("Rename")) m_DoubleClicked = node;
//				ImGui::EndPopup();
//			}
//
//			if (!doubleClicked && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
//			{
//				auto ptr = node;
//				ImGui::SetDragDropPayload("Drag_Entity", &ptr, sizeof(Entity**));
//				ImGui::Text("Moving %s", node->GetName().c_str());
//				ImGui::EndDragDropSource();
//			}
//
//			if (ImGui::BeginDragDropTarget())
//			{
//				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Drag_Entity"))
//				{
//					LUMOS_ASSERT(payload->DataSize == sizeof(Entity**), "Error ImGUI drag entity");
//					auto entity = *reinterpret_cast<Entity**>(payload->Data);
//					node->AddChild(entity);
//
//					m_HadRecentDroppedEntity = node;
//
//					if (m_Editor->GetSelected() == entity)
//						m_Editor->SetSelected(nullptr);
//				}
//				ImGui::EndDragDropTarget();
//			}
//
//			if (ImGui::IsItemClicked() && !deleteEntity)
//				m_Editor->SetSelected(node);
//			else if (m_DoubleClicked == node && ImGui::IsMouseClicked(0) && !ImGui::IsItemHovered(ImGuiHoveredFlags_None))
//				m_DoubleClicked = nullptr;
//
//			if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered(ImGuiHoveredFlags_None))
//				m_DoubleClicked = node;
//
//			if (nodeOpen == false)
//				return;
//
//			for (auto child : node->GetChildren())
//			{
//				this->DrawNode(child);
//			}
//
//			if(deleteEntity)
//				EntityManager::Instance()->DeleteEntity(node);
//
//			ImGui::TreePop();
//		}
//		else
//		{
//			for (auto child : node->GetChildren())
//			{
//				this->DrawNode(child);
//			}
//		}
	}

void HierarchyWindow::DrawNode(entt::entity node, entt::registry& registry)
{
    bool show = true;

	bool hasName = registry.has<NameComponent>(node);
    String name;
	if (hasName)
		name = registry.get<NameComponent>(node).name;
	else
		name = StringFormat::ToString((u32)node);
    
    if (m_HierarchyFilter.IsActive())
    {
        if (!m_HierarchyFilter.PassFilter(name.c_str()))
        {
            show = false;
        }
    }

    if (show)
    {
        bool noChildren = true;//node->GetChildren().empty();

        ImGuiTreeNodeFlags nodeFlags = ((m_Editor->GetSelected() == node) ? ImGuiTreeNodeFlags_Selected : 0);

        nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding;

        if (noChildren)
        {
            nodeFlags |= ImGuiTreeNodeFlags_Leaf;
        }

        String icon(ICON_FA_CUBE);
        
        std::stringstream ss;
        ss << "##";
        ss << (u32)node;
        
        bool active = true;//node->ActiveInHierarchy();
        if(!active)
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

        bool doubleClicked = false;
        if (node == m_DoubleClicked)
        {
            doubleClicked = true;
        }

        if(doubleClicked)
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 1.0f,2.0f });

        if (m_HadRecentDroppedEntity == node)
        {
            ImGui::SetNextItemOpen(true);
            m_HadRecentDroppedEntity = entt::null;
        }

        bool nodeOpen = ImGui::TreeNodeEx(ss.str().c_str(), nodeFlags, doubleClicked ? (icon).c_str() :(icon + " " + name).c_str(), 0);
        
        if (doubleClicked)
        {
            ImGui::SameLine();
            static char objName[INPUT_BUF_SIZE];
            strcpy(objName, name.c_str());

			if (!hasName)
				registry.assign<NameComponent>(node, name);
            ImGui::PushItemWidth(-1);
            if (ImGui::InputText("##Name", objName, IM_ARRAYSIZE(objName), 0))
				registry.get<NameComponent>(node).name = objName;
            ImGui::PopStyleVar();
        }

        if (!active)
            ImGui::PopStyleColor();

        bool deleteEntity = false;
        if (ImGui::BeginPopupContextItem("Entity context menu"))
        {
            if (ImGui::Selectable("Copy")) m_CopiedEntity = node;

            if (m_CopiedEntity != entt::null)
            {
                if (ImGui::Selectable("Paste"))
                {
					//auto e = registry.clone(node);// EntityManager::Instance()->DuplicateEntity(m_CopiedEntity);
                    //node->AddChild(e);
                    m_CopiedEntity = entt::null;
                }
            }
            else
            {
                ImGui::TextDisabled("Paste");
            }

            ImGui::Separator();
			
			//if (ImGui::Selectable("Duplicate")) registry.clone(node);// EntityManager::Instance()->DuplicateEntity(node);
            if (ImGui::Selectable("Remove")) deleteEntity = true; if (m_Editor->GetSelected() == node) m_Editor->SetSelected(entt::null);
            ImGui::Separator();
            if (ImGui::Selectable("Rename")) m_DoubleClicked = node;
            ImGui::EndPopup();
        }

        if (!doubleClicked && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            auto ptr = node;
            ImGui::SetDragDropPayload("Drag_Entity", &ptr, sizeof(Entity**));
            //ImGui::Text("Moving %s", node->GetName().c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Drag_Entity"))
            {
                LUMOS_ASSERT(payload->DataSize == sizeof(entt::entity*), "Error ImGUI drag entity");
                auto entity = *reinterpret_cast<entt::entity*>(payload->Data);
                //node->AddChild(entity);

                m_HadRecentDroppedEntity = node;

                if (m_Editor->GetSelected() == entity)
                    m_Editor->SetSelected(entt::null);
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::IsItemClicked() && !deleteEntity)
            m_Editor->SetSelected(node);
        else if (m_DoubleClicked == node && ImGui::IsMouseClicked(0) && !ImGui::IsItemHovered(ImGuiHoveredFlags_None))
            m_DoubleClicked = entt::null;// = nullptr;

        if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered(ImGuiHoveredFlags_None))
            m_DoubleClicked = node;

        if (nodeOpen == false)
            return;

        if(deleteEntity)
            registry.destroy(node);

        ImGui::TreePop();
    }
//    else
//    {
//        for (auto child : node->GetChildren())
//        {
//            this->DrawNode(child);
//        }
//    }
}

	void HierarchyWindow::OnImGui()
	{
		auto flags = ImGuiWindowFlags_NoCollapse;
		ImGui::Begin(m_Name.c_str(), &m_Active, flags);
		{
			ImGui::Indent();
			ImGui::Text(ICON_FA_SEARCH);
			ImGui::SameLine();
			m_HierarchyFilter.Draw("##HierarchyFilter", ImGui::GetContentRegionAvailWidth() - ImGui::GetStyle().IndentSpacing);
            
            ImGui::Unindent();

			if (ImGui::CollapsingHeader("Scene",ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent();
                
                auto& registry = Application::Instance()->GetSceneManager()->GetCurrentScene()->GetRegistry();
                
                registry.each([&](auto entity)
                {
                    DrawNode(entity, registry);
                });
			}

			Application::Instance()->GetSceneManager()->GetCurrentScene()->OnImGui();
		}
		ImGui::End();
	}
}
