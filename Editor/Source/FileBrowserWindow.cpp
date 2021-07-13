#include "FileBrowserWindow.h"
#include "Editor.h"
#include <Lumos/Core/StringUtilities.h>
#include <Lumos/Core/OS/FileSystem.h>
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>
#include <imgui/imgui.h>
#include <imgui/plugins/ImFileBrowser.h>

namespace Lumos
{
    FileBrowserWindow::FileBrowserWindow()
    {
        m_Name = "FileBrowserWindow";
        m_SimpleName = "FileBrowser";

        m_FileBrowser = new ImGui::FileBrowser(ImGuiFileBrowserFlags_CreateNewDir | ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_NoModal | ImGuiFileBrowserFlags_HideHiddenFiles);
        m_FileBrowser->SetTitle("Test File Browser");
        //m_FileBrowser->SetFileFilters({ ".sh" , ".h" });
        m_FileBrowser->SetLabels(ICON_MDI_FOLDER, ICON_MDI_FILE, ICON_MDI_FOLDER_OPEN);
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
            std::string tempFilePath = m_FileBrowser->GetSelected().string();

            std::string filePath = Lumos::StringUtilities::BackSlashesToSlashes(tempFilePath);

            m_Callback(filePath);

            m_FileBrowser->ClearSelected();
        }
    }

    void FileBrowserWindow::SetCurrentPath(const std::string& path)
    {
        m_FileBrowser->SetPwd(path);
    }

    void FileBrowserWindow::Open()
    {
        m_FileBrowser->Open();
    }

    void FileBrowserWindow::SetOpenDirectory(bool value)
    {
        if(value)
        {
            auto flags = m_FileBrowser->GetFlags();
            flags |= ImGuiFileBrowserFlags_SelectDirectory;
            m_FileBrowser->SetFlags(ImGuiFileBrowserFlags_CreateNewDir | ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_NoModal | ImGuiFileBrowserFlags_SelectDirectory | ImGuiFileBrowserFlags_HideHiddenFiles);
        }
        else
        {
            auto flags = m_FileBrowser->GetFlags();
            flags &= ~(ImGuiFileBrowserFlags_SelectDirectory);
            m_FileBrowser->SetFlags(ImGuiFileBrowserFlags_CreateNewDir | ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_NoModal | ImGuiFileBrowserFlags_HideHiddenFiles);
        }
    }
}
