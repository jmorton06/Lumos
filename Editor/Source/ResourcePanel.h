#pragma once

#include "EditorPanel.h"

namespace Lumos
{

    struct DirectoryInformation
    {
        std::string filename;
        std::string fileType;
        std::string absolutePath;
        bool isFile;

    public:
        DirectoryInformation(const std::string& fname, const std::string& ftype, const std::string& absPath, bool isF)
        {
            filename = fname;
            fileType = ftype;
            absolutePath = absPath;
            isFile = isF;
        }
    };

    class ResourcePanel : public EditorPanel
    {
    public:
        ResourcePanel();
        ~ResourcePanel() = default;

        void OnImGui() override;

        bool RenderFile(int dirIndex, bool folder, int shownIndex, bool gridView);
        void DrawFolder(const DirectoryInformation& dirInfo);
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

        static std::vector<DirectoryInformation> GetFsContents(const std::string& path);
        static std::vector<DirectoryInformation> ReadDirectory(const std::string& path);
        static std::vector<DirectoryInformation> ReadDirectoryRecursive(const std::string& path);
        static std::string GetParentPath(const std::string& path);

        static std::vector<std::string> SearchFiles(const std::string& query);
        static bool MoveFile(const std::string& filePath, const std::string& movePath);

        static std::string StripExtras(const std::string& filename);

    private:
        static inline std::vector<std::string> assetTypes = {
            "fbx", "obj", "wav", "cs", "png", "blend", "lsc", "ogg", "lua"
        };

        std::string m_CurrentDirPath;
        std::string m_BaseDirPath;
        std::string m_PreviousDirPath;
        std::string m_MovePath;
        std::string m_LastNavPath;
        static std::string m_Delimiter;

        std::string m_Directories[100];
        int m_DirectoryCount;

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

        std::vector<DirectoryInformation> m_CurrentDir;
        std::vector<DirectoryInformation> m_BaseProjectDir;
    };
}
