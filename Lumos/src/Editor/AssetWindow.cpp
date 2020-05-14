#include "lmpch.h"
#include "AssetWindow.h"
#include "Core/OS/FileSystem.h"
#include "Editor.h"

#include <imgui/imgui.h>

namespace Lumos
{
	AssetWindow::AssetWindow()
	{
		m_Name = "AssetWindow";
		m_SimpleName = "Assets";
	}

	void AssetWindow::OnImGui()
	{
        ImGui::Begin("Assets", nullptr, 0);
        ImGui::End();
	}
}
