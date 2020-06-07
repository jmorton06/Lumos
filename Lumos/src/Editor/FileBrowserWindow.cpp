#include "lmpch.h"
#include "FileBrowserWindow.h"
#include "Core/OS/FileSystem.h"
#include "Editor.h"

#include <imgui/imgui.h>
#include <imgui/plugins/ImFileBrowser.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>

namespace Lumos
{
	FileBrowserWindow::FileBrowserWindow()
	{
		m_Name = "FileBrowserWindow";
		m_SimpleName = "FileBrowser";

        m_FileBrowser = lmnew ImGui::FileBrowser(ImGuiFileBrowserFlags_CreateNewDir | ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_NoModal);
		m_FileBrowser->SetTitle("Test File Browser");
		//m_FileBrowser->SetFileFilters({ ".sh" , ".h" });
		m_FileBrowser->SetLabels(ICON_FA_FOLDER, ICON_FA_FILE, ICON_FA_FOLDER_OPEN);
		m_FileBrowser->Refresh();

	}

    FileBrowserWindow::~FileBrowserWindow()
    {
        delete m_FileBrowser;
    }

	void FileBrowserWindow::OnImGui()
	{
        m_FileBrowser->Display();

        if(m_FileBrowser->HasSelected())
        {
            String tempFilePath = m_FileBrowser->GetSelected().string();
    
            String filePath = Lumos::BackSlashesToSlashes(tempFilePath);

            m_Callback(filePath);

            m_FileBrowser->ClearSelected();
        }
    }

    void FileBrowserWindow::Open()
    {
        m_FileBrowser->Open();
    }
}
