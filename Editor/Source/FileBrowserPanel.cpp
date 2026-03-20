#include "FileBrowserPanel.h"
#include "Editor.h"
#include <Lumos/Utilities/StringUtilities.h>
#include <Lumos/Core/OS/FileSystem.h>
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>
#include <imgui/imgui.h>

#if defined(LUMOS_PLATFORM_MACOS)
#include "MacOSFileDialog.h"
#elif defined(LUMOS_PLATFORM_IOS)
#include "iOSFileDialog.h"
#else
#include <imgui/Plugins/ImFileBrowser.h>
#endif

#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS)
#define LUMOS_USE_NATIVE_FILE_DIALOG
#endif

namespace Lumos
{
    FileBrowserPanel::FileBrowserPanel()
    {
        m_Name       = "FileBrowserWindow";
        m_SimpleName = "FileBrowser";

#ifndef LUMOS_USE_NATIVE_FILE_DIALOG
        m_FileBrowser = new ImGui::FileBrowser(ImGuiFileBrowserFlags_CreateNewDir | ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_HideHiddenFiles);

        m_FileBrowser->SetTitle("File Browser");
        m_FileBrowser->SetLabels(ICON_MDI_FOLDER, ICON_MDI_FILE, ICON_MDI_FOLDER_OPEN);
        m_FileBrowser->Refresh();
#else
        m_FileBrowser = nullptr;
#endif
    }

    FileBrowserPanel::~FileBrowserPanel()
    {
#ifndef LUMOS_USE_NATIVE_FILE_DIALOG
        delete m_FileBrowser;
#endif
    }

    void FileBrowserPanel::OnImGui()
    {
#ifndef LUMOS_USE_NATIVE_FILE_DIALOG
        m_FileBrowser->Display();

        if(m_FileBrowser->HasSelected())
        {
            std::string tempFilePath = m_FileBrowser->GetSelected().string();

            std::string filePath = Lumos::StringUtilities::BackSlashesToSlashes(tempFilePath);

            m_Callback(filePath);

            m_FileBrowser->ClearSelected();
            auto flags = m_FileBrowser->GetFlags();
            flags &= ~(ImGuiFileBrowserFlags_SelectDirectory);
            m_FileBrowser->SetFlags(flags);
        }
#endif
    }

    bool FileBrowserPanel::IsOpen()
    {
#ifdef LUMOS_USE_NATIVE_FILE_DIALOG
        return false;
#else
        return m_FileBrowser->IsOpened();
#endif
    }

    void FileBrowserPanel::SetCurrentPath(const std::string& path)
    {
#ifdef LUMOS_USE_NATIVE_FILE_DIALOG
        m_CurrentPath = path;
#else
        m_FileBrowser->SetPwd(path);
#endif
    }

    void FileBrowserPanel::Open()
    {
#ifdef LUMOS_USE_NATIVE_FILE_DIALOG
        OpenNativeFileDialog(m_SelectDirectory, m_FileFilters, m_CurrentPath, m_Callback);
#else
        m_FileBrowser->Open();
#endif
    }

    void FileBrowserPanel::SetOpenDirectory(bool value)
    {
#ifdef LUMOS_USE_NATIVE_FILE_DIALOG
        m_SelectDirectory = value;
#else
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
#endif
    }

    void FileBrowserPanel::SetFileTypeFilters(const std::vector<const char*>& fileFilters)
    {
#ifdef LUMOS_USE_NATIVE_FILE_DIALOG
        m_FileFilters.clear();
        for(auto f : fileFilters)
            m_FileFilters.push_back(f);
#else
        m_FileBrowser->SetFileFilters(fileFilters);
#endif
    }

    void FileBrowserPanel::ClearFileTypeFilters()
    {
#ifdef LUMOS_USE_NATIVE_FILE_DIALOG
        m_FileFilters.clear();
#else
        m_FileBrowser->ClearFilters();
#endif
    }

    std::filesystem::path& FileBrowserPanel::GetPath()
    {
#ifdef LUMOS_USE_NATIVE_FILE_DIALOG
        return m_Path;
#else
        return m_FileBrowser->GetPath();
#endif
    }

}
