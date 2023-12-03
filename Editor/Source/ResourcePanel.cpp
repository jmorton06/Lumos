#include "Editor.h"
#include "ResourcePanel.h"
#include <Lumos/Core/OS/Input.h>
#include <Lumos/Graphics/RHI/Texture.h>
#include <Lumos/Core/Profiler.h>
#include <Lumos/Core/StringUtilities.h>
#include <Lumos/Core/VFS.h>
#include <Lumos/Core/OS/Window.h>
#include <Lumos/Graphics/Material.h>
#include <Lumos/Maths/MathsUtilities.h>
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <Lumos/Embedded/browserFile.inl>
#include <Lumos/Embedded/browserFolder.inl>
#include <Lumos/Core/OS/OS.h>

#ifdef LUMOS_PLATFORM_WINDOWS
#include <Windows.h>
#undef RemoveDirectory
#undef MoveFile
#include <Shellapi.h>
#endif

namespace Lumos
{

    static const std::unordered_map<FileType, const char*> s_FileTypesToString = {
        { FileType::Unknown, "Unknown" },
        { FileType::Scene, "Scene" },
        { FileType::Prefab, "Prefab" },
        { FileType::Script, "Script" },
        { FileType::Shader, "Shader" },
        { FileType::Texture, "Texture" },
        { FileType::Font, "Font" },
        { FileType::Cubemap, "Cubemap" },
        { FileType::Model, "Model" },
        { FileType::Audio, "Audio" },
    };

    static const std::unordered_map<std::string, FileType> s_FileTypes = {
        { ".lsn", FileType::Scene },
        { ".lprefab", FileType::Prefab },
        { ".cs", FileType::Script },
        { ".lua", FileType::Script },
        { ".glsl", FileType::Shader },
        { ".shader", FileType::Shader },
        { ".frag", FileType::Shader },
        { ".vert", FileType::Shader },
        { ".comp", FileType::Shader },

        { ".png", FileType::Texture },
        { ".jpg", FileType::Texture },
        { ".jpeg", FileType::Texture },
        { ".bmp", FileType::Texture },
        { ".gif", FileType::Texture },
        { ".tga", FileType::Texture },
        { ".ttf", FileType::Font },

        { ".hdr", FileType::Cubemap },

        { ".obj", FileType::Model },
        { ".fbx", FileType::Model },
        { ".gltf", FileType::Model },

        { ".mp3", FileType::Audio },
        { ".m4a", FileType::Audio },
        { ".wav", FileType::Audio },
        { ".ogg", FileType::Audio },
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

#ifdef LUMOS_PLATFORM_WINDOWS
    std::string ResourcePanel::m_Delimiter = "\\";
#else
    std::string ResourcePanel::m_Delimiter = "/";
#endif
    ResourcePanel::ResourcePanel()
    {
        LUMOS_PROFILE_FUNCTION();
        m_Name       = ICON_MDI_FOLDER_STAR " Resources###resources";
        m_SimpleName = "Resources";

        float dpi = Application::Get().GetWindow()->GetDPIScale();
        m_GridSize = 180.0f;
		m_GridSize *= dpi;
		MinGridSize *= dpi;
		MaxGridSize *= dpi;

        // TODO: Get Project path from editor
        // #ifdef LUMOS_PLATFORM_IOS
        //         m_BaseDirPath = "Assets";
        // #else
        //         m_BaseProjectDir = std::filesystem::path(m_Editor->GetProjectRoot() + "/ExampleProject/Assets");
        //         //m_BaseDirPath = ROOT_DIR "/ExampleProject/Assets";
        // #endif
        m_BasePath = Application::Get().GetProjectSettings().m_ProjectRoot + "Assets";

        std::string assetsBasePath;
        VFS::Get().ResolvePhysicalPath("//Assets", assetsBasePath);
        m_AssetPath = std::filesystem::path(assetsBasePath);

        std::string baseDirectoryHandle = ProcessDirectory(std::filesystem::path(m_BasePath), nullptr);
        m_BaseProjectDir                = m_Directories[baseDirectoryHandle];
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
        m_FileIcon     = Graphics::Texture2D::CreateFromSource(browserFileWidth, browserFileHeight, (void*)browserFile, desc);
        m_FolderIcon   = Graphics::Texture2D::CreateFromSource(browserFolderWidth, browserFolderHeight, (void*)browserFolder, desc);
    }

    void ResourcePanel::ChangeDirectory(SharedPtr<DirectoryInformation>& directory)
    {
        if(!directory)
            return;

        m_PreviousDirectory    = m_CurrentDir;
        m_CurrentDir           = directory;
        m_UpdateNavigationPath = true;
    }

    void ResourcePanel::RemoveDirectory(SharedPtr<DirectoryInformation>& directory, bool removeFromParent)
    {
        if(directory->Parent && removeFromParent)
        {
            directory->Parent->Children.clear();
        }

        for(auto& subdir : directory->Children)
            RemoveDirectory(subdir, false);

        m_Directories.erase(m_Directories.find(directory->FilePath.string()));
    }

    bool IsHidden(const std::filesystem::path& filePath)
    {
        try
        {
            std::filesystem::file_status status = std::filesystem::status(filePath);
            return (status.permissions() & std::filesystem::perms::owner_read) == std::filesystem::perms::none;
        }
        catch(const std::filesystem::filesystem_error& ex)
        {
            std::cerr << "Error accessing file: " << ex.what() << std::endl;
        }

        return false; // Return false by default if any error occurs
    }

    std::string ResourcePanel::ProcessDirectory(const std::filesystem::path& directoryPath, const SharedPtr<DirectoryInformation>& parent)
    {
        const auto& directory = m_Directories[directoryPath.string()];
        if(directory)
            return directory->FilePath.string();

        SharedPtr<DirectoryInformation> directoryInfo = CreateSharedPtr<DirectoryInformation>(directoryPath, !std::filesystem::is_directory(directoryPath));
        directoryInfo->Parent                         = parent;

        if(directoryPath == m_BasePath)
            directoryInfo->FilePath = m_BasePath;
        else
            directoryInfo->FilePath = std::filesystem::relative(directoryPath, m_BasePath);

        directoryInfo->FullPath   = directoryPath;
        directoryInfo->FileTypeID = GetParsedAssetID(StringUtilities::GetFilePathExtension(directoryInfo->FilePath.string()));

        if(std::filesystem::is_directory(directoryPath))
        {
            directoryInfo->IsFile = false;
            directoryInfo->Name   = directoryPath.filename().string();
            for(auto& entry : std::filesystem::directory_iterator(directoryPath))
            {
                if(!m_ShowHiddenFiles && IsHidden(entry.path()))
                {
                    continue;
                }
                {
                    std::string subdirHandle = ProcessDirectory(entry.path(), directoryInfo);
                    directoryInfo->Children.push_back(m_Directories[subdirHandle]);
                }
            }
        }
        else
        {
            auto fileType          = FileType::Unknown;
            const auto& fileTypeIt = s_FileTypes.find(directoryPath.extension().string());
            if(fileTypeIt != s_FileTypes.end())
                fileType = fileTypeIt->second;

            directoryInfo->IsFile   = true;
            directoryInfo->Type     = fileType;
            directoryInfo->Name     = directoryPath.filename().string();
            directoryInfo->FileSize = std::filesystem::exists(directoryPath) ? StringUtilities::BytesToString(std::filesystem::file_size(directoryPath)) : "";

            std::string_view fileTypeString = s_FileTypesToString.at(FileType::Unknown);
            const auto& fileStringTypeIt    = s_FileTypesToString.find(fileType);
            if(fileStringTypeIt != s_FileTypesToString.end())
                fileTypeString = fileStringTypeIt->second;

            ImVec4 fileTypeColor        = { 1.0f, 1.0f, 1.0f, 1.0f };
            const auto& fileTypeColorIt = s_TypeColors.find(fileType);
            if(fileTypeColorIt != s_TypeColors.end())
                fileTypeColor = fileTypeColorIt->second;

            directoryInfo->FileTypeString = fileTypeString;
            directoryInfo->FileTypeColour = fileTypeColor;
        }

        m_Directories[directoryInfo->FilePath.string()] = directoryInfo;
        return directoryInfo->FilePath.string();
    }

    void ResourcePanel::DrawFolder(const SharedPtr<DirectoryInformation>& dirInfo, bool defaultOpen)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGuiTreeNodeFlags nodeFlags = ((dirInfo == m_CurrentDir) ? ImGuiTreeNodeFlags_Selected : 0);
        nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

        if(dirInfo->Parent == nullptr)
            nodeFlags |= ImGuiTreeNodeFlags_Framed;

        const ImColor TreeLineColor = ImColor(128, 128, 128, 128);
        const float SmallOffsetX    = 6.0f * Application::Get().GetWindowDPI();
        ImDrawList* drawList        = ImGui::GetWindowDrawList();

        if(Input::Get().GetKeyHeld(InputCode::Key::LeftControl))
		{
			float mouseScroll = Input::Get().GetScrollOffset();
			m_GridSize += mouseScroll;
			m_GridSize = Maths::Clamp(m_GridSize, MinGridSize, MaxGridSize);
        }

        if(!dirInfo->IsFile)
        {
            bool containsFolder = false;

            for(auto& file : dirInfo->Children)
            {
                if(!file->IsFile)
                {
                    containsFolder = true;
                    break;
                }
            }
            if(!containsFolder)
                nodeFlags |= ImGuiTreeNodeFlags_Leaf;

            if(defaultOpen)
                nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf;

            nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_SpanAvailWidth;

            bool isOpen = ImGui::TreeNodeEx((void*)(intptr_t)(dirInfo.get()), nodeFlags, "");
            if(ImGui::IsItemClicked())
            {
                m_PreviousDirectory    = m_CurrentDir;
                m_CurrentDir           = dirInfo;
                m_UpdateNavigationPath = true;
            }

            const char* folderIcon = ((isOpen && containsFolder) || m_CurrentDir == dirInfo) ? ICON_MDI_FOLDER_OPEN : ICON_MDI_FOLDER;
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtilities::GetIconColour());
            ImGui::Text("%s ", folderIcon);
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextUnformatted((const char*)dirInfo->FilePath.filename().string().c_str());

            ImVec2 verticalLineStart = ImGui::GetCursorScreenPos();

            if(isOpen && containsFolder)
            {
                verticalLineStart.x += SmallOffsetX; // to nicely line up with the arrow symbol
                ImVec2 verticalLineEnd = verticalLineStart;

                for(int i = 0; i < dirInfo->Children.size(); i++)
                {
                    if(!dirInfo->Children[i]->IsFile)
                    {
                        auto currentPos = ImGui::GetCursorScreenPos();

                        ImGui::Indent(10.0f);

                        bool containsFolderTemp = false;
                        for(auto& file : dirInfo->Children[i]->Children)
                        {
                            if(!file->IsFile)
                            {
                                containsFolderTemp = true;
                                break;
                            }
                        }
                        float HorizontalTreeLineSize = 16.0f * Application::Get().GetWindowDPI(); // chosen arbitrarily

                        if(containsFolderTemp)
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

            if(isOpen && !containsFolder)
                ImGui::TreePop();
        }

        if(m_IsDragging && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        {
            m_MovePath = dirInfo->FilePath.string().c_str();
        }
    }

    static int FileIndex = 0;

    void ResourcePanel::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();

        m_TextureLibrary.Update((float)Engine::Get().GetTimeStep().GetElapsedSeconds());
        if(ImGui::Begin(m_Name.c_str(), &m_Active))
        {
            FileIndex        = 0;
            auto windowSize  = ImGui::GetWindowSize();
            bool vertical    = windowSize.y > windowSize.x;
            static bool Init = false;
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
                RenderBreadCrumbs();

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
                    std::string file = (char*)data->Data;
                    if(MoveFile(file, m_MovePath))
                    {
                        LUMOS_LOG_INFO("Moved File: " + file + " to " + m_MovePath);
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

            // ImGui::BeginChild("##directory_structure", ImVec2(0, ImGui::GetWindowHeight() - ImGui::GetFrameHeightWithSpacing() * 2.6f - offset));
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
                            Refresh();
                        }

                        if(ImGui::Selectable("New folder"))
                        {
                            std::filesystem::create_directory(std::filesystem::path(m_CurrentDir->FullPath / "NewFolder"));
                            Refresh();
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
                            m_PreviousDirectory    = m_CurrentDir;
                            m_CurrentDir           = m_CurrentDir->Parent;
                            m_UpdateNavigationPath = true;
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
                        m_BreadCrumbData.clear();
                        auto current = m_CurrentDir;
                        while(current)
                        {
                            if(current->Parent != nullptr)
                            {
                                m_BreadCrumbData.push_back(current);
                                current = current->Parent;
                            }
                            else
                            {
                                m_BreadCrumbData.push_back(m_BaseProjectDir);
                                current = nullptr;
                            }
                        }

                        std::reverse(m_BreadCrumbData.begin(), m_BreadCrumbData.end());
                        m_UpdateNavigationPath = false;
                    }

                    {
                        int secIdx = 0, newPwdLastSecIdx = -1;

                        auto& AssetsDir = m_CurrentDir->FilePath;

                        size_t PhysicalPathCount = 0;

                        for(auto& sec : m_AssetPath)
                        {
                            PhysicalPathCount++;
                        }
                        int dirIndex = 0;
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.2f, 0.7f, 0.0f));

                        for(auto& directory : m_BreadCrumbData)
                        {
                            const std::string& directoryName = directory->FilePath.filename().string();
                            if(ImGui::SmallButton(directoryName.c_str()))
                                ChangeDirectory(directory);

                            ImGui::SameLine();
                        }

                        ImGui::PopStyleColor();

                        if(newPwdLastSecIdx >= 0)
                        {
                            int i = 0;
                            std::filesystem::path newPwd;
                            for(auto& sec : AssetsDir)
                            {
                                if(i++ > newPwdLastSecIdx)
                                    break;
                                newPwd /= sec;
                            }
#ifdef _WIN32
                            if(newPwdLastSecIdx == 0)
                                newPwd /= "\\";
#endif

                            m_PreviousDirectory    = m_CurrentDir;
                            m_CurrentDir           = m_Directories[newPwd.string()];
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
                        m_GridItemsPerRow = (int)floor(xAvail / (m_GridSize + ImGui::GetStyle().ItemSpacing.x));
                        m_GridItemsPerRow = Maths::Max(1, m_GridItemsPerRow);

                        textureCreated = false;

                        ImGuiUtilities::PushID();

                        if(m_IsInListView)
                        {
                            for(int i = 0; i < m_CurrentDir->Children.size(); i++)
                            {
                                if(m_CurrentDir->Children.size() > 0)
                                {
                                    if(!m_ShowHiddenFiles && Lumos::StringUtilities::IsHiddenFile(m_CurrentDir->Children[i]->FilePath.filename().string()))
                                    {
                                        continue;
                                    }

                                    if(m_Filter.IsActive())
                                    {
                                        if(!m_Filter.PassFilter(m_CurrentDir->Children[i]->FilePath.filename().string().c_str()))
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
                        }
                        else
                        {
                            for(int i = 0; i < m_CurrentDir->Children.size(); i++)
                            {
                                if(!m_ShowHiddenFiles && Lumos::StringUtilities::IsHiddenFile(m_CurrentDir->Children[i]->FilePath.filename().string()))
                                {
                                    continue;
                                }

                                if(m_Filter.IsActive())
                                {
                                    if(!m_Filter.PassFilter(m_CurrentDir->Children[i]->FilePath.filename().string().c_str()))
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
                            if(std::filesystem::exists(m_CopiedPath) && ImGui::Selectable("Paste"))
                            {
                                if(m_CutFile)
                                {
                                    const std::filesystem::path& fullPath = m_CopiedPath;
                                    std::string filename                  = fullPath.stem().string();
                                    std::string extension                 = fullPath.extension().string();
                                    std::filesystem::path destinationPath = m_BasePath / m_CurrentDir->FilePath / (filename + extension);

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
                                    const std::filesystem::path& fullPath = m_CopiedPath;
                                    std::string filename                  = fullPath.stem().string();
                                    std::string extension                 = fullPath.extension().string();
                                    std::filesystem::path destinationPath = m_BasePath / m_CurrentDir->FilePath / (filename + extension);

                                    {
                                        while(std::filesystem::exists(destinationPath))
                                        {
                                            filename += "_copy";
                                            destinationPath = destinationPath.parent_path() / (filename + extension);
                                        }
                                    }
                                    std::filesystem::copy(fullPath, destinationPath);
                                }
                                m_CopiedPath = "";
                                m_CutFile    = false;
                                Refresh();
                            }

                            if(ImGui::Selectable("Open Location"))
                            {
                                auto fullPath = m_BasePath + "/" + m_CurrentDir->FilePath.string();
                                Lumos::OS::Instance()->OpenFileLocation(fullPath);
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
                                std::filesystem::create_directory(std::filesystem::path(m_CurrentDir->Parent ? m_CurrentDir->Parent->FilePath.string() + "/NewFolder" : m_BasePath + "/NewFolder"));
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
                    std::string a = (char*)data->Data;
                    if(MoveFile(a, m_MovePath))
                    {
                        LUMOS_LOG_INFO("Moved File: " + a + " to " + m_MovePath);
                    }
                    m_IsDragging = false;
                }
                ImGui::EndDragDropTarget();
            }
        }
        ImGui::End();
    }

    void ResourcePanel::RenderBreadCrumbs()
    {
        LUMOS_PROFILE_FUNCTION();
        // ImGui::BeginChild("##directory_breadcrumbs", ImVec2(ImGui::GetColumnWidth(), ImGui::GetFontSize() * 2.0f));
        //  {

        //            int dirIndex = 0;
        //            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.2f, 0.7f, 0.0f));
        //
        //            for(auto& directory : m_BreadCrumbData)
        //            {
        //                std::string directoryName = directory->FilePath.filename().string();
        //                if(ImGui::SmallButton(directoryName.c_str()))
        //                    ChangeDirectory(directory);
        //
        //                ImGui::SameLine();
        //            }
        //
        //            ImGui::PopStyleColor();
        //
        //            if(newPwdLastSecIdx >= 0)
        //            {
        //                int i = 0;
        //                std::filesystem::path newPwd;
        //                for(auto& sec : AssetsDir)
        //                {
        //                    if(i++ > newPwdLastSecIdx)
        //                        break;
        //                    newPwd /= sec;
        //                }
        // #ifdef _WIN32
        //                if(newPwdLastSecIdx == 0)
        //                    newPwd /= "\\";
        // #endif
        //
        //                m_PreviousDirectory = m_CurrentDir;
        //                m_CurrentDir = m_Directories[newPwd.string()];
        //                m_UpdateNavigationPath = true;
        //            }
        //
        //            ImGui::SameLine();
        //        }

        // ImGui::EndChild();
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
            auto& CurrentEnty = m_CurrentDir->Children[dirIndex];
            void* textureId   = m_FolderIcon->GetHandle();

            auto cursorPos = ImGui::GetCursorPos();

            if(CurrentEnty->IsFile)
            {
                if(CurrentEnty->Type == FileType::Texture)
                {
                    if(CurrentEnty->Thumbnail)
                    {
                        textureId = CurrentEnty->Thumbnail->GetHandle();
                    }
                    else if(!textureCreated)
                    {
                        textureCreated         = true;
                        CurrentEnty->Thumbnail = m_TextureLibrary.GetResource(CurrentEnty->FullPath.string());
                        textureId              = CurrentEnty->Thumbnail ? CurrentEnty->Thumbnail->GetHandle() : m_FileIcon->GetHandle();
                    }
                    else
                    {
                        textureId = m_FileIcon->GetHandle();
                    }
                }
                else if(CurrentEnty->Type == FileType::Scene)
                {
                    if(CurrentEnty->Thumbnail)
                    {
                        textureId = CurrentEnty->Thumbnail->GetHandle();
                    }
                    else if(!textureCreated)
                    {
                        auto sceneScreenShotPath = m_BasePath + "/Scenes/Cache/" + CurrentEnty->FilePath.stem().string() + ".png";
                        if(std::filesystem::exists(std::filesystem::path(sceneScreenShotPath)))
                        {
                            textureCreated         = true;
                            CurrentEnty->Thumbnail = m_TextureLibrary.GetResource(sceneScreenShotPath);
                            textureId              = CurrentEnty->Thumbnail ? CurrentEnty->Thumbnail->GetHandle() : m_FileIcon->GetHandle();
                        }
                        else
                            textureId = m_FileIcon->GetHandle();
                    }
                    else
                    {
                        textureId = m_FileIcon->GetHandle();
                    }
                }
                else
                {
                    textureId = m_FileIcon->GetHandle();
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
                    m_CopiedPath = (m_BasePath / m_CurrentDir->Children[dirIndex]->FilePath).string();
                    m_CutFile    = true;
                }

                if(ImGui::Selectable("Copy"))
                {
                    m_CopiedPath = (m_BasePath / m_CurrentDir->Children[dirIndex]->FilePath).string();
                    m_CutFile    = false;
                }

                if(ImGui::Selectable("Delete"))
                {
                    auto& fullPath = m_CurrentDir->Children[dirIndex]->FullPath;
                    std::filesystem::remove_all(fullPath);
                    Refresh();
                }

                if(ImGui::Selectable("Duplicate"))
                {
                    std::filesystem::path fullPath        = m_BasePath + "/" + m_CurrentDir->Children[dirIndex]->FilePath.string();
                    std::filesystem::path destinationPath = fullPath;

                    {
                        std::string filename  = fullPath.stem().string();
                        std::string extension = fullPath.extension().string();

                        while(std::filesystem::exists(destinationPath))
                        {
                            filename += "_copy";
                            destinationPath = destinationPath.parent_path() / (filename + extension);
                        }
                    }
                    std::filesystem::copy(fullPath, destinationPath);
                    Refresh();
                }

                if(ImGui::Selectable("Open Location"))
                {
                    auto& fullPath = m_CurrentDir->Children[dirIndex]->FullPath;
                    Lumos::OS::Instance()->OpenFileLocation(fullPath);
                }

                if(m_CurrentDir->Children[dirIndex]->IsFile && ImGui::Selectable("Open External"))
                {
                    auto& fullPath = m_CurrentDir->Children[dirIndex]->FullPath;
                    Lumos::OS::Instance()->OpenFileExternal(fullPath);
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
                    std::filesystem::create_directory(std::filesystem::path(m_CurrentDir->FilePath.string() + "/NewFolder"));
                    Refresh();
                }

                if(!m_IsInListView)
                {
					ImGui::SliderFloat("##GridSize", &m_GridSize, MinGridSize, MaxGridSize);
                }
                ImGui::EndPopup();
            }

            if(ImGui::IsItemHovered() && m_CurrentDir->Children[dirIndex]->Thumbnail)
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted((m_CurrentDir->Children[dirIndex]->FullPath.string()).c_str());
                ImGui::PopTextWrapPos();
                ImGui::Image(m_CurrentDir->Children[dirIndex]->Thumbnail->GetHandle(), { 512, 512 }, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                ImGui::EndTooltip();
            }
            else
                ImGuiUtilities::Tooltip((const char*)(m_CurrentDir->Children[dirIndex]->FullPath.string()).c_str());

            if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                ImGui::TextUnformatted(m_Editor->GetIconFontIcon(m_CurrentDir->Children[dirIndex]->FilePath.string()));

                ImGui::SameLine();
                m_MovePath = m_BasePath + "/" + m_CurrentDir->Children[dirIndex]->FilePath.string();
                ImGui::TextUnformatted(m_MovePath.c_str());
                size_t size = sizeof(const char*) + strlen(m_MovePath.c_str());
                ImGui::SetDragDropPayload("AssetFile", m_MovePath.c_str(), size);
                m_IsDragging = true;
                ImGui::EndDragDropSource();
            }
            if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                doubleClicked = true;
            }

            ImGui::SetCursorPos({ cursorPos.x + padding, cursorPos.y + padding });
            ImGui::SetNextItemAllowOverlap();
            ImGui::Image(reinterpret_cast<ImTextureID>(Graphics::Material::GetDefaultTexture()->GetHandle()), { backgroundThumbnailSize.x - padding * 2.0f, backgroundThumbnailSize.y - padding * 2.0f }, { 0, 0 }, { 1, 1 }, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg) + ImVec4(0.04f, 0.04f, 0.04f, 0.04f));

            ImGui::SetCursorPos({ cursorPos.x + thumbnailPadding * 0.75f, cursorPos.y + thumbnailPadding });
            ImGui::SetNextItemAllowOverlap();
            ImGui::Image(reinterpret_cast<ImTextureID>(textureId), { thumbnailSize, thumbnailSize }, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

            const ImVec2 typeColorFrameSize = { scaledThumbnailSizeX, scaledThumbnailSizeX * 0.03f };
            ImGui::SetCursorPosX(cursorPos.x + padding);
            ImGui::Image(reinterpret_cast<ImTextureID>(Graphics::Material::GetDefaultTexture()->GetHandle()), typeColorFrameSize, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f), !CurrentEnty->IsFile ? ImVec4(0.0f, 0.0f, 0.0f, 0.0f) : CurrentEnty->FileTypeColour);

            const ImVec2 rectMin  = ImGui::GetItemRectMin() + ImVec2(0.0f, 8.0f);
            const ImVec2 rectSize = ImGui::GetItemRectSize() + ImVec2(0.0f, 4.0f);
            const ImRect clipRect = ImRect({ rectMin.x + padding * 2.0f, rectMin.y + padding * 4.0f },
                                           { rectMin.x + rectSize.x, rectMin.y + scaledThumbnailSizeX - ImGui::GetIO().Fonts->Fonts[2]->FontSize - padding * 4.0f });

            ImGuiUtilities::ClippedText(clipRect.Min, clipRect.Max, CurrentEnty->Name.c_str(), nullptr, nullptr, { 0, 0 }, nullptr, clipRect.GetSize().x);

            if(CurrentEnty->IsFile)
            {
                ImGui::SetCursorPos({ cursorPos.x + padding * (float)m_Editor->GetWindow()->GetDPIScale(), cursorPos.y + backgroundThumbnailSize.y - (ImGui::GetIO().Fonts->Fonts[2]->FontSize - padding * (float)m_Editor->GetWindow()->GetDPIScale()) * 3.2f });
                ImGui::BeginDisabled();
                ImGuiUtilities::ScopedFont smallFont(ImGui::GetIO().Fonts->Fonts[2]);
                ImGui::TextUnformatted(CurrentEnty->FileTypeString.data());
                cursorPos = ImGui::GetCursorPos();
                ImGui::SetCursorPos({ cursorPos.x + padding * (float)m_Editor->GetWindow()->GetDPIScale(), cursorPos.y - (ImGui::GetIO().Fonts->Fonts[2]->FontSize * 0.6f - padding * (float)m_Editor->GetWindow()->GetDPIScale()) });
                ImGui::TextUnformatted(CurrentEnty->FileSize.c_str());
                ImGui::EndDisabled();
            }
        }
        else
        {
            ImGui::TextUnformatted(folder ? ICON_MDI_FOLDER : m_Editor->GetIconFontIcon(m_CurrentDir->Children[dirIndex]->FilePath.string()));
            ImGui::SameLine();
            if(ImGui::Selectable(m_CurrentDir->Children[dirIndex]->FilePath.filename().string().c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
            {
                if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    doubleClicked = true;
                }
            }

            ImGuiUtilities::Tooltip((const char*)m_CurrentDir->Children[dirIndex]->FilePath.filename().string().c_str());

            if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                ImGui::TextUnformatted(m_Editor->GetIconFontIcon(m_CurrentDir->Children[dirIndex]->FilePath.string()));

                ImGui::SameLine();
                m_MovePath = m_BasePath + "/" + m_CurrentDir->Children[dirIndex]->FilePath.string();
                ImGui::TextUnformatted(m_MovePath.c_str());
                size_t size = sizeof(const char*) + strlen(m_MovePath.c_str());
                ImGui::SetDragDropPayload("AssetFile", m_MovePath.c_str(), size);
                m_IsDragging = true;
                ImGui::EndDragDropSource();
            }
        }

        if(doubleClicked)
        {
            if(folder)
            {
                m_PreviousDirectory    = m_CurrentDir;
                m_CurrentDir           = m_CurrentDir->Children[dirIndex];
                m_UpdateNavigationPath = true;
            }
            else
            {
                m_Editor->FileOpenCallback(m_BasePath + "/" + m_CurrentDir->Children[dirIndex]->FilePath.string());
            }
        }

        return doubleClicked;
    }

    void ResourcePanel::RenderBottom()
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::BeginChild("##nav", ImVec2(ImGui::GetColumnWidth(), ImGui::GetFontSize() * 1.8f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            int secIdx = 0, newPwdLastSecIdx = -1;

            auto& AssetsDir = m_CurrentDir->FilePath;

            size_t PhysicalPathCount = 0;

            for(auto& sec : m_AssetPath)
            {
                PhysicalPathCount++;
            }
            int dirIndex = 0;
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.2f, 0.7f, 0.0f));

            for(auto& directory : m_BreadCrumbData)
            {
                const std::string& directoryName = directory->FilePath.filename().string();
                if(ImGui::SmallButton(directoryName.c_str()))
                    ChangeDirectory(directory);

                ImGui::SameLine();
            }

            ImGui::PopStyleColor();

            if(newPwdLastSecIdx >= 0)
            {
                int i = 0;
                std::filesystem::path newPwd;
                for(auto& sec : AssetsDir)
                {
                    if(i++ > newPwdLastSecIdx)
                        break;
                    newPwd /= sec;
                }
#ifdef _WIN32
                if(newPwdLastSecIdx == 0)
                    newPwd /= "\\";
#endif

                m_PreviousDirectory    = m_CurrentDir;
                m_CurrentDir           = m_Directories[newPwd.string()];
                m_UpdateNavigationPath = true;
            }

            ImGui::SameLine();
        }

        if(!m_IsInListView)
        {
			ImGui::SliderFloat("##GridSize", &m_GridSize, MinGridSize, MaxGridSize);
        }
        ImGui::EndChild();
    }

    std::vector<std::string> ResourcePanel::SearchFiles(const std::string& query)
    {
        return std::vector<std::string>();
    }

    bool ResourcePanel::MoveFile(const std::string& filePath, const std::string& movePath)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string s = "move " + filePath + " " + movePath.c_str();
#ifndef LUMOS_PLATFORM_IOS
        system(s.c_str());
#endif

        return std::filesystem::exists(std::filesystem::path(movePath + m_Delimiter + StringUtilities::GetFileName(filePath)));
    }

    std::string ResourcePanel::StripExtras(const std::string& filename)
    {
        LUMOS_PROFILE_FUNCTION();
        std::vector<std::string> out;
        size_t start;
        size_t end = 0;

        while((start = filename.find_first_not_of(".", end)) != std::string::npos)
        {
            end = filename.find(".", start);
            out.push_back(filename.substr(start, end - start));
        }

        int maxChars = int(m_GridSize / (ImGui::GetFontSize() * 0.5f));

        if(out[0].length() >= maxChars)
        {
            auto cutFilename = "     " + out[0].substr(0, maxChars - 3) + "...";
            return cutFilename;
        }

        auto filenameLength = out[0].length();
        //        auto paddingToAdd = maxChars - 1 - filenameLength;
        //
        std::string newFileName;
        //
        //        for(int i = 0; i <= paddingToAdd; i++)
        //        {
        //            newFileName += " ";
        //        }

        newFileName += out[0];

        return newFileName;
    }

    void ResourcePanel::OnNewProject()
    {
        Refresh();
    }

    void ResourcePanel::Refresh()
    {
        m_TextureLibrary.Destroy();

        m_BasePath = Application::Get().GetProjectSettings().m_ProjectRoot + "Assets";

        auto& currentPath = m_CurrentDir->FilePath;

        m_UpdateNavigationPath = true;

        m_Directories.clear();
        std::string baseDirectoryHandle = ProcessDirectory(std::filesystem::path(m_BasePath), nullptr);
        m_BaseProjectDir                = m_Directories[baseDirectoryHandle];
        m_PreviousDirectory             = nullptr;
        m_CurrentDir                    = nullptr;

        if(m_Directories.find(currentPath.string()) != m_Directories.end())
            m_CurrentDir = m_Directories[currentPath.string()];
        else
            ChangeDirectory(m_BaseProjectDir);

        std::string assetsBasePath;
        VFS::Get().ResolvePhysicalPath("//Assets", assetsBasePath);
        m_AssetPath = std::filesystem::path(assetsBasePath);
    }
}
