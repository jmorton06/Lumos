#pragma once

#include "EditorPanel.h"
#include <Lumos/Utilities/AssetManager.h>

#if __has_include(<filesystem>)
#include <filesystem>
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
#endif

namespace Lumos
{
    enum class FileType
    {
        Unknown = 0,
        Scene,
        Prefab,
        Script,
        Audio,
        Shader,
        Texture,
        Cubemap,
        Model,
        Material,
        Project,
        Ini,
        Font
    };

    struct DirectoryInformation
    {
        SharedPtr<DirectoryInformation> Parent;
        std::vector<SharedPtr<DirectoryInformation>> Children;

        std::filesystem::path FilePath;
        bool IsFile;

        std::string Name;
        std::string Extension;
        std::filesystem::directory_entry DirectoryEntry;
        SharedPtr<Graphics::Texture2D> Thumbnail = nullptr;

        ImVec4 FileTypeColour;
        FileType Type;
        std::string_view FileTypeString;
        std::string FileSize;

    public:
        DirectoryInformation(const std::filesystem::path& fname, bool isF)
        {
            FilePath = fname;
            IsFile   = isF;
        }

        ~DirectoryInformation()
        {
        }
    };

    class ResourcePanel : public EditorPanel
    {
    public:
        ResourcePanel();
        ~ResourcePanel()
        {
            m_TextureLibrary.Destroy();
        }

        void OnImGui() override;

        bool RenderFile(int dirIndex, bool folder, int shownIndex, bool gridView);
        void DrawFolder(const SharedPtr<DirectoryInformation>& dirInfo, bool defaultOpen = false);
        void RenderBreadCrumbs();
        void RenderBottom();
        // void GetDirectories(const std::string& path);

        void DestroyGraphicsResources() override
        {
            for(auto& dir : m_Directories)
            {
                if(dir.second)
                {
                    dir.second->Parent.reset();
                    dir.second->Children.clear();
                }
            }
            m_FolderIcon.reset();
            m_FileIcon.reset();
            m_Directories.clear();
            m_CurrentSelected.reset();
            m_CurrentDir.reset();
            m_BaseProjectDir.reset();
            m_NextDirectory.reset();
            m_PreviousDirectory.reset();
            m_BreadCrumbData.clear();
            m_TextureLibrary.Destroy();
        }

        int GetParsedAssetID(const std::string& extension)
        {
            for(int i = 0; i < assetTypes.size(); i++)
            {
                if(extension == assetTypes[i])
                {
                    return i;
                }
            }

            return -1;
        }

        static std::string GetParentPath(const std::string& path);

        static std::vector<std::string> SearchFiles(const std::string& query);
        static bool MoveFile(const std::string& filePath, const std::string& movePath);

        std::string StripExtras(const std::string& filename);
        std::string ProcessDirectory(const std::filesystem::path& directoryPath, const SharedPtr<DirectoryInformation>& parent);

        void ChangeDirectory(SharedPtr<DirectoryInformation>& directory);
        void RemoveDirectory(SharedPtr<DirectoryInformation>& directory, bool removeFromParent = true);
        void OnNewProject() override;
        void Refresh();

    private:
        static inline std::vector<std::string> assetTypes = {
            "fbx", "obj", "wav", "cs", "png", "blend", "lsc", "ogg", "lua"
        };

        std::string m_MovePath;
        std::string m_LastNavPath;
        static std::string m_Delimiter;

        size_t m_BasePathLen;
        bool m_IsDragging;
        bool m_IsInListView;
        bool m_UpdateBreadCrumbs;
        bool m_ShowHiddenFiles;
        int m_GridItemsPerRow;
        float m_GridSize = 360.0f;

        ImGuiTextFilter m_Filter;

        char* inputText;
        char* inputHint;
        char inputBuffer[1024];

        bool textureCreated = false;

        std::string m_BasePath;
        std::filesystem::path m_AssetPath;

        bool m_UpdateNavigationPath = true;

        SharedPtr<DirectoryInformation> m_CurrentDir;
        SharedPtr<DirectoryInformation> m_BaseProjectDir;
        SharedPtr<DirectoryInformation> m_NextDirectory;
        SharedPtr<DirectoryInformation> m_PreviousDirectory;
        std::unordered_map<std::string, SharedPtr<DirectoryInformation>> m_Directories;
        std::vector<SharedPtr<DirectoryInformation>> m_BreadCrumbData;
        SharedPtr<Graphics::Texture2D> m_FolderIcon;
        SharedPtr<Graphics::Texture2D> m_FileIcon;

        SharedPtr<DirectoryInformation> m_CurrentSelected;

        Lumos::TextureLibrary m_TextureLibrary;
        
        std::filesystem::path m_CopiedPath;
        bool m_CutFile = false;
    };
}
