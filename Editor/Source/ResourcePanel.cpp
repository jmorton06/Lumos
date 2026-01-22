#include "Editor.h"
#include "ResourcePanel.h"
#include <Lumos/Core/OS/Input.h>
#include <Lumos/Graphics/RHI/Texture.h>
#include <Lumos/Graphics/RHI/IMGUIRenderer.h>
#include <Lumos/Core/Profiler.h>
#include <Lumos/Utilities/StringUtilities.h>
#include <Lumos/Core/OS/Window.h>
#include <Lumos/Graphics/Material.h>
#include <Lumos/Maths/MathsUtilities.h>
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <Lumos/Embedded/browserFile.inl>
#include <Lumos/Embedded/browserFolder.inl>
#include <Lumos/Core/OS/OS.h>
#include <Lumos/Core/String.h>
#include <Lumos/ImGui/ImGuiManager.h>
#include <Lumos/Core/Thread.h>
#include <Lumos/Core/Asset/AssetManager.h>

#ifdef LUMOS_PLATFORM_WINDOWS
#include <Shellapi.h>
#endif

#include "Core/OS/FileSystem.h"

namespace Lumos
{

    static const std::unordered_map<FileType, String8> s_FileTypesToString = {
        { FileType::Unknown, Str8Lit("Unknown") },
        { FileType::Scene, Str8Lit("Scene") },
        { FileType::Prefab, Str8Lit("Prefab") },
        { FileType::Script, Str8Lit("Script") },
        { FileType::Shader, Str8Lit("Shader") },
        { FileType::Texture, Str8Lit("Texture") },
        { FileType::Font, Str8Lit("Font") },
        { FileType::Cubemap, Str8Lit("Cubemap") },
        { FileType::Model, Str8Lit("Model") },
        { FileType::Audio, Str8Lit("Audio") },
        { FileType::Material, Str8Lit("Material") },
    };

    static const std::unordered_map<std::string, FileType> s_FileTypes = {
        { "lsn", FileType::Scene },
        { "lprefab", FileType::Prefab },
        { "cs", FileType::Script },
        { "lua", FileType::Script },
        { "glsl", FileType::Shader },
        { "shader", FileType::Shader },
        { "frag", FileType::Shader },
        { "vert", FileType::Shader },
        { "comp", FileType::Shader },
        { "png", FileType::Texture },
        { "jpg", FileType::Texture },
        { "jpeg", FileType::Texture },
        { "bmp", FileType::Texture },
        { "gif", FileType::Texture },
        { "tga", FileType::Texture },
        { "ttf", FileType::Font },
        { "hdr", FileType::Cubemap },
        { "obj", FileType::Model },
        { "fbx", FileType::Model },
        { "gltf", FileType::Model },
        { "glb", FileType::Model },
        { "mp3", FileType::Audio },
        { "m4a", FileType::Audio },
        { "wav", FileType::Audio },
        { "ogg", FileType::Audio },
        { "lmat", FileType::Material },
    };

    static const std::unordered_map<FileType, ImVec4> s_TypeColors = {
        { FileType::Scene, { 0.8f, 0.4f, 0.22f, 1.00f } },
        { FileType::Prefab, { 0.10f, 0.50f, 0.80f, 1.00f } },
        { FileType::Script, { 0.10f, 0.50f, 0.80f, 1.00f } },
        { FileType::Font, { 0.60f, 0.19f, 0.32f, 1.00f } },
        { FileType::Shader, { 0.10f, 0.50f, 0.80f, 1.00f } },
        { FileType::Texture, { 0.82f, 0.20f, 0.33f, 1.00f } },
        { FileType::Cubemap, { 0.82f, 0.18f, 0.30f, 1.00f } },
        { FileType::Model, { 0.18f, 0.82f, 0.76f, 1.00f } },
        { FileType::Audio, { 0.20f, 0.80f, 0.50f, 1.00f } },
    };

    static const std::unordered_map<FileType, const char*> s_FileTypesToIcon = {
        { FileType::Unknown, ICON_MDI_FILE },
        { FileType::Scene, ICON_MDI_FILE },
        { FileType::Prefab, ICON_MDI_FILE },
        { FileType::Script, ICON_MDI_LANGUAGE_LUA },
        { FileType::Shader, ICON_MDI_IMAGE_FILTER_BLACK_WHITE },
        { FileType::Texture, ICON_MDI_FILE_IMAGE },
        { FileType::Font, ICON_MDI_CARD_TEXT },
        { FileType::Cubemap, ICON_MDI_IMAGE_FILTER_HDR },
        { FileType::Model, ICON_MDI_VECTOR_POLYGON },
        { FileType::Audio, ICON_MDI_MICROPHONE },
    };

    ResourcePanel::ResourcePanel()
    {
        LUMOS_PROFILE_FUNCTION();
        m_Name       = ICON_MDI_FOLDER_STAR " Resources###resources";
        m_SimpleName = "Resources";

        m_Arena = ArenaAlloc(Megabytes(8));
#ifdef LUMOS_PLATFORM_WINDOWS
        m_Delimiter = Str8Lit("\\");
#else
        m_Delimiter = Str8Lit("/");
#endif

        float dpi  = Application::Get().GetWindow()->GetDPIScale();
        m_GridSize = 120.0f;
        m_GridSize *= dpi;
        MinGridSize *= dpi;
        MaxGridSize *= dpi;
        m_BasePath = PushStr8F(m_Arena, "%sAssets", Application::Get().GetProjectSettings().m_ProjectRoot.c_str());

        String8 assetsBasePath;
        FileSystem::Get().ResolvePhysicalPath(m_Arena, Str8Lit("//Assets"), &assetsBasePath);
        m_AssetPath = PushStr8Copy(m_Arena, Str8C((char*)std::filesystem::path((const char*)assetsBasePath.str).string().c_str()));

        String8 baseDirectoryHandle = ProcessDirectory(m_BasePath, nullptr, true);
        m_BaseProjectDir            = m_Directories[baseDirectoryHandle];
        ChangeDirectory(m_BaseProjectDir);

        m_CurrentDir = m_BaseProjectDir;

        m_UpdateNavigationPath = true;
        m_IsDragging           = false;
        m_IsInListView         = false;
        m_UpdateBreadCrumbs    = true;
        m_ShowHiddenFiles      = false;

        Graphics::TextureDesc desc;
        desc.minFilter = Graphics::TextureFilter::LINEAR;
        desc.magFilter = Graphics::TextureFilter::LINEAR;
        desc.wrap      = Graphics::TextureWrap::CLAMP;

        Graphics::TextureLoadOptions options;
        options.flipY = true;
        m_FileIcon    = Graphics::Texture2D::CreateFromSource(browserFileWidth, browserFileHeight, (void*)browserFile, desc, options);
        m_FolderIcon  = Graphics::Texture2D::CreateFromSource(browserFolderWidth, browserFolderHeight, (void*)browserFolder, desc, options);
        m_Refresh     = false;
    }

    void ResourcePanel::ChangeDirectory(DirectoryInformation* directory)
    {
        if(!directory)
            return;

        m_PreviousDirectory    = m_CurrentDir;
        m_CurrentDir           = directory;
        m_UpdateNavigationPath = true;

        if(!m_CurrentDir->Opened)
        {
            ProcessDirectory(m_CurrentDir->AssetPath, m_CurrentDir->Parent, true);
        }
    }

    void ResourcePanel::RemoveDirectory(DirectoryInformation* directory, bool removeFromParent)
    {
        if(directory->Parent && removeFromParent)
        {
            directory->Parent->Children.Clear();
        }

        for(auto& subdir : directory->Children)
            RemoveDirectory(subdir, false);

        m_Directories.erase(m_Directories.find(directory->AssetPath));
    }

    bool IsHidden(const std::filesystem::path& filePath)
    {
        try
        {
            std::filesystem::file_status status = std::filesystem::status(filePath);
            std::string name                    = filePath.stem().string();
            return (status.permissions() & std::filesystem::perms::owner_read) == std::filesystem::perms::none || name == ".DS_Store";
        }
        catch(const std::filesystem::filesystem_error& ex)
        {
            LERROR("Error accessing file: %s", ex.what());
        }

        return false; // Return false by default if any error occurs
    }

    String8 ResourcePanel::ProcessDirectory(String8 directoryPath, DirectoryInformation* parent, bool processChildren)
    {
        const auto& directory = m_Directories[directoryPath];
        if(directory && directory->Opened)
            return directory->AssetPath;

        ArenaTemp temp       = ScratchBegin(&m_Arena, 1);
        String8 absolutePath = StringUtilities::RelativeToAbsolutePath(temp.arena, directoryPath, Str8Lit("//Assets"), m_BasePath);
        auto stdPath         = std::filesystem::path(std::string((const char*)absolutePath.str, absolutePath.size));

        SharedPtr<DirectoryInformation> directoryInfo = directory ? directory : CreateSharedPtr<DirectoryInformation>(directoryPath, !std::filesystem::is_directory(stdPath));
        directoryInfo->Parent                         = parent;

        // TODO: create paths at max size and use free list
        directoryInfo->AssetPath = StringUtilities::AbsolutePathToRelativeFileSystemPath(m_Arena, directoryPath, m_BasePath, Str8Lit("//Assets"));

        String8 extension = StringUtilities::Str8PathSkipLastPeriod(directoryInfo->AssetPath);

        ScratchEnd(temp);

        if(std::filesystem::is_directory(stdPath))
        {
            directoryInfo->IsFile = false;
            directoryInfo->Leaf   = true;
            for(auto& entry : std::filesystem::directory_iterator(stdPath))
            {
                if(!m_ShowHiddenFiles && IsHidden(entry.path()))
                {
                    continue;
                }

                if(Str8Match(directoryInfo->AssetPath, Str8Lit("//Assets/Cache")))
                {
                    directoryInfo->Hidden = true;
                    continue;
                }

                if(entry.is_directory())
                    directoryInfo->Leaf = false;

                if(processChildren)
                {
                    directoryInfo->Opened = true;

                    String8 subdirHandle = ProcessDirectory(PushStr8Copy(m_Arena, Str8C((char*)entry.path().generic_string().c_str())), directoryInfo, false);
                    directoryInfo->Children.PushBack(m_Directories[subdirHandle].get());
                }
            }
        }
        else
        {
            auto fileType          = FileType::Unknown;
            const auto& fileTypeIt = s_FileTypes.find(std::string((const char*)extension.str, extension.size));
            if(fileTypeIt != s_FileTypes.end())
                fileType = fileTypeIt->second;

            directoryInfo->IsFile   = true;
            directoryInfo->Type     = fileType;
            directoryInfo->FileSize = std::filesystem::exists(stdPath) ? std::filesystem::file_size(stdPath) : 0;
            directoryInfo->Hidden   = std::filesystem::exists(stdPath) ? IsHidden(stdPath) : true;
            directoryInfo->Opened   = true;
            directoryInfo->Leaf     = true;

            ImVec4 fileTypeColor        = { 1.0f, 1.0f, 1.0f, 1.0f };
            const auto& fileTypeColorIt = s_TypeColors.find(fileType);
            if(fileTypeColorIt != s_TypeColors.end())
                fileTypeColor = fileTypeColorIt->second;

            directoryInfo->FileTypeColour = fileTypeColor;
        }

        if(!directory)
            m_Directories[directoryInfo->AssetPath] = directoryInfo;
        return directoryInfo->AssetPath;
    }

    void ResourcePanel::DrawFolder(DirectoryInformation* dirInfo, bool defaultOpen)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGuiTreeNodeFlags nodeFlags = ((dirInfo == m_CurrentDir) ? ImGuiTreeNodeFlags_Selected : 0);
        nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

        if(dirInfo->Parent == nullptr)
            nodeFlags |= ImGuiTreeNodeFlags_Framed;

        const ImColor TreeLineColor = ImColor(128, 128, 128, 128);
        const float SmallOffsetX    = 6.0f * Application::Get().GetWindowDPI();
        ImDrawList* drawList        = ImGui::GetWindowDrawList();

        if(!dirInfo->IsFile)
        {
            if(dirInfo->Leaf)
                nodeFlags |= ImGuiTreeNodeFlags_Leaf;

            if(defaultOpen)
                nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf;

            nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_SpanAvailWidth;

            bool isOpen = ImGui::TreeNodeEx((void*)(intptr_t)(dirInfo), nodeFlags, "");
            if(ImGui::IsItemClicked())
            {
                ChangeDirectory(dirInfo);
            }

            const char* folderIcon = ((isOpen && !dirInfo->Leaf) || m_CurrentDir == dirInfo) ? ICON_MDI_FOLDER_OPEN : ICON_MDI_FOLDER;
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtilities::GetIconColour());
            ImGui::TextUnformatted(folderIcon);
            ImGui::PopStyleColor();
            ImGui::SameLine();

            ImGui::TextUnformatted((const char*)(StringUtilities::Str8PathSkipLastSlash(dirInfo->AssetPath).str));

            ImVec2 verticalLineStart = ImGui::GetCursorScreenPos();

            if(isOpen && !dirInfo->Leaf)
            {
                verticalLineStart.x += SmallOffsetX; // to nicely line up with the arrow symbol
                ImVec2 verticalLineEnd = verticalLineStart;

                for(int i = 0; i < dirInfo->Children.Size(); i++)
                {
                    if(!m_ShowHiddenFiles && dirInfo->Children[i]->Hidden)
                    {
                        continue;
                    }

                    if(!dirInfo->Children[i]->IsFile)
                    {
                        auto currentPos = ImGui::GetCursorScreenPos();

                        ImGui::Indent(10.0f);

                        float HorizontalTreeLineSize = 16.0f * Application::Get().GetWindowDPI(); // chosen arbitrarily

                        if(!dirInfo->Children[i]->Leaf)
                            HorizontalTreeLineSize *= 0.5f;
                        DrawFolder(dirInfo->Children[i]);

                        const ImRect childRect = ImRect(currentPos, currentPos + ImVec2(0.0f, ImGui::GetFontSize()));

                        const float midpoint = (childRect.Min.y + childRect.Max.y) * 0.5f;
                        drawList->AddLine(ImVec2(verticalLineStart.x, midpoint), ImVec2(verticalLineStart.x + HorizontalTreeLineSize, midpoint), TreeLineColor);
                        verticalLineEnd.y = midpoint;

                        ImGui::Unindent(10.0f);
                    }
                }

                drawList->AddLine(verticalLineStart, verticalLineEnd, TreeLineColor);

                ImGui::TreePop();
            }

            if(isOpen && dirInfo->Leaf)
                ImGui::TreePop();
        }

        if(m_IsDragging && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        {
            m_MovePath = dirInfo->AssetPath;
        }
    }

    static int FileIndex = 0;

    Vec2 GetAspectCorrectedSize(const Vec2& originalSize, float maxSize)
    {
        float aspect = originalSize.x / originalSize.y;
        if(aspect > 1.0f)
            return { maxSize, maxSize / aspect }; // Wider than tall
        else
            return { maxSize * aspect, maxSize }; // Taller than wide or square
    }

    void ResourcePanel::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();

        if(ImGui::Begin(m_Name.c_str(), &m_Active))
        {
            FileIndex        = 0;
            auto windowSize  = ImGui::GetWindowSize();
            bool vertical    = windowSize.y > windowSize.x;
            static bool Init = false;

            if(m_Refresh)
            {
                Refresh();
                m_Refresh = false;
            }

            if(!vertical)
            {
                ImGui::BeginColumns("ResourcePanelColumns", 2, 0);
                if(!Init)
                {
                    ImGui::SetColumnWidth(0, ImGui::GetWindowContentRegionMax().x / 3.0f);
                    Init = true;
                }
                ImGui::BeginChild("##folders_common");
            }
            else
                ImGui::BeginChild("##folders_common", ImVec2(0, ImGui::GetWindowHeight() / 3.0f));

            {
                {
                    ImGui::BeginChild("##folders");
                    {
                        DrawFolder(m_BaseProjectDir, true);
                    }
                    ImGui::EndChild();
                }
            }

            ImGui::EndChild();

            if(ImGui::BeginDragDropTarget())
            {
                auto data = ImGui::AcceptDragDropPayload("selectable", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
                if(data)
                {
                    String8* file = (String8*)data->Data;
                    if(MoveFile(*file, m_MovePath))
                    {
                        LINFO("Moved File: %s to %s", (const char*)((*file).str), (const char*)m_MovePath.str);
                    }
                    m_IsDragging = false;
                }
                ImGui::EndDragDropTarget();
            }
            float offset = 0.0f;
            if(!vertical)
            {
                ImGui::NextColumn();
            }
            else
            {
                offset = ImGui::GetWindowHeight() / 3.0f + 6.0f;
                ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
            }

            {
                {
                    ImGui::BeginChild("##directory_breadcrumbs", ImVec2(ImGui::GetColumnWidth(), ImGui::GetFrameHeightWithSpacing() * 2.0f));

                    ImGui::AlignTextToFramePadding();
                    // Button for advanced settings
                    {
                        ImGuiUtilities::ScopedColour buttonColour(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                        if(ImGui::Button(ICON_MDI_COGS))
                            ImGui::OpenPopup("SettingsPopup");
                    }
                    if(ImGui::BeginPopup("SettingsPopup"))
                    {
                        if(m_IsInListView)
                        {
                            if(ImGui::Button(ICON_MDI_VIEW_LIST " Switch to Grid View"))
                            {
                                m_IsInListView = !m_IsInListView;
                            }
                        }
                        else
                        {
                            if(ImGui::Button(ICON_MDI_VIEW_GRID " Switch to List View"))
                            {
                                m_IsInListView = !m_IsInListView;
                            }
                        }

                        if(ImGui::Selectable("Refresh"))
                        {
                            QueueRefresh();
                        }

                        if(ImGui::Selectable("New folder"))
                        {
                            ArenaTemp temp   = ScratchBegin(&m_Arena, 1);
                            String8 fullPath = StringUtilities::RelativeToAbsolutePath(temp.arena, m_CurrentDir->AssetPath, Str8Lit("//Assets"), m_BasePath);
                            std::filesystem::create_directory(std::filesystem::path(std::string((char*)fullPath.str, fullPath.size)) / "NewFolder");
                            ScratchEnd(temp);
                            QueueRefresh();
                        }

                        if(!m_IsInListView)
                        {
                            ImGui::SliderFloat("##GridSize", &m_GridSize, 40.0f, 400.0f);
                        }

                        ImGui::EndPopup();
                    }
                    ImGui::SameLine();

                    ImGui::TextUnformatted(ICON_MDI_MAGNIFY);
                    ImGui::SameLine();

                    m_Filter.Draw("##Filter", ImGui::GetContentRegionAvail().x - ImGui::GetStyle().IndentSpacing);

                    if(ImGui::Button(ICON_MDI_ARROW_LEFT))
                    {
                        if(m_CurrentDir != m_BaseProjectDir)
                        {
                            ChangeDirectory(m_CurrentDir->Parent);
                        }
                    }
                    ImGui::SameLine();
                    if(ImGui::Button(ICON_MDI_ARROW_RIGHT))
                    {
                        m_PreviousDirectory = m_CurrentDir;
                        // m_CurrentDir = m_LastNavPath;
                        m_UpdateNavigationPath = true;
                    }
                    ImGui::SameLine();

                    if(m_UpdateNavigationPath)
                    {
                        m_BreadCrumbData.Clear();
                        auto current = m_CurrentDir;
                        while(current)
                        {
                            if(current->Parent != nullptr)
                            {
                                m_BreadCrumbData.PushBack(current);
                                current = current->Parent;
                            }
                            else
                            {
                                m_BreadCrumbData.PushBack(m_BaseProjectDir);
                                current = nullptr;
                            }
                        }

                        for(u32 i = 0; i < (u32)m_BreadCrumbData.Size() / 2; i++)
                        {
                            Swap(m_BreadCrumbData[i], m_BreadCrumbData[m_BreadCrumbData.Size() - i - 1]);
                        }

                        m_UpdateNavigationPath = false;
                    }
                    {
                        int secIdx = 0, newPwdLastSecIdx = -1;
                        int dirIndex = 0;
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.2f, 0.7f, 0.0f));

                        for(auto& directory : m_BreadCrumbData)
                        {
                            if(ImGui::SmallButton((const char*)StringUtilities::GetFileName(directory->AssetPath).str))
                                ChangeDirectory(directory);

                            ImGui::SameLine();
                        }

                        ImGui::PopStyleColor();

                        if(newPwdLastSecIdx >= 0)
                        {
                            int i = 0;

                            ArenaTemp temp      = ScratchBegin(&m_Arena, 1);
                            String8 currentPath = StringUtilities::RelativeToAbsolutePath(temp.arena, m_CurrentDir->AssetPath, Str8Lit("//Assets"), m_BasePath);
                            auto stdPath        = std::filesystem::path(std::string((const char*)currentPath.str, currentPath.size));
                            ScratchEnd(temp);

                            for(const auto& sec : stdPath)
                            {
                                if(i++ > newPwdLastSecIdx)
                                    break;
                                stdPath /= sec;
                            }
#ifdef _WIN32
                            if(newPwdLastSecIdx == 0)
                                stdPath /= "\\";
#endif

                            m_PreviousDirectory    = m_CurrentDir;
                            m_CurrentDir           = m_Directories[Str8C((char*)stdPath.string().c_str())];
                            m_UpdateNavigationPath = true;
                        }

                        ImGui::SameLine();
                    }
                    ImGui::EndChild();
                }

                {
                    int shownIndex = 0;

                    float xAvail = ImGui::GetContentRegionAvail().x;

                    constexpr float padding          = 4.0f;
                    const float scaledThumbnailSize  = m_GridSize * ImGui::GetIO().FontGlobalScale;
                    const float scaledThumbnailSizeX = scaledThumbnailSize * 0.55f;
                    const float cellSize             = scaledThumbnailSizeX + 2 * padding + scaledThumbnailSizeX * 0.1f;

                    constexpr float overlayPaddingY  = 6.0f * padding;
                    constexpr float thumbnailPadding = overlayPaddingY * 0.5f;
                    const float thumbnailSize        = scaledThumbnailSizeX - thumbnailPadding;

                    const ImVec2 backgroundThumbnailSize = { scaledThumbnailSizeX + padding * 2, scaledThumbnailSize + padding * 2 };

                    const float panelWidth = ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ScrollbarSize;
                    int columnCount        = static_cast<int>(panelWidth / cellSize);
                    if(columnCount < 1)
                        columnCount = 1;

                    float lineHeight = ImGui::GetTextLineHeight();
                    int flags        = ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_ScrollY;

                    if(m_IsInListView)
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 0, 0 });
                        columnCount = 1;
                        flags |= ImGuiTableFlags_RowBg
                            | ImGuiTableFlags_NoPadOuterX
                            | ImGuiTableFlags_NoPadInnerX
                            | ImGuiTableFlags_SizingStretchSame;
                    }
                    else
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { scaledThumbnailSizeX * 0.05f, scaledThumbnailSizeX * 0.05f });
                        flags |= ImGuiTableFlags_PadOuterX | ImGuiTableFlags_SizingFixedFit;
                    }

                    ImVec2 cursorPos    = ImGui::GetCursorPos();
                    const ImVec2 region = ImGui::GetContentRegionAvail();
                    ImGui::InvisibleButton("##DragDropTargetAssetPanelBody", region);

                    ImGui::SetNextItemAllowOverlap();
                    ImGui::SetCursorPos(cursorPos);

                    if(ImGui::BeginTable("BodyTable", columnCount, flags))
                    {
                        if(ImGui::IsWindowHovered() && (Input::Get().GetKeyHeld(InputCode::Key::LeftSuper) || Input::Get().GetKeyHeld(InputCode::Key::LeftControl)))
                        {
                            float mouseScroll = Input::Get().GetScrollOffset();

                            if(m_IsInListView)
                            {
                                if(mouseScroll > 0)
                                    m_IsInListView = false;
                            }
                            else
                            {
                                m_GridSize += mouseScroll;
                                if(m_GridSize < MinGridSize)
                                    m_IsInListView = true;
                            }

                            m_GridSize = Maths::Clamp(m_GridSize, MinGridSize, MaxGridSize);
                        }

                        m_GridItemsPerRow = (int)floor(xAvail / (m_GridSize + ImGui::GetStyle().ItemSpacing.x));
                        m_GridItemsPerRow = Maths::Max(1, m_GridItemsPerRow);

                        textureCreated = false;

                        ImGuiUtilities::PushID();

                        if(m_IsInListView)
                        {
                            for(int i = 0; i < m_CurrentDir->Children.Size(); i++)
                            {
                                if(!m_ShowHiddenFiles && m_CurrentDir->Children[i]->Hidden)
                                {
                                    continue;
                                }

                                if(m_Filter.IsActive())
                                {
                                    if(!m_Filter.PassFilter((const char*)(StringUtilities::Str8PathSkipLastSlash(m_CurrentDir->Children[i]->AssetPath).str)))
                                    {
                                        continue;
                                    }
                                }

                                ImGui::TableNextColumn();
                                bool doubleClicked = RenderFile(i, !m_CurrentDir->Children[i]->IsFile, shownIndex, !m_IsInListView);

                                if(doubleClicked)
                                    break;
                                shownIndex++;
                            }
                        }
                        else
                        {
                            for(int i = 0; i < m_CurrentDir->Children.Size(); i++)
                            {
                                if(!m_ShowHiddenFiles && m_CurrentDir->Children[i]->Hidden)
                                {
                                    continue;
                                }

                                if(m_Filter.IsActive())
                                {
                                    if(!m_Filter.PassFilter((const char*)(StringUtilities::Str8PathSkipLastSlash(m_CurrentDir->Children[i]->AssetPath).str)))
                                    {
                                        continue;
                                    }
                                }

                                ImGui::TableNextColumn();
                                bool doubleClicked = RenderFile(i, !m_CurrentDir->Children[i]->IsFile, shownIndex, !m_IsInListView);

                                if(doubleClicked)
                                    break;
                                shownIndex++;
                            }
                        }

                        ImGuiUtilities::PopID();

                        if(ImGui::BeginPopupContextWindow("AssetPanelHierarchyContextWindow", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
                        {
                            if(std::filesystem::exists(ToStdString(m_CopiedPath)) && ImGui::Selectable("Paste"))
                            {
                                if(m_CutFile)
                                {
                                    const std::filesystem::path& fullPath = ToStdString(m_CopiedPath);
                                    std::string filename                  = fullPath.stem().string();
                                    std::string extension                 = fullPath.extension().string();

                                    ArenaTemp temp      = ScratchBegin(&m_Arena, 1);
                                    String8 currentPath = StringUtilities::RelativeToAbsolutePath(temp.arena, m_CurrentDir->AssetPath, Str8Lit("//Assets"), m_BasePath);
                                    auto stdPath        = std::filesystem::path(std::string((const char*)currentPath.str, currentPath.size));
                                    ScratchEnd(temp);

                                    std::filesystem::path destinationPath = stdPath / (filename + extension);

                                    {
                                        while(std::filesystem::exists(destinationPath))
                                        {
                                            filename += "_copy";
                                            destinationPath = destinationPath.parent_path() / (filename + extension);
                                        }
                                    }
                                    std::filesystem::rename(fullPath, destinationPath);
                                }
                                else
                                {
                                    const std::filesystem::path& fullPath = ToStdString(m_CopiedPath);
                                    std::string filename                  = fullPath.stem().string();
                                    std::string extension                 = fullPath.extension().string();

                                    ArenaTemp temp      = ScratchBegin(&m_Arena, 1);
                                    String8 currentPath = StringUtilities::RelativeToAbsolutePath(temp.arena, m_CurrentDir->AssetPath, Str8Lit("//Assets"), m_BasePath);

                                    std::filesystem::path destinationPath = std::filesystem::path(ToStdString(currentPath)) / (filename + extension);
                                    {
                                        while(std::filesystem::exists(destinationPath))
                                        {
                                            filename += "_copy";
                                            destinationPath = destinationPath.parent_path() / (filename + extension);
                                        }
                                    }
                                    std::filesystem::copy(fullPath, destinationPath);

                                    ScratchEnd(temp);
                                }
                                m_CopiedPath.str  = nullptr;
                                m_CopiedPath.size = 0;

                                m_CutFile = false;
                                Refresh();
                            }

                            if(ImGui::Selectable("Open Location"))
                            {
                                Lumos::OS::Get().OpenFileLocation(ToStdString(m_CurrentDir->AssetPath));
                            }

                            ImGui::Separator();

                            if(ImGui::Selectable("Import New Asset"))
                            {
                                m_Editor->OpenFile();
                            }

                            if(ImGui::Selectable("Refresh"))
                            {
                                Refresh();
                            }

                            if(ImGui::Selectable("New folder"))
                            {
                                ArenaTemp temp      = ScratchBegin(&m_Arena, 1);
                                String8 currentPath = StringUtilities::RelativeToAbsolutePath(temp.arena, m_CurrentDir->AssetPath, Str8Lit("//Assets"), m_BasePath);
                                std::filesystem::create_directory(std::filesystem::path(ToStdString(currentPath)));
                                ScratchEnd(temp);

                                Refresh();
                            }

                            if(!m_IsInListView)
                            {
                                ImGui::SliderFloat("##GridSize", &m_GridSize, MinGridSize, MaxGridSize);
                            }
                            ImGui::EndPopup();
                        }

                        ImGui::EndTable();
                    }
                    ImGui::PopStyleVar();
                }
            }

            if(ImGui::BeginDragDropTarget())
            {
                auto data = ImGui::AcceptDragDropPayload("selectable", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
                if(data)
                {
                    String8* a = (String8*)data->Data;
                    if(MoveFile(*a, m_MovePath))
                    {
                        LINFO("Moved File: %s to %s", (const char*)((*a).str), (const char*)m_MovePath.str);
                    }
                    m_IsDragging = false;
                }
                ImGui::EndDragDropTarget();
            }
        }
        ImGui::End();
    }

    bool ResourcePanel::RenderFile(int dirIndex, bool folder, int shownIndex, bool gridView)
    {
        LUMOS_PROFILE_FUNCTION();
        constexpr float padding          = 4.0f;
        const float scaledThumbnailSize  = m_GridSize * ImGui::GetIO().FontGlobalScale;
        const float scaledThumbnailSizeX = scaledThumbnailSize * 0.55f;
        const float cellSize             = scaledThumbnailSizeX + 2 * padding + scaledThumbnailSizeX * 0.1f;

        constexpr float overlayPaddingY  = 6.0f * padding;
        constexpr float thumbnailPadding = overlayPaddingY * 0.5f;
        const float thumbnailSize        = scaledThumbnailSizeX - thumbnailPadding;

        const ImVec2 backgroundThumbnailSize = { scaledThumbnailSizeX + padding * 2, scaledThumbnailSize + padding * 2 };

        const float panelWidth = ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ScrollbarSize;
        int columnCount        = static_cast<int>(panelWidth / cellSize);
        if(columnCount < 1)
            columnCount = 1;

        bool doubleClicked = false;

        if(gridView)
        {
            auto& CurrentEnty              = m_CurrentDir->Children[dirIndex];
            Graphics::Texture2D* textureId = m_FolderIcon;

            auto cursorPos = ImGui::GetCursorPos();
            auto cursorScreenPos = ImGui::GetCursorScreenPos();

            if(CurrentEnty->IsFile)
            {
                textureId                    = m_FileIcon;
                static bool EnableThumbnails = false;
                if(EnableThumbnails)
                    switch(CurrentEnty->Type)
                    {
                    case FileType::Texture:
                    {
                        if(CurrentEnty->Thumbnail)
                        {
                            textureId = CurrentEnty->Thumbnail;
                        }
                        else if(!textureCreated)
                        {
                            textureCreated = true;
                            if(!m_Editor->GetAssetManager()->AssetExists(CurrentEnty->AssetPath))
                                CurrentEnty->Thumbnail = m_Editor->GetAssetManager()->LoadTextureAsset(CurrentEnty->AssetPath, true);
                            else
                                CurrentEnty->Thumbnail = m_Editor->GetAssetManager()->GetAssetData(CurrentEnty->AssetPath).As<Graphics::Texture2D>();
                            textureId = CurrentEnty->Thumbnail ? CurrentEnty->Thumbnail : m_FileIcon;
                        }
                        break;
                    }
                    case FileType::Scene:
                    {
                        ArenaTemp scratch                       = ArenaTempBegin(m_Arena);
                        String8 fileName                        = PushStr8Copy(scratch.arena, StringUtilities::GetFileName(CurrentEnty->AssetPath));
                        String8 sceneScreenShotPath             = PushStr8F(scratch.arena, "%s/Scenes/Cache/%s.png", (const char*)m_BasePath.str, (const char*)fileName.str);
                        std::string sceneScreenShotPathtdString = std::string((const char*)sceneScreenShotPath.str, sceneScreenShotPath.size);
                        if(std::filesystem::exists(std::filesystem::path(sceneScreenShotPathtdString)))
                        {
                            textureCreated                   = true;
                            String8 sceneScreenShotAssetPath = PushStr8F(scratch.arena, "%s/Scenes/Cache/%s.png", (const char*)Str8Lit("//Assets").str, (const char*)fileName.str);

                            if(!m_Editor->GetAssetManager()->AssetExists(sceneScreenShotAssetPath))
                                CurrentEnty->Thumbnail = m_Editor->GetAssetManager()->LoadTextureAsset(sceneScreenShotAssetPath, true);
                            else
                                CurrentEnty->Thumbnail = m_Editor->GetAssetManager()->GetAssetData(sceneScreenShotAssetPath).As<Graphics::Texture2D>();
                            textureId = CurrentEnty->Thumbnail ? CurrentEnty->Thumbnail : m_FileIcon;
                        }
                        ArenaTempEnd(scratch);
                        break;
                    }
                    case FileType::Material:
                    case FileType::Model:
                    {
                        if(CurrentEnty->Thumbnail)
                        {
                            textureId = CurrentEnty->Thumbnail;
                        }
                        else if(!textureCreated)
                        {
                            textureCreated    = true;
                            ArenaTemp scratch = ArenaTempBegin(m_Arena);

                            String8 thumbnailPath;
                            String8 thumbnailAssetPath;
                            CreateThumbnailPath(scratch.arena, CurrentEnty, thumbnailAssetPath, thumbnailPath);

                            if(std::filesystem::exists(std::filesystem::path((const char*)thumbnailPath.str)))
                            {
                                textureCreated = true;
                                if(!m_Editor->GetAssetManager()->AssetExists(thumbnailAssetPath))
                                    CurrentEnty->Thumbnail = m_Editor->GetAssetManager()->LoadTextureAsset(thumbnailPath, true);
                                else
                                    CurrentEnty->Thumbnail = m_Editor->GetAssetManager()->GetAssetData(thumbnailAssetPath).As<Graphics::Texture2D>();
                                textureId = CurrentEnty->Thumbnail ? CurrentEnty->Thumbnail : m_FileIcon;
                            }
                            else
                            {
                                m_Editor->RequestThumbnail(CurrentEnty->AssetPath);
                                textureId = m_FileIcon;
                            }

                            ArenaTempEnd(scratch);
                        }
                        break;
                    }
                    default:
                        break;
                    }
            }
            bool flipImage = false;

            bool highlight = false;
            {
                highlight = m_CurrentDir->Children[dirIndex] == m_CurrentSelected;
            }

            // Background button
            bool const clicked = ImGuiUtilities::ToggleButton(ImGuiUtilities::GenerateID(), highlight, backgroundThumbnailSize, 0.0f, 1.0f, ImGuiButtonFlags_AllowOverlap);
            if(clicked)
            {
                m_CurrentSelected = m_CurrentDir->Children[dirIndex];
            }

            if(ImGui::BeginPopupContextItem())
            {
                m_CurrentSelected = m_CurrentDir->Children[dirIndex];

                if(ImGui::Selectable("Cut"))
                {
                    m_CopiedPath = m_CurrentDir->Children[dirIndex]->AssetPath;
                    m_CutFile    = true;
                }

                if(ImGui::Selectable("Copy"))
                {
                    m_CopiedPath = m_CurrentDir->Children[dirIndex]->AssetPath;
                    m_CutFile    = false;
                }

                if(ImGui::Selectable("Delete"))
                {
                    ArenaTemp temp   = ScratchBegin(&m_Arena, 1);
                    String8 fullPath = StringUtilities::RelativeToAbsolutePath(temp.arena, m_CurrentDir->Children[dirIndex]->AssetPath, Str8Lit("//Assets"), m_BasePath);
                    std::filesystem::remove_all(std::string((const char*)fullPath.str, fullPath.size));
                    ScratchEnd(temp);
                    QueueRefresh();
                }

                if(ImGui::Selectable("Duplicate"))
                {

                    ArenaTemp temp   = ScratchBegin(&m_Arena, 1);
                    String8 fullPath = StringUtilities::RelativeToAbsolutePath(temp.arena, m_CurrentDir->Children[dirIndex]->AssetPath, Str8Lit("//Assets"), m_BasePath);

                    std::filesystem::path fullPathFS      = std::string((const char*)fullPath.str, fullPath.size);
                    std::filesystem::path destinationPath = fullPathFS;

                    {
                        std::string filename  = fullPathFS.stem().string();
                        std::string extension = fullPathFS.extension().string();

                        while(std::filesystem::exists(destinationPath))
                        {
                            filename += "_copy";
                            destinationPath = destinationPath.parent_path() / (filename + extension);
                        }
                    }
                    std::filesystem::copy(fullPathFS, destinationPath);

                    ScratchEnd(temp);
                    QueueRefresh();
                }

                ImGui::Separator();

                if(ImGui::Selectable("Open Location"))
                {
                    ArenaTemp temp   = ScratchBegin(&m_Arena, 1);
                    String8 fullPath = StringUtilities::RelativeToAbsolutePath(temp.arena, m_CurrentDir->Children[dirIndex]->AssetPath, Str8Lit("//Assets"), m_BasePath);
                    Lumos::OS::Get().OpenFileLocation(std::string((const char*)fullPath.str, fullPath.size));
                    ScratchEnd(temp);
                }

                if(m_CurrentDir->Children[dirIndex]->IsFile && ImGui::Selectable("Open External"))
                {
                    ArenaTemp temp   = ScratchBegin(&m_Arena, 1);
                    String8 fullPath = StringUtilities::RelativeToAbsolutePath(temp.arena, m_CurrentDir->Children[dirIndex]->AssetPath, Str8Lit("//Assets"), m_BasePath);
                    Lumos::OS::Get().OpenFileExternal(std::string((const char*)fullPath.str, fullPath.size));
                    ScratchEnd(temp);
                }

                if(ImGui::Selectable("Copy Full Path"))
                {
                    ArenaTemp temp   = ScratchBegin(&m_Arena, 1);
                    String8 fullPath = StringUtilities::RelativeToAbsolutePath(temp.arena, m_CurrentDir->Children[dirIndex]->AssetPath, Str8Lit("//Assets"), m_BasePath);
                    ImGui::SetClipboardText((const char*)ToStdString(fullPath).c_str());
                    ScratchEnd(temp);
                }

                if(m_CurrentDir->Children[dirIndex]->IsFile && ImGui::Selectable("Copy Asset Path"))
                {
                    ImGui::SetClipboardText((const char*)ToStdString(m_CurrentDir->Children[dirIndex]->AssetPath).c_str());
                }

                ImGui::Separator();

                if(ImGui::Selectable("Import New Asset"))
                {
                    m_Editor->OpenFile();
                }

                if(ImGui::Selectable("Refresh"))
                {
                    QueueRefresh();
                }

                if(ImGui::Selectable("New folder"))
                {
                    ArenaTemp temp   = ScratchBegin(&m_Arena, 1);
                    String8 fullPath = StringUtilities::RelativeToAbsolutePath(temp.arena, m_CurrentDir->AssetPath, Str8Lit("//Assets"), m_BasePath);
                    std::filesystem::create_directory(std::filesystem::path(std::string((const char*)fullPath.str, fullPath.size) + "/NewFolder"));
                    ScratchEnd(temp);

                    QueueRefresh();
                }

                if(!m_IsInListView)
                {
                    ImGui::SliderFloat("##GridSize", &m_GridSize, MinGridSize, MaxGridSize);
                }
                ImGui::EndPopup();
            }

            if(ImGui::IsItemHovered() && m_CurrentDir->Children[dirIndex]->Thumbnail)
            {
                Vec2 TooltipSize = GetAspectCorrectedSize(Vec2(textureId->GetWidth(), textureId->GetHeight()), 512);
                ImGuiUtilities::Tooltip(m_CurrentDir->Children[dirIndex]->Thumbnail, TooltipSize, (const char*)(m_CurrentDir->Children[dirIndex]->AssetPath.str));
            }
            else
                ImGuiUtilities::Tooltip((const char*)(m_CurrentDir->Children[dirIndex]->AssetPath.str));

            if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                ImGui::TextUnformatted(m_Editor->GetIconFontIcon(ToStdString(m_CurrentDir->Children[dirIndex]->AssetPath)));

                ImGui::SameLine();
                m_MovePath           = m_CurrentDir->Children[dirIndex]->AssetPath;
                String8 resolvedPath = StringUtilities::AbsolutePathToRelativeFileSystemPath(m_Arena, m_MovePath, m_BasePath, Str8Lit("//Assets"));

                ImGui::TextUnformatted((const char*)resolvedPath.str);

                size_t size = sizeof(const char*) + resolvedPath.size;
                ImGui::SetDragDropPayload("AssetFile", resolvedPath.str, size);
                m_IsDragging = true;
                ImGui::EndDragDropSource();
            }

            if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                doubleClicked = true;
            }

            ImGui::SetCursorPos({ cursorPos.x + padding, cursorPos.y + padding });
            ImGui::SetNextItemAllowOverlap();
            ImGui::Image(reinterpret_cast<ImTextureID>(Application::Get().GetImGuiManager()->GetImGuiRenderer()->AddTexture(Graphics::Material::GetDefaultTexture())), { backgroundThumbnailSize.x - padding * 2.0f, backgroundThumbnailSize.y - padding * 2.0f }, { 0, 0 }, { 1, 1 }, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg) + ImVec4(0.04f, 0.04f, 0.04f, 0.04f));

            ImGui::SetCursorPos({ cursorPos.x + thumbnailPadding * 0.75f, cursorPos.y + thumbnailPadding });
            ImGui::SetNextItemAllowOverlap();

            Vec2 correctedSize = GetAspectCorrectedSize(Vec2(textureId->GetWidth(), textureId->GetHeight()), thumbnailSize);
            Vec2 padding2      = (Vec2(thumbnailSize) - correctedSize) * 0.5f;
            ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(padding2.x, padding2.y));
            ImGuiUtilities::Image(textureId, correctedSize);

            const ImVec2 typeColorFrameSize = { scaledThumbnailSizeX, scaledThumbnailSizeX * 0.03f };
            ImGui::SetCursorPosX(cursorPos.x + padding);
            ImGui::Image(reinterpret_cast<ImTextureID>(Application::Get().GetImGuiManager()->GetImGuiRenderer()->AddTexture(Graphics::Material::GetDefaultTexture())), typeColorFrameSize, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f), !CurrentEnty->IsFile ? ImVec4(0.0f, 0.0f, 0.0f, 0.0f) : CurrentEnty->FileTypeColour);

            const float smallFontSize = ImGui::GetIO().Fonts->Fonts[2]->FontSize;
            const float dpiScale = (float)m_Editor->GetWindow()->GetDPIScale();
            const bool smallIcons = m_GridSize < 140 * dpiScale;
            const float bottomLabelsHeight = CurrentEnty->IsFile ? (smallIcons ? smallFontSize * 1.2f : smallFontSize * 2.2f) : 0.0f;
            const float textTopY = cursorScreenPos.y + thumbnailSize + thumbnailPadding + typeColorFrameSize.y + padding * 3.0f;

            const ImRect clipRect = ImRect({ cursorScreenPos.x + padding * 2.0f, textTopY },
                                           { cursorScreenPos.x + backgroundThumbnailSize.x - padding * 2.0f, cursorScreenPos.y + backgroundThumbnailSize.y - bottomLabelsHeight - padding });

            {
                if(smallIcons)
                {
                    ImGuiUtilities::ScopedFont smallFont(ImGui::GetIO().Fonts->Fonts[2]);
                    ImGuiUtilities::ClippedText(clipRect.Min, clipRect.Max, (const char*)(StringUtilities::GetFileName(CurrentEnty->AssetPath, !CurrentEnty->IsFile).str), nullptr, nullptr, { 0, 0 }, nullptr, clipRect.GetSize().x);
                }
                else
                {
                    ImGuiUtilities::ClippedText(clipRect.Min, clipRect.Max, (const char*)(StringUtilities::GetFileName(CurrentEnty->AssetPath, !CurrentEnty->IsFile).str), nullptr, nullptr, { 0, 0 }, nullptr, clipRect.GetSize().x);
                }
            }

            if(CurrentEnty->IsFile)
            {
                ImGui::BeginDisabled();
                ImGuiUtilities::ScopedFont smallFont(ImGui::GetIO().Fonts->Fonts[2]);

                const float labelPadding = padding * dpiScale;
                const float labelHeight = smallFontSize;

                if(smallIcons)
                {
                    // Only show file type when icons are small
                    ImGui::SetCursorPos({ cursorPos.x + labelPadding, cursorPos.y + backgroundThumbnailSize.y - labelHeight - labelPadding });
                    ImGui::Indent();

                    String8 fileTypeString       = s_FileTypesToString.at(FileType::Unknown);
                    const auto& fileStringTypeIt = s_FileTypesToString.find(CurrentEnty->Type);
                    if(fileStringTypeIt != s_FileTypesToString.end())
                        fileTypeString = fileStringTypeIt->second;

                    ImGui::TextUnformatted((const char*)(fileTypeString).str);
                    ImGui::Unindent();
                }
                else
                {
                    // Show both file type and size when icons are larger
                    ImGui::SetCursorPos({ cursorPos.x + labelPadding, cursorPos.y + backgroundThumbnailSize.y - labelHeight * 2.0f - labelPadding });
                    ImGui::Indent();

                    String8 fileTypeString       = s_FileTypesToString.at(FileType::Unknown);
                    const auto& fileStringTypeIt = s_FileTypesToString.find(CurrentEnty->Type);
                    if(fileStringTypeIt != s_FileTypesToString.end())
                        fileTypeString = fileStringTypeIt->second;

                    ImGui::TextUnformatted((const char*)(fileTypeString).str);
                    ImGui::Unindent();

                    ImGui::SetCursorPos({ cursorPos.x + labelPadding, cursorPos.y + backgroundThumbnailSize.y - labelHeight - labelPadding });
                    ImGui::Indent();
                    ImGui::TextUnformatted(StringUtilities::BytesToString(CurrentEnty->FileSize).c_str());
                    ImGui::Unindent();
                }

                ImGui::EndDisabled();
            }
        }
        else
        {
            ImGui::TextUnformatted(folder ? ICON_MDI_FOLDER : m_Editor->GetIconFontIcon(std::string((const char*)m_CurrentDir->Children[dirIndex]->AssetPath.str, m_CurrentDir->Children[dirIndex]->AssetPath.size)));
            ImGui::SameLine();

            if(ImGui::Selectable((const char*)StringUtilities::GetFileName(m_CurrentDir->Children[dirIndex]->AssetPath, !m_CurrentDir->Children[dirIndex]->IsFile).str, false, ImGuiSelectableFlags_AllowDoubleClick))
            {
                if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    doubleClicked = true;
                }
            }

            ImGuiUtilities::Tooltip((const char*)m_CurrentDir->Children[dirIndex]->AssetPath.str);

            if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                ImGui::TextUnformatted(m_Editor->GetIconFontIcon(ToStdString(m_CurrentDir->Children[dirIndex]->AssetPath)));

                ImGui::SameLine();
                m_MovePath = m_CurrentDir->Children[dirIndex]->AssetPath;
                ImGui::TextUnformatted((const char*)ToStdString(m_MovePath).c_str());

                size_t size = sizeof(const char*) + m_MovePath.size;
                ImGui::SetDragDropPayload("AssetFile", m_MovePath.str, size);
                m_IsDragging = true;
                ImGui::EndDragDropSource();
            }
        }

        if(doubleClicked)
        {
            if(folder)
            {
                ChangeDirectory(m_CurrentDir->Children[dirIndex]);
            }
            else
            {
                ArenaTemp temp      = ScratchBegin(&m_Arena, 1);
                String8 currentPath = StringUtilities::RelativeToAbsolutePath(temp.arena, m_CurrentDir->Children[dirIndex]->AssetPath, Str8Lit("//Assets"), m_BasePath);
                m_Editor->FileOpenCallback(std::string((const char*)currentPath.str, currentPath.size));
                ScratchEnd(temp);
            }
        }

        return doubleClicked;
    }

    bool ResourcePanel::MoveFile(const String8 filePath, String8 movePath)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string s = "move " + std::string((const char*)filePath.str, filePath.size) + " " + std::string((const char*)movePath.str, movePath.size);

#ifndef LUMOS_PLATFORM_IOS
        system(s.c_str());
#endif

        return std::filesystem::exists(std::filesystem::path(std::string((const char*)movePath.str, movePath.size)) / StringUtilities::GetFileName(std::string((const char*)filePath.str, filePath.size)));
    }

    void ResourcePanel::OnNewProject()
    {
        QueueRefresh();
    }

    void ResourcePanel::Refresh()
    {
        ArenaTemp temp      = ScratchBegin(&m_Arena, 1);
        String8 currentPath = PushStr8Copy(temp.arena, m_CurrentDir->AssetPath);

        ArenaClear(m_Arena);
        m_Directories.clear();

        m_BasePath                  = PushStr8F(m_Arena, "%sAssets", Application::Get().GetProjectSettings().m_ProjectRoot.c_str());
        String8 baseDirectoryHandle = ProcessDirectory(m_BasePath, nullptr, true);
        m_BaseProjectDir            = m_Directories[baseDirectoryHandle];
        ChangeDirectory(m_BaseProjectDir);

        m_UpdateNavigationPath = true;

        m_BaseProjectDir    = m_Directories[baseDirectoryHandle];
        m_PreviousDirectory = nullptr;
        m_CurrentDir        = nullptr;

        bool dirFound = false;
        for(auto dir : m_Directories)
        {
            if(Str8Match(dir.first, currentPath))
            {
                m_CurrentDir = dir.second;
                dirFound     = true;
                break;
            }
        }
        if(!dirFound)
            ChangeDirectory(m_BaseProjectDir);
        else
            ChangeDirectory(m_CurrentDir);

        ScratchEnd(temp);
    }

    void ResourcePanel::CreateThumbnailPath(Arena* arena, DirectoryInformation* directoryInfo, String8& assetPath, String8& AbsolutePath)
    {
        String8 assetPath1    = directoryInfo->AssetPath;
        String8 thumbnailPath = PushStr8F(arena, "%s_thumbnail.png", (const char*)assetPath1.str);

        assetPath    = StringUtilities::AbsolutePathToRelativeFileSystemPath(arena, thumbnailPath, Str8Lit("//Assets"), Str8Lit("//Assets/Cache"));
        AbsolutePath = StringUtilities::AbsolutePathToRelativeFileSystemPath(arena, assetPath, Str8Lit("//Assets"), m_BasePath);
    }

    void ResourcePanel::GetAllAssets(std::vector<std::string>& outAssets)
    {
        std::string basePath = ToStdString(m_BasePath);
        if(!std::filesystem::exists(basePath))
            return;

        for(auto& entry : std::filesystem::recursive_directory_iterator(basePath))
        {
            if(entry.is_regular_file())
            {
                std::string path = entry.path().string();
                // Convert to asset path (//Assets/...)
                if(path.size() > basePath.size())
                {
                    std::string relativePath = "//Assets" + path.substr(basePath.size());
                    // Skip hidden files and cache
                    if(relativePath.find("/.") == std::string::npos &&
                       relativePath.find("/Cache/") == std::string::npos)
                    {
                        outAssets.push_back(relativePath);
                    }
                }
            }
        }
        std::sort(outAssets.begin(), outAssets.end());
    }
}
