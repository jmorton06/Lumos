#pragma once
#include "lmpch.h"
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
        void SetCallback(const std::function<void(const String&)>& callback) { m_Callback = callback; }
		
	private:
        std::function<void(const String&)> m_Callback;
        ImGui::FileBrowser* m_FileBrowser;
	};
}
