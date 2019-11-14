#include "lmpch.h"
#include "HierarchyWindow.h"
#include "Editor.h"
#include "App/Application.h"
#include "App/SceneManager.h"
#include "ImGui/ImGuiHelpers.h"
#include "App/SceneGraph.h"
#include <IconFontCppHeaders/IconsFontAwesome5.h>

namespace Lumos
{
	HierarchyWindow::HierarchyWindow()
	{
		m_Name = ICON_FA_LIST_ALT" Hierarchy###hierarchy";
		m_SimpleName = "Hierarchy";
	}

	void HierarchyWindow::DrawNode(entt::entity node, entt::registry& registry)
	{
		bool show = true;

		if (!registry.valid(node))
			return;

		auto nameComponent = registry.try_get<NameComponent>(node);
		String name = nameComponent ? nameComponent->name : StringFormat::ToString(entt::to_integer(node));
    
		if (m_HierarchyFilter.IsActive())
		{
			if (!m_HierarchyFilter.PassFilter(name.c_str()))
			{
				show = false;
			}
		}

		if (show)
		{
			auto hierarchyComponent = registry.try_get<Hierarchy>(node);
			bool noChildren = true;

			if (hierarchyComponent != nullptr && hierarchyComponent->first() != entt::null)
				noChildren = false;

			ImGuiTreeNodeFlags nodeFlags = ((m_Editor->GetSelected() == node) ? ImGuiTreeNodeFlags_Selected : 0);

			nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding;

			if (noChildren)
			{
				nodeFlags |= ImGuiTreeNodeFlags_Leaf;
			}
        
			bool active = true;
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

			bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)entt::to_integer(node), nodeFlags, ICON_FA_CUBE " %s", doubleClicked ? "" :(name).c_str());

			if (doubleClicked)
			{
				ImGui::SameLine();
				static char objName[INPUT_BUF_SIZE];
				strcpy(objName, name.c_str());

				ImGui::PushItemWidth(-1);
				if (ImGui::InputText("##Name", objName, IM_ARRAYSIZE(objName), 0))
					registry.get_or_assign<NameComponent>(node).name = objName;
				ImGui::PopStyleVar();
			}

			if (!active)
				ImGui::PopStyleColor();

			bool deleteEntity = false;
			if (ImGui::BeginPopupContextItem(name.c_str()))
			{
				if (ImGui::Selectable("Copy")) m_CopiedEntity = node;

				if (m_CopiedEntity != entt::null)
				{
					if (ImGui::Selectable("Paste"))
					{
						//auto e = registry.clone(node);
						//node->AddChild(e);
						m_CopiedEntity = entt::null;
					}
				}
				else
				{
					ImGui::TextDisabled("Paste");
				}

				ImGui::Separator();
			
				//if (ImGui::Selectable("Duplicate")) registry.clone(node);
				if (ImGui::Selectable("Remove")) deleteEntity = true; if (m_Editor->GetSelected() == node) m_Editor->SetSelected(entt::null);
				ImGui::Separator();
				if (ImGui::Selectable("Rename")) m_DoubleClicked = node;
				ImGui::EndPopup();
			}

			if (!doubleClicked && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				auto ptr = node;
				ImGui::SetDragDropPayload("Drag_Entity", &ptr, sizeof(entt::entity*));
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
					auto hierarchyComponent = registry.try_get<Hierarchy>(entity);
					if (hierarchyComponent)
					{
						bool entityIsParentOfNode = false;
						auto nodeHierarchyComponent = registry.try_get<Hierarchy>(node);
						if (nodeHierarchyComponent)
						{
							auto parent = nodeHierarchyComponent->parent();
							while (parent != entt::null)
							{
								if (parent == entity)
								{
									entityIsParentOfNode = true;
									break;
								}
								else
								{
									nodeHierarchyComponent = registry.try_get<Hierarchy>(parent);
									parent = nodeHierarchyComponent ? nodeHierarchyComponent->parent() : entt::null;
								}
							}
						}
						

						if(!entityIsParentOfNode)
							Hierarchy::Reparent(entity, node, registry, *hierarchyComponent);
					}
					else
					{
						registry.assign<Hierarchy>(entity, node);
					}
					m_HadRecentDroppedEntity = node;

					if (m_Editor->GetSelected() == entity)
						m_Editor->SetSelected(entt::null);
				}
				ImGui::EndDragDropTarget();
			}

			if (ImGui::IsItemClicked() && !deleteEntity)
				m_Editor->SetSelected(node);
			else if (m_DoubleClicked == node && ImGui::IsMouseClicked(0) && !ImGui::IsItemHovered(ImGuiHoveredFlags_None))
				m_DoubleClicked = entt::null;

			if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered(ImGuiHoveredFlags_None))
				m_DoubleClicked = node;

			if (deleteEntity)
			{
				DestroyEntity(node, registry);
				if(nodeOpen)
					ImGui::TreePop();
				return;
			}
		
			if (nodeOpen == false)
			{
				return;
			}

			if (!noChildren)
			{
				entt::entity child = hierarchyComponent->first();
				while (child != entt::null && registry.valid(child))
				{
					ImGui::Indent();
					DrawNode(child, registry);
					ImGui::Unindent();
					auto hierarchyComponent = registry.try_get<Hierarchy>(child);
					if (hierarchyComponent)
						child = hierarchyComponent->next();
				}
			}

			ImGui::TreePop();
		}
	}

	void HierarchyWindow::DestroyEntity(entt::entity entity, entt::registry& registry)
	{
		auto hierarchyComponent = registry.try_get<Hierarchy>(entity);
		if (hierarchyComponent)
		{
			entt::entity child = hierarchyComponent->first();
			while (child != entt::null)
			{
				auto hierarchyComponent = registry.try_get<Hierarchy>(child);
				auto next = hierarchyComponent ? hierarchyComponent->next() : entt::null;
				DestroyEntity(child, registry);
				child = next;
			}
		}
		registry.destroy(entity);
	}

	void HierarchyWindow::OnImGui()
	{
		auto flags = ImGuiWindowFlags_NoCollapse;
		ImGui::Begin(m_Name.c_str(), &m_Active, flags);
		{
			ImGui::Indent();
			ImGui::TextUnformatted(ICON_FA_SEARCH);
			ImGui::SameLine();
			m_HierarchyFilter.Draw("##HierarchyFilter", ImGui::GetContentRegionAvailWidth() - ImGui::GetStyle().IndentSpacing);
            
            ImGui::Unindent();

			if (ImGui::CollapsingHeader("Scene",ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent();
                
                auto& registry = Application::Instance()->GetSceneManager()->GetCurrentScene()->GetRegistry();
                
                registry.each([&](auto entity)
                {
					if (registry.valid(entity))
					{
						auto hierarchyComponent = registry.try_get<Hierarchy>(entity);

						if (!hierarchyComponent || (hierarchyComponent && hierarchyComponent->parent() == entt::null))
							DrawNode(entity, registry);
					}
                });
			}
		}
		ImGui::End();
	}
}
