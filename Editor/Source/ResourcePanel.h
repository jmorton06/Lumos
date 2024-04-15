#pragma once

#include "EditorPanel.h"
#include <Lumos/Utilities/AssetManager.h>
#include <Lumos/Core/String.h>

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

        bool IsFile;
        bool Opened = false;
        bool Leaf   = true;

        String8 AssetPath;
        String8 Path;
        String8 ThumbnailPath;
        SharedPtr<Graphics::Texture2D> Thumbnail = nullptr;
        FileType Type;
        uint64_t FileSize;
        String8 FileSizeString;
        int64_t FileTypeID;
        bool Hidden;
        ImVec4 FileTypeColour;

    public:
        DirectoryInformation(String8 path, bool isF)
        {
            Path   = path;
            IsFile = isF;
            Hidden = false;
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
            ArenaRelease(m_Arena);
        }

        void OnImGui() override;

        bool RenderFile(int dirIndex, bool folder, int shownIndex, bool gridView);
        void DrawFolder(SharedPtr<DirectoryInformation>& dirInfo, bool defaultOpen = false);

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
        }

        int GetParsedAssetID(String8 extension)
        {
            for(int i = 0; i < assetTypes.size(); i++)
            {
                if(Str8Match(extension, assetTypes[i]))
                {
                    return i;
                }
            }

            return -1;
        }

        // static  String8 GetParentPath(String8 path);

        static bool MoveFile(String8 filePath, String8 movePath);

        // String8 StripExtras(String8& filename);
        String8 ProcessDirectory(String8 directoryPath, const SharedPtr<DirectoryInformation>& parent, bool processChildren);

        void ChangeDirectory(SharedPtr<DirectoryInformation>& directory);
        void RemoveDirectory(SharedPtr<DirectoryInformation>& directory, bool removeFromParent = true);
        void OnNewProject() override;
        void Refresh();
        void QueueRefresh() { m_Refresh = true; }

    private:
        static inline std::vector<String8> assetTypes = {
            Str8Lit("fbx"), Str8Lit("obj"), Str8Lit("wav"), Str8Lit("cs"), Str8Lit("png"), Str8Lit("blend"), Str8Lit("lsc"), Str8Lit("ogg"), Str8Lit("lua")
        };

        float MinGridSize = 50;
        float MaxGridSize = 400;
        String8 m_MovePath;
        String8 m_LastNavPath;
        String8 m_Delimiter;

        size_t m_BasePathLen;
        bool m_IsDragging;
        bool m_IsInListView;
        bool m_UpdateBreadCrumbs;
        bool m_ShowHiddenFiles;
        int m_GridItemsPerRow;
        float m_GridSize = 360.0f;

        ImGuiTextFilter m_Filter;

        bool textureCreated = false;

        String8 m_BasePath;
        String8 m_AssetPath;

        bool m_Refresh = false;

        bool m_UpdateNavigationPath = true;

        SharedPtr<DirectoryInformation> m_CurrentDir;
        SharedPtr<DirectoryInformation> m_BaseProjectDir;
        SharedPtr<DirectoryInformation> m_NextDirectory;
        SharedPtr<DirectoryInformation> m_PreviousDirectory;

        struct cmp_str
        {
            bool operator()(String8 a, String8 b) const
            {
                return Str8Match(a, b);
            }
        };

        struct String8Hash
        {
            std::size_t operator()(const String8& s) const
            {
                // Simple hash function that combines the bytes of the string
                std::size_t hash = 0;
                for(uint64_t i = 0; i < s.size; ++i)
                {
                    hash = hash * 31 + s.str[i];
                }
                return hash;
            }
        };

        std::unordered_map<String8, SharedPtr<DirectoryInformation>, String8Hash, cmp_str> m_Directories;
        std::vector<SharedPtr<DirectoryInformation>> m_BreadCrumbData;
        SharedPtr<Graphics::Texture2D> m_FolderIcon;
        SharedPtr<Graphics::Texture2D> m_FileIcon;

        SharedPtr<DirectoryInformation> m_CurrentSelected;

        String8 m_RequestedThumbnailPath;
        String8 m_CopiedPath;
        bool m_CutFile = false;

        Arena* m_Arena;
    };
}
