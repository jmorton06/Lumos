#pragma once

#include "EditorWindow.h"

namespace ImGui
{
	class FileBrowser;
}

namespace Lumos
{
	class FileBrowserWindow : public EditorWindow
	{
	public:
		FileBrowserWindow();
		~FileBrowserWindow();

		void Open();
		void OnImGui() override;
		void SetCallback(const std::function<void(const std::string&)>& callback)
		{
			m_Callback = callback;
		}

	private:
		std::function<void(const std::string&)> m_Callback;
		ImGui::FileBrowser* m_FileBrowser;
	};
}
