#include "lmpch.h"
#include "HierarchyWindow.h"
#include "Editor.h"
#include "Core/Application.h"
#include "Scene/SceneManager.h"
#include "ImGui/ImGuiHelpers.h"
#include "Scene/SceneGraph.h"
#include "Maths/Transform.h"
#include "InspectorWindow.h"
#include "Scene/Entity.h"

#include "Graphics/Camera/Camera.h"
#include "Graphics/Light.h"
#include "Graphics/Environment.h"
#include "Graphics/Sprite.h"
#include "Scene/Component/Components.h"

#include <imgui/imgui_internal.h>
#include <IconFontCppHeaders/IconsMaterialDesignIcons.h>

namespace Lumos
{
	HierarchyWindow::HierarchyWindow()
		: m_HadRecentDroppedEntity(entt::null)
        , m_DoubleClicked(entt::null)
	{
		m_Name = "Hierarchy###hierarchy";
		m_SimpleName = "Hierarchy";
	}

	void HierarchyWindow::DrawNode(entt::entity node, entt::registry& registry)
	{
		bool show = true;

		if(!registry.valid(node))
			return;

		const auto nameComponent = registry.try_get<NameComponent>(node);
		std::string name = nameComponent ? nameComponent->name : StringFormat::ToString(entt::to_integral(node));

		if(m_HierarchyFilter.IsActive())
		{
			if(!m_HierarchyFilter.PassFilter(name.c_str()))
			{
				show = false;
			}
		}

		if(show)
		{
			auto hierarchyComponent = registry.try_get<Hierarchy>(node);
			bool noChildren = true;

			if(hierarchyComponent != nullptr && hierarchyComponent->first() != entt::null)
				noChildren = false;

			ImGuiTreeNodeFlags nodeFlags = ((m_Editor->GetSelected() == node) ? ImGuiTreeNodeFlags_Selected : 0);

			nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding;

			if(noChildren)
			{
				nodeFlags |= ImGuiTreeNodeFlags_Leaf;
			}

			auto activeComponent = registry.try_get<ActiveComponent>(node);
			bool active = activeComponent ? activeComponent->active : true;

			if(!active)
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

			bool doubleClicked = false;
			if(node == m_DoubleClicked)
			{
				doubleClicked = true;
			}

			if(doubleClicked)
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {1.0f, 2.0f});

			if(m_HadRecentDroppedEntity == node)
			{
				ImGui::SetNextItemOpen(true);
				m_HadRecentDroppedEntity = entt::null;
			}

			std::string icon = ICON_MDI_CUBE_OUTLINE;
			auto& iconMap = m_Editor->GetComponentIconMap();

			if(registry.has<Camera>(node))
			{
				if(iconMap.find(typeid(Camera).hash_code()) != iconMap.end())
					icon = iconMap[typeid(Camera).hash_code()];
			}
			else if(registry.has<SoundComponent>(node))
			{
				if(iconMap.find(typeid(SoundComponent).hash_code()) != iconMap.end())
					icon = iconMap[typeid(SoundComponent).hash_code()];
			}
			else if(registry.has<Physics2DComponent>(node))
			{
				if(iconMap.find(typeid(Physics2DComponent).hash_code()) != iconMap.end())
					icon = iconMap[typeid(Physics2DComponent).hash_code()];
			}
			else if(registry.has<Graphics::Light>(node))
			{
				if(iconMap.find(typeid(Graphics::Light).hash_code()) != iconMap.end())
					icon = iconMap[typeid(Graphics::Light).hash_code()];
			}
			else if(registry.has<Graphics::Environment>(node))
			{
				if(iconMap.find(typeid(Graphics::Environment).hash_code()) != iconMap.end())
					icon = iconMap[typeid(Graphics::Environment).hash_code()];
			}
            else if(registry.has<Graphics::Sprite>(node))
            {
                if(iconMap.find(typeid(Graphics::Sprite).hash_code()) != iconMap.end())
                    icon = iconMap[typeid(Graphics::Sprite).hash_code()];
            }

			bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)entt::to_integral(node), nodeFlags, (icon + " %s").c_str(), doubleClicked ? "" : (name).c_str());

			if(doubleClicked)
			{
				ImGui::SameLine();
				static char objName[INPUT_BUF_SIZE];
				strcpy(objName, name.c_str());

				ImGui::PushItemWidth(-1);
				if(ImGui::InputText("##Name", objName, IM_ARRAYSIZE(objName), 0))
					registry.get_or_emplace<NameComponent>(node).name = objName;
				ImGui::PopStyleVar();
			}
#if 0
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 22.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));
            if(ImGui::Button(active ? ICON_MDI_EYE : ICON_MDI_EYE_OFF))
            {
                auto& activeComponent = registry.get_or_emplace<ActiveComponent>(node);

                activeComponent.active = !active;
            }
            ImGui::PopStyleColor();
#endif

			if(!active)
				ImGui::PopStyleColor();

			bool deleteEntity = false;
			if(ImGui::BeginPopupContextItem(name.c_str()))
			{
				if(ImGui::Selectable("Copy"))
					m_Editor->SetCopiedEntity(node);

				if(ImGui::Selectable("Cut"))
					m_Editor->SetCopiedEntity(node, true);

				if(m_Editor->GetCopiedEntity() != entt::null && registry.valid(m_Editor->GetCopiedEntity()))
				{
					if(ImGui::Selectable("Paste"))
					{
                        auto scene = Application::Get().GetSceneManager()->GetCurrentScene();
                        Entity copiedEntity = {m_Editor->GetCopiedEntity(), scene};
                        if(!copiedEntity.Valid())
                        {
                            m_Editor->SetCopiedEntity(entt::null);
                        }
                        else
                        {
                            scene->DuplicateEntity(copiedEntity, {node, scene});
                            
                            if(m_Editor->GetCutCopyEntity())
                                deleteEntity = true;
                        }
					}
				}
				else
				{
					ImGui::TextDisabled("Paste");
				}

				ImGui::Separator();

				if (ImGui::Selectable("Duplicate")) 
				{
					auto scene = Application::Get().GetSceneManager()->GetCurrentScene();
					scene->DuplicateEntity({node , scene});
				}
				if(ImGui::Selectable("Remove"))
					deleteEntity = true;
				if(m_Editor->GetSelected() == node)
					m_Editor->SetSelected(entt::null);
				ImGui::Separator();
				if(ImGui::Selectable("Rename"))
					m_DoubleClicked = node;
                ImGui::Separator();

                if (ImGui::Selectable("Add Child"))
                {
                    auto scene = Application::Get().GetSceneManager()->GetCurrentScene();
                    auto child = scene->CreateEntity();
                    child.SetParent({ node, scene});
                }
				ImGui::EndPopup();
			}

			if(!doubleClicked && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				auto ptr = node;
				ImGui::SetDragDropPayload("Drag_Entity", &ptr, sizeof(entt::entity*));
				ImGui::Text(ICON_MDI_ARROW_UP);
				ImGui::EndDragDropSource();
			}

			const ImGuiPayload* payload = ImGui::GetDragDropPayload();
			if(payload != NULL && payload->IsDataType("Drag_Entity"))
			{
				bool acceptable;

				LUMOS_ASSERT(payload->DataSize == sizeof(entt::entity*), "Error ImGUI drag entity");
				auto entity = *reinterpret_cast<entt::entity*>(payload->Data);
				auto hierarchyComponent = registry.try_get<Hierarchy>(entity);
				if(hierarchyComponent != nullptr)
				{
					acceptable = entity != node && (!IsParentOfEntity(entity, node, registry)) && (hierarchyComponent->parent() != node);
				}
				else
					acceptable = entity != node;

				if(ImGui::BeginDragDropTarget())
				{
					// Drop directly on to node and append to the end of it's children list.
					if(ImGui::AcceptDragDropPayload("Drag_Entity"))
					{
						if(acceptable)
						{
							if(hierarchyComponent)
								Hierarchy::Reparent(entity, node, registry, *hierarchyComponent);
							else
							{
								registry.emplace<Hierarchy>(entity, node);
							}
							m_HadRecentDroppedEntity = node;
						}
					}

					ImGui::EndDragDropTarget();
				}

				if(m_Editor->GetSelected() == entity)
					m_Editor->SetSelected(entt::null);
			}

			if(ImGui::IsItemClicked() && !deleteEntity)
				m_Editor->SetSelected(node);
			else if(m_DoubleClicked == node && ImGui::IsMouseClicked(0) && !ImGui::IsItemHovered(ImGuiHoveredFlags_None))
				m_DoubleClicked = entt::null;

			if(ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered(ImGuiHoveredFlags_None))
			{
				m_DoubleClicked = node;
				if(Application::Get().GetEditorState() == EditorState::Preview)
				{
					auto transform = registry.try_get<Maths::Transform>(node);
					if(transform)
						m_Editor->FocusCamera(transform->GetWorldPosition(), 2.0f, 2.0f);
				}
			}

			if(deleteEntity)
			{
				DestroyEntity(node, registry);
				if(nodeOpen)
					ImGui::TreePop();
				return;
			}

			if(nodeOpen == false)
			{
				return;
			}

			if(!noChildren)
			{
				entt::entity child = hierarchyComponent->first();
				while(child != entt::null && registry.valid(child))
				{
					ImGui::Indent(8.0f);
					DrawNode(child, registry);
					ImGui::Unindent(8.0f);
                
                    if(registry.valid(child))
                    {
                        auto hierarchyComponent = registry.try_get<Hierarchy>(child);
                        if(hierarchyComponent)
                            child = hierarchyComponent->next();
                    }
				}
			}

			ImGui::TreePop();
		}
	}

	void HierarchyWindow::DestroyEntity(entt::entity entity, entt::registry& registry)
	{
		auto hierarchyComponent = registry.try_get<Hierarchy>(entity);
		if(hierarchyComponent)
		{
			entt::entity child = hierarchyComponent->first();
			while(child != entt::null)
			{
				auto hierarchyComponent = registry.try_get<Hierarchy>(child);
				auto next = hierarchyComponent ? hierarchyComponent->next() : entt::null;
				DestroyEntity(child, registry);
				child = next;
			}
		}
		registry.destroy(entity);
	}

	bool HierarchyWindow::IsParentOfEntity(entt::entity entity, entt::entity child, entt::registry& registry)
	{
		auto nodeHierarchyComponent = registry.try_get<Hierarchy>(child);
		if(nodeHierarchyComponent)
		{
			auto parent = nodeHierarchyComponent->parent();
			while(parent != entt::null)
			{
				if(parent == entity)
				{
					return true;
				}
				else
				{
					nodeHierarchyComponent = registry.try_get<Hierarchy>(parent);
					parent = nodeHierarchyComponent ? nodeHierarchyComponent->parent() : entt::null;
				}
			}
		}

		return false;
	}

	void HierarchyWindow::OnImGui()
	{
		auto flags = ImGuiWindowFlags_NoCollapse;
		ImGui::Begin(m_Name.c_str(), &m_Active, flags);
		{
			auto scene = Application::Get().GetSceneManager()->GetCurrentScene();
			auto& registry = scene->GetRegistry();

			if(ImGui::BeginPopupContextWindow())
			{
				if(ImGui::Selectable("Add Entity"))
				{
					scene->CreateEntity();
				}
				ImGui::EndPopup();
			}
        
            const std::string& sceneName = scene->GetSceneName();
            
            static char objName[INPUT_BUF_SIZE];
            strcpy(objName, sceneName.c_str());
            
            ImGui::PushItemWidth(-1);
            if(ImGui::InputText("##Name", objName, IM_ARRAYSIZE(objName), 0))
                scene->SetName(objName);
            ImGui::Separator();

			ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImGui::GetStyleColorVec4(ImGuiCol_TabActive));
			ImGui::TextUnformatted(ICON_MDI_MAGNIFY);
			ImGui::SameLine();
			m_HierarchyFilter.Draw("##HierarchyFilter", ImGui::GetContentRegionAvail().x - ImGui::GetStyle().IndentSpacing);
			ImGui::PopStyleColor();
			ImGui::Unindent();

			{
				if(ImGui::BeginDragDropTarget())
				{
					if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Drag_Entity"))
					{
						LUMOS_ASSERT(payload->DataSize == sizeof(entt::entity*), "Error ImGUI drag entity");
						auto entity = *reinterpret_cast<entt::entity*>(payload->Data);
						auto hierarchyComponent = registry.try_get<Hierarchy>(entity);
						if(hierarchyComponent)
						{
							Hierarchy::Reparent(entity, entt::null, registry, *hierarchyComponent);
						}
					}
					ImGui::EndDragDropTarget();
				}

				ImGui::Indent();

				registry.each([&](auto entity) {
					if(registry.valid(entity))
					{
						auto hierarchyComponent = registry.try_get<Hierarchy>(entity);

						if(!hierarchyComponent || hierarchyComponent->parent() == entt::null)
							DrawNode(entity, registry);
					}
				});

				//Only supports one scene
				ImVec2 min_space = ImGui::GetWindowContentRegionMin();
				ImVec2 max_space = ImGui::GetWindowContentRegionMax();

				float yOffset = Maths::Max(45.0f, ImGui::GetScrollY()); // Dont include search bar
				min_space.x += ImGui::GetWindowPos().x + 1.0f;
				min_space.y += ImGui::GetWindowPos().y + 1.0f + yOffset;
				max_space.x += ImGui::GetWindowPos().x - 1.0f;
				max_space.y += ImGui::GetWindowPos().y - 1.0f + ImGui::GetScrollY();
				ImRect bb{min_space, max_space};


				const ImGuiPayload* payload = ImGui::GetDragDropPayload();
				if(payload != NULL && payload->IsDataType("Drag_Entity"))
				{
					bool acceptable = false;

					LUMOS_ASSERT(payload->DataSize == sizeof(entt::entity*), "Error ImGUI drag entity");
					auto entity = *reinterpret_cast<entt::entity*>(payload->Data);
					auto hierarchyComponent = registry.try_get<Hierarchy>(entity);
					if(hierarchyComponent)
					{
						acceptable = hierarchyComponent->parent() != entt::null;
					}

					if(acceptable && ImGui::BeginDragDropTargetCustom(bb, ImGui::GetID("Panel Hierarchy")))
					{
						if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Drag_Entity"))
						{
							LUMOS_ASSERT(payload->DataSize == sizeof(entt::entity*), "Error ImGUI drag entity");
							auto entity = *reinterpret_cast<entt::entity*>(payload->Data);
							auto hierarchyComponent = registry.try_get<Hierarchy>(entity);
							if(hierarchyComponent)
							{
								Hierarchy::Reparent(entity, entt::null, registry, *hierarchyComponent);
                                Entity e(entity, scene);
                                e.RemoveComponent<Hierarchy>();
							}
						}
						ImGui::EndDragDropTarget();
					}
				}
			}
		}
		ImGui::End();
	}
}
