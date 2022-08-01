#include "FileBrowserPanel.h"
#include "Editor.h"
#include <Lumos/Core/StringUtilities.h>
#include <Lumos/Core/OS/FileSystem.h>
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>
#include <imgui/imgui.h>
#include <imgui/Plugins/ImFileBrowser.h>

namespace Lumos
{
    FileBrowserPanel::FileBrowserPanel()
    {
        m_Name       = "FileBrowserWindow";
        m_SimpleName = "FileBrowser";

        m_FileBrowser = new ImGui::FileBrowser(ImGuiFileBrowserFlags_CreateNewDir | ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_HideHiddenFiles);

        m_FileBrowser->SetTitle("File Browser");
        // m_FileBrowser->SetFileFilters({ ".sh" , ".h" });
        m_FileBrowser->SetLabels(ICON_MDI_FOLDER, ICON_MDI_FILE, ICON_MDI_FOLDER_OPEN);
        m_FileBrowser->Refresh();
    }

    FileBrowserPanel::~FileBrowserPanel()
    {
        delete m_FileBrowser;
    }

    void FileBrowserPanel::OnImGui()
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

    bool FileBrowserPanel::IsOpen()
    {
        return m_FileBrowser->IsOpened();
    }

    void FileBrowserPanel::SetCurrentPath(const std::string& path)
    {
        m_FileBrowser->SetPwd(path);
    }

    void FileBrowserPanel::Open()
    {
        m_FileBrowser->Open();
    }

    void FileBrowserPanel::SetOpenDirectory(bool value)
    {
        auto flags = m_FileBrowser->GetFlags();

        if(value)
        {
            flags |= ImGuiFileBrowserFlags_SelectDirectory;
        }
        else
        {
            flags &= ~(ImGuiFileBrowserFlags_SelectDirectory);
        }
        m_FileBrowser->SetFlags(flags);
    }

    void FileBrowserPanel::SetFileTypeFilters(const std::vector<const char*>& fileFilters)
    {
        m_FileBrowser->SetFileFilters(fileFilters);
    }

    void FileBrowserPanel::ClearFileTypeFilters()
    {
        m_FileBrowser->ClearFilters();
    }

    std::filesystem::path& FileBrowserPanel::GetPath()
    {
        return m_FileBrowser->GetPath();
    }

}
