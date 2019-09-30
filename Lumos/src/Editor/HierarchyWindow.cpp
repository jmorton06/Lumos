#include "lmpch.h"
#include "HierarchyWindow.h"
#include "Editor.h"
#include "ECS/EntityManager.h"
#include "App/Application.h"
#include "App/SceneManager.h"

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

			nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

			if (noChildren)
			{
				nodeFlags |= ImGuiTreeNodeFlags_Leaf;
			}

			String icon(ICON_FA_CUBE);
			bool nodeOpen = ImGui::TreeNodeEx("##" + node->GetUUID(), nodeFlags, (icon + " " + node->GetName()).c_str(), 0);

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
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

					if (m_Editor->GetSelected() == entity)
						m_Editor->SetSelected(nullptr);
				}
				ImGui::EndDragDropTarget();
			}

			if (ImGui::IsItemClicked())
				m_Editor->SetSelected(node);

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