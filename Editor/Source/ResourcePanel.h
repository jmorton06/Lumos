#pragma once

#include "EditorPanel.h"

#if __has_include(<filesystem>)
#include <filesystem>
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
#endif

namespace Lumos
{

    struct DirectoryInformation
    {
        SharedRef<DirectoryInformation> Parent;
        std::vector<SharedRef<DirectoryInformation>> Children;

        std::filesystem::path FilePath;
        bool IsFile;

    public:
        DirectoryInformation(const std::filesystem::path& fname, bool isF)
        {
            FilePath = fname;
            IsFile = isF;
        }
    };

    class ResourcePanel : public EditorPanel
    {
    public:
        ResourcePanel();
        ~ResourcePanel() = default;

        void OnImGui() override;

        bool RenderFile(int dirIndex, bool folder, int shownIndex, bool gridView);
        void DrawFolder(const SharedRef<DirectoryInformation>& dirInfo, bool defaultOpen = false);
        void RenderBreadCrumbs();
        void RenderBottom();
        void GetDirectories(const std::string& path);

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
        std::string ProcessDirectory(const std::filesystem::path& directoryPath, const SharedRef<DirectoryInformation>& parent);

        void ChangeDirectory(SharedRef<DirectoryInformation>& directory);
        void RemoveDirectory(SharedRef<DirectoryInformation>& directory, bool removeFromParent = true);

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
        float m_GridSize = 128.0f;

        ImGuiTextFilter m_Filter;

        char* inputText;
        char* inputHint;
        char inputBuffer[1024];

        std::string m_BasePath;

        bool m_UpdateNavigationPath = true;

        SharedRef<DirectoryInformation> m_CurrentDir;
        SharedRef<DirectoryInformation> m_BaseProjectDir;
        SharedRef<DirectoryInformation> m_NextDirectory;
        SharedRef<DirectoryInformation> m_PreviousDirectory;
        std::unordered_map<std::string, SharedRef<DirectoryInformation>> m_Directories;
        std::vector<SharedRef<DirectoryInformation>> m_BreadCrumbData;
    };
}
