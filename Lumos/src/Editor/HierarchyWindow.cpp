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

	void HierarchyWindow::DrawNode(Entity* node)
	{
		if (node == nullptr)
			return;

		bool show = true;

		if (m_HierarchyFilter.IsActive())
		{
			if (!m_HierarchyFilter.PassFilter((node->GetName().c_str())))
			{
				show = false;
			}
		}

		if (show)
		{
			bool noChildren = node->GetChildren().empty();

			ImGuiTreeNodeFlags nodeFlags = ((m_Editor->GetSelected() == node) ? ImGuiTreeNodeFlags_Selected : 0);

			nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding;

			if (noChildren)
			{
				nodeFlags |= ImGuiTreeNodeFlags_Leaf;
			}

			String icon(ICON_FA_CUBE);
            
            std::stringstream ss;
			ss << "##";
			ss << node->GetUUID();
            
			bool active = node->ActiveInHierarchy();
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
				m_HadRecentDroppedEntity = nullptr;
			}

            bool nodeOpen = ImGui::TreeNodeEx(ss.str().c_str(), nodeFlags, doubleClicked ? (icon).c_str() :(icon + " " + node->GetName()).c_str(), 0);
			
			if (doubleClicked)
			{
				ImGui::SameLine();
				static char objName[INPUT_BUF_SIZE];
				strcpy(objName, node->GetName().c_str());

				ImGui::PushItemWidth(-1);
				if (ImGui::InputText("##Name", objName, IM_ARRAYSIZE(objName), 0))
					node->SetName(objName);
				ImGui::PopStyleVar();
			}

			if (!active)
				ImGui::PopStyleColor();

			if (!doubleClicked && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				auto ptr = node;
				ImGui::SetDragDropPayload("Drag_Entity", &ptr, sizeof(Entity**));
				ImGui::Text("Moving %s", node->GetName().c_str());
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Drag_Entity"))
				{
					LUMOS_ASSERT(payload->DataSize == sizeof(Entity**), "Error ImGUI drag entity");
					auto entity = *reinterpret_cast<Entity**>(payload->Data);
					node->AddChild(entity);

					m_HadRecentDroppedEntity = node;

					if (m_Editor->GetSelected() == entity)
						m_Editor->SetSelected(nullptr);
				}
				ImGui::EndDragDropTarget();
			}

			if (ImGui::IsItemClicked())
				m_Editor->SetSelected(node);
			else if (m_DoubleClicked == node && ImGui::IsMouseClicked(0) && !ImGui::IsItemHovered(ImGuiHoveredFlags_None))
				m_DoubleClicked = nullptr;

			if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered(ImGuiHoveredFlags_None))
				m_DoubleClicked = node;

			if (nodeOpen == false)
				return;

			for (auto child : node->GetChildren())
			{
				this->DrawNode(child);
			}

			ImGui::TreePop();

		}
		else
		{
			for (auto child : node->GetChildren())
			{
				this->DrawNode(child);
			}
		}
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
            
            //const ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);
            //const ImU32 bg = ImGui::GetColorU32(ImGuiCol_TextSelectedBg);

            //ImGui::NewLine();
            //ImGuiHelpers::Spinner("##spinner", 8, 6, col);
            //ImGuiHelpers::BufferingBar("##buffer_bar", 0.7f, Maths::Vector2(400, 6), bg, col);

            ImGui::Unindent();

			if (ImGui::TreeNode("Scene"))
			{
				ImGui::Indent();

				DrawNode(Application::Instance()->GetSceneManager()->GetCurrentScene()->GetRootEntity());

				ImGui::TreePop();
			}

			Application::Instance()->GetSceneManager()->GetCurrentScene()->OnImGui();
		}
		ImGui::End();
	}
}
