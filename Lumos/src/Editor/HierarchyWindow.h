#pragma once
#include "lmpch.h"
#include "EditorWindow.h"
#include <imgui/imgui.h>

namespace Lumos
{
	class Entity;

	class HierarchyWindow : public EditorWindow
	{
	public:
		HierarchyWindow();
		~HierarchyWindow() = default;

		void DrawNode(Entity* node);
		void OnImGui() override;

	private:
		ImGuiTextFilter m_HierarchyFilter;
	};
}