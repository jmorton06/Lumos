#include "Editor.h"
#include "ResourcePanel.h"
#include <Lumos/Core/OS/FileSystem.h>
#include <Lumos/Core/Profiler.h>
#include <Lumos/Core/StringUtilities.h>
#include <Lumos/Core/VFS.h>

#if __has_include(<filesystem>)
#include <filesystem>
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
#endif

#include <Lumos/ImGui/IconsMaterialDesignIcons.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace Lumos
{

#ifdef LUMOS_PLATFORM_WINDOWS
    std::string AssetWindow::m_Delimiter = "\\";
#else
    std::string ResourcePanel::m_Delimiter = "/";
#endif
    ResourcePanel::ResourcePanel()
    {
        LUMOS_PROFILE_FUNCTION();
        m_Name = "ResourceWindow";
        m_SimpleName = "Resources";

        //TODO: Get Project path from editor
#ifdef LUMOS_PLATFORM_IOS
        m_BaseDirPath = "Assets";
#else
//        m_BaseDirPath = m_Editor->GetProjectRoot() + "/ExampleProject/Assets";
        m_BaseDirPath = ROOT_DIR "/ExampleProject/Assets";
#endif
        m_CurrentDirPath = m_BaseDirPath;
        m_PreviousDirPath = m_CurrentDirPath;
        m_LastNavPath = m_BaseDirPath;
        m_BaseProjectDir = GetFsContents(m_BaseDirPath);
        m_CurrentDir = m_BaseProjectDir;
        m_BasePathLen = strlen(m_BaseDirPath.c_str());

        m_IsDragging = false;
        m_IsInListView = true;
        m_UpdateBreadCrumbs = true;
        m_ShowHiddenFiles = false;
    }

    void ResourcePanel::DrawFolder(const DirectoryInformation& dirInfo)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGuiTreeNodeFlags nodeFlags = ((dirInfo.absolutePath == m_CurrentDirPath) ? ImGuiTreeNodeFlags_Selected : 0);
        nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

        const ImColor TreeLineColor = ImColor(128, 128, 128, 128);
        const float SmallOffsetX = 6.0f * Application::Get().GetWindowDPI();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        if(!dirInfo.isFile)
        {
            auto dirData = ReadDirectory(dirInfo.absolutePath.c_str());

            bool containsFolder = false;

            for(auto& file : dirData)
            {
                if(!file.isFile)
                {
                    containsFolder = true;
                    break;
                }
            }
            if(!containsFolder)
                nodeFlags |= ImGuiTreeNodeFlags_Leaf;

            static std::string folderIcon = ICON_MDI_FOLDER " ";

            bool isOpen = ImGui::TreeNodeEx((folderIcon + dirInfo.filename).c_str(), nodeFlags);

            ImVec2 verticalLineStart = ImGui::GetCursorScreenPos();

            if(ImGui::IsItemClicked())
            {
                m_PreviousDirPath = GetParentPath(m_CurrentDirPath);
                m_CurrentDirPath = dirInfo.absolutePath;
                m_CurrentDir = dirData;
            }

            if(isOpen && containsFolder)
            {
                verticalLineStart.x += SmallOffsetX; //to nicely line up with the arrow symbol
                ImVec2 verticalLineEnd = verticalLineStart;
                float HorizontalTreeLineSize = 16.0f * Application::Get().GetWindowDPI(); //chosen arbitrarily

                for(int i = 0; i < dirData.size(); i++)
                {
                    if(!dirData[i].isFile)
                    {
                        auto currentPos = ImGui::GetCursorScreenPos();

                        ImGui::Indent(10.0f);

                        auto dirDataTemp = ReadDirectory(dirData[i].absolutePath.c_str());

                        bool containsFolderTemp = false;
                        for(auto& file : dirDataTemp)
                        {
                            if(!file.isFile)
                            {
                                containsFolderTemp = true;
                                break;
                            }
                        }
                        if(containsFolderTemp)
                            HorizontalTreeLineSize *= 0.5f;
                        DrawFolder(dirData[i]);

                        const ImRect childRect = ImRect(currentPos, currentPos + ImVec2(0.0f, ImGui::GetFontSize()));

                        const float midpoint = (childRect.Min.y + childRect.Max.y) / 2.0f;
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
            m_MovePath = dirInfo.absolutePath.c_str();
        }
    }

    void ResourcePanel::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::Begin(m_SimpleName.c_str());
        {
            ImGui::BeginColumns("AssetWindowColumns", 2, ImGuiOldColumnFlags_NoResize);
            ImGui::SetColumnWidth(0, ImGui::GetWindowContentRegionWidth() / 3.0f);
            ImGui::BeginChild("##folders_common");
            {
                RenderBreadCrumbs();

                ImGuiTreeNodeFlags nodeFlag = ImGuiTreeNodeFlags_DefaultOpen;
                bool assetTreeNodeOpen = ImGui::TreeNodeEx("//Assets", nodeFlag);
                
                if(ImGui::IsItemHovered())
                {
                    std::string fullPath;
                    VFS::Get()->ResolvePhysicalPath("//Assets", fullPath);
                    ImGuiHelpers::Tooltip(fullPath);
                }

                if(ImGui::IsItemClicked())
                {
                    std::string assetsBasePath;
                    VFS::Get()->ResolvePhysicalPath("//Assets", assetsBasePath);
                    m_PreviousDirPath = GetParentPath(m_CurrentDirPath);
                    m_CurrentDirPath = assetsBasePath;
                    m_CurrentDir = ReadDirectory(m_CurrentDirPath);
                }

                if(assetTreeNodeOpen)
                {
                    ImGui::BeginChild("##folders");
                    {
                        for(int i = 0; i < m_BaseProjectDir.size(); i++)
                            DrawFolder(m_BaseProjectDir[i]);
                    }
                    ImGui::EndChild();
                    ImGui::TreePop();
                }

                if(ImGui::IsMouseDown(1))
                {
                    ImGui::OpenPopup("window");
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
                        m_CurrentDir = ReadDirectory(m_CurrentDirPath);
                    }
                    m_IsDragging = false;
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::NextColumn();

            ImGui::BeginChild("##directory_structure");
            {
                {
                    ImGui::BeginChild("##directory_breadcrumbs", ImVec2(ImGui::GetColumnWidth(), ImGui::GetFontSize() * 2.0f));
                    if(m_IsInListView)
                    {
                        if(ImGui::Button(ICON_MDI_VIEW_GRID))
                        {
                            m_IsInListView = !m_IsInListView;
                        }
                        ImGui::SameLine();
                    }
                    else
                    {
                        if(ImGui::Button(ICON_MDI_VIEW_LIST))
                        {
                            m_IsInListView = !m_IsInListView;
                        }
                        ImGui::SameLine();
                    }

                    ImGui::TextUnformatted(ICON_MDI_MAGNIFY);

                    ImGui::SameLine();

                    m_Filter.Draw("##Filter", ImGui::GetContentRegionAvail().x - ImGui::GetStyle().IndentSpacing);

                    ImGui::EndChild();
                }

                {
                    ImGui::BeginChild("##Scrolling");

                    int shownIndex = 0;

                    float xAvail = ImGui::GetContentRegionAvail().x;
                    
                    m_GridItemsPerRow = (int)floor(xAvail / (m_GridSize + ImGui::GetStyle().ItemSpacing.x));
                    m_GridItemsPerRow = Maths::Max(1, m_GridItemsPerRow);

                    if(m_IsInListView)
                    {
                        for(int i = 0; i < m_CurrentDir.size(); i++)
                      {
                          if(m_CurrentDir.size() > 0)
                          {
                              if(!m_ShowHiddenFiles && Lumos::StringUtilities::IsHiddenFile(m_CurrentDir[i].filename))
                              {
                                  continue;
                              }
  
                              if(m_Filter.IsActive())
                              {
                                  if(!m_Filter.PassFilter(m_CurrentDir[i].filename.c_str()))
                                  {
                                      continue;
                                  }
                              }
  
                              bool doubleClicked = RenderFile(i, !m_CurrentDir[i].isFile, shownIndex, !m_IsInListView);
  
                              if(doubleClicked)
                                  break;
                              shownIndex++;
                          }
                      }
                        
                    }
                    else
                    {
                               for(int i = 0; i < m_CurrentDir.size(); i++)
                             {
                                 if(m_CurrentDir.size() > 0)
                                 {
                                     if(!m_ShowHiddenFiles && Lumos::StringUtilities::IsHiddenFile(m_CurrentDir[i].filename))
                                     {
                                         continue;
                                     }
         
                                     if(m_Filter.IsActive())
                                     {
                                         if(!m_Filter.PassFilter(m_CurrentDir[i].filename.c_str()))
                                         {
                                             continue;
                                         }
                                     }
         
                                     bool doubleClicked = RenderFile(i, !m_CurrentDir[i].isFile, shownIndex, !m_IsInListView);
         
                                     if(doubleClicked)
                                         break;
                                     shownIndex++;
                                 }
                       }
                    }
                    ImGui::EndChild();
                }
                ImGui::EndChild();
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

            if(ImGui::BeginMenuBar())
            {
                if(ImGui::BeginMenu("Create"))
                {
                    if(ImGui::MenuItem("Import New Asset", "Ctrl + O"))
                    {
                        m_Editor->OpenFile();
                    }

                    if(ImGui::MenuItem("Refresh", "Ctrl + R"))
                    {
                        auto data = GetFsContents(m_BaseDirPath);
                        for(int i = 0; i < data.size(); i++)
                        {
                            LUMOS_LOG_INFO(data[i].filename);
                        }
                    }

                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            
            ImGui::End();
        }
    }

    void ResourcePanel::RenderBreadCrumbs()
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::BeginChild("##directory_breadcrumbs", ImVec2(ImGui::GetColumnWidth(), ImGui::GetFontSize() * 2.0f));
        {
            if(ImGui::Button(ICON_MDI_ARROW_LEFT))
            {
                if(strlen(m_CurrentDirPath.c_str()) != m_BasePathLen)
                {
                    m_PreviousDirPath = GetParentPath(m_CurrentDirPath);
                    m_CurrentDirPath = m_PreviousDirPath;
                    m_CurrentDir = ReadDirectory(m_CurrentDirPath);
                }
            }
            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_ARROW_RIGHT))
            {
                m_PreviousDirPath = GetParentPath(m_CurrentDirPath);
                m_CurrentDirPath = m_LastNavPath;
                m_CurrentDir = ReadDirectory(m_LastNavPath);
            }
            ImGui::SameLine();

            GetDirectories(m_CurrentDirPath);

            int secIdx = 0, newPwdLastSecIdx = -1;

            std::string assetsBasePath;
            VFS::Get()->ResolvePhysicalPath("//Assets", assetsBasePath);
            auto dir = std::filesystem::path(assetsBasePath);

            auto AssetsDir = std::filesystem::path(m_CurrentDirPath);

            size_t PhysicalPathCount = 0;

            for(auto& sec : dir)
            {
                PhysicalPathCount++;
            }


            int dirIndex = 0;

            for(auto& sec : AssetsDir)
            {
                if(dirIndex < PhysicalPathCount - 1)
                {
                    dirIndex++;
                    secIdx++;
                    continue;
                }

                dirIndex++;
#ifdef _WIN32
                if(secIdx == 1)
                {
                    secIdx++;
                    continue;
                }
#endif
                if(!sec.u8string().empty())
                {
                    ImGui::PushID(secIdx);
                    //if(secIdx > 0)
                    ImGui::SameLine();
                    if(ImGui::SmallButton(sec.u8string().c_str()))
                    {
                        newPwdLastSecIdx = secIdx;
                    }
                    ImGui::PopID();
                }

                secIdx++;
            }

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

                m_PreviousDirPath = GetParentPath(m_CurrentDirPath);
                m_CurrentDirPath = newPwd.string();
                m_CurrentDir = ReadDirectory(m_CurrentDirPath);
            }

            ImGui::SameLine();
        }

        ImGui::EndChild();
    }

    bool ResourcePanel::RenderFile(int dirIndex, bool folder, int shownIndex, bool gridView)
    {
        LUMOS_PROFILE_FUNCTION();
        auto fileID = GetParsedAssetID(m_CurrentDir[dirIndex].fileType);

        bool doubleClicked = false;

        if(gridView)
        {
            ImGui::BeginGroup();

            auto fileID = GetParsedAssetID(m_CurrentDir[dirIndex].fileType);

            if(ImGui::Button(folder ? ICON_MDI_FOLDER : m_Editor->GetIconFontIcon(m_CurrentDir[dirIndex].absolutePath), ImVec2(m_GridSize, m_GridSize)))
            {
            }

            if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                doubleClicked = true;
            }

            auto& fname = m_CurrentDir[dirIndex].filename;
            auto newFname = StripExtras(fname);

            ImGui::TextUnformatted(newFname.c_str());
            ImGui::EndGroup();
            
            if((shownIndex + 1) % m_GridItemsPerRow != 0)
                ImGui::SameLine();
        }
        else
        {
            ImGui::TextUnformatted(folder ? ICON_MDI_FOLDER : m_Editor->GetIconFontIcon(m_CurrentDir[dirIndex].absolutePath));
            ImGui::SameLine();
            if(ImGui::Selectable(m_CurrentDir[dirIndex].filename.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
            {
                if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    doubleClicked = true;
                }
            }
        }

        if(doubleClicked)
        {
            if(folder)
            {
                m_PreviousDirPath = m_CurrentDir[dirIndex].absolutePath;
                m_CurrentDirPath = m_CurrentDir[dirIndex].absolutePath;
                m_CurrentDir = ReadDirectory(m_CurrentDir[dirIndex].absolutePath);
            }

            else
            {
                m_Editor->FileOpenCallback(m_CurrentDir[dirIndex].absolutePath);
            }
        }

        if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            ImGui::TextUnformatted(m_Editor->GetIconFontIcon(m_CurrentDir[dirIndex].absolutePath));

            ImGui::SameLine();
            ImGui::TextUnformatted(m_CurrentDir[dirIndex].filename.c_str());
            size_t size = sizeof(const char*) + strlen(m_CurrentDir[dirIndex].absolutePath.c_str());
            ImGui::SetDragDropPayload("AssetFile", m_CurrentDir[dirIndex].absolutePath.c_str(), size);
            m_IsDragging = true;
            ImGui::EndDragDropSource();
        }

        return doubleClicked;
    }

    void ResourcePanel::RenderBottom()
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::BeginChild("##nav", ImVec2(ImGui::GetColumnWidth() - 12, ImGui::GetFontSize() * 1.8f));
        {
            ImGui::TextUnformatted("Grid Size");
            ImGui::DragFloat("#GridSize", &m_GridSize, 1.0f, 40.0f, 400.0f);
            ImGui::EndChild();
        }
    }

    std::vector<DirectoryInformation> ResourcePanel::GetFsContents(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        std::vector<DirectoryInformation> dInfo;

        for(const auto& entry : std::filesystem::directory_iterator(path))
        {
            if(Lumos::StringUtilities::IsHiddenFile(entry.path().string()))
            {
                break;
            }

            bool isDir = std::filesystem::is_directory(entry);

            auto dir_data = StringUtilities::GetFileName(entry.path().string());
            ;
            auto fileExt = StringUtilities::GetFilePathExtension(dir_data);

            DirectoryInformation d(dir_data, fileExt, entry.path().string(), !isDir);
            dInfo.push_back(d);
        }

        return dInfo;
    }

    std::vector<DirectoryInformation> ResourcePanel::ReadDirectory(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        std::vector<DirectoryInformation> dInfo;

        for(const auto& entry : std::filesystem::directory_iterator(path))
        {
            bool isDir = std::filesystem::is_directory(entry);
            auto dir_data = StringUtilities::GetFileName(entry.path().string());
            auto fileExt = StringUtilities::GetFilePathExtension(dir_data);

            DirectoryInformation d(dir_data, fileExt, entry.path().string(), !isDir);
            dInfo.push_back(d);
        }

        return dInfo;
    }

    std::vector<DirectoryInformation> ResourcePanel::ReadDirectoryRecursive(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        std::vector<DirectoryInformation> dInfo;

        for(const auto& entry : std::filesystem::recursive_directory_iterator(path))
        {
            bool isDir = std::filesystem::is_directory(entry);
            auto dir_data = StringUtilities::GetFileName(entry.path().string());
            DirectoryInformation d(dir_data, ".Lumos", entry.path().string(), !isDir);
        }

        return dInfo;
    }

    std::string ResourcePanel::GetParentPath(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        auto p = std::filesystem::path(path);
        return p.parent_path().string();
    }

    void ResourcePanel::GetDirectories(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        m_DirectoryCount = 0;
        size_t start;
        size_t end = 0;

        while((start = path.find_first_not_of(m_Delimiter.c_str(), end)) != std::string::npos)
        {
            end = path.find(m_Delimiter.c_str(), start);
            m_Directories[m_DirectoryCount] = path.substr(start, end - start);
            m_DirectoryCount++;
        }
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

        if(out[0].length() >= 8)
        {
            auto cutFilename = "     " + out[0].substr(0, 5) + "...";
            return cutFilename;
        }

        auto filenameLength = out[0].length();
        auto paddingToAdd = 9 - filenameLength;

        std::string newFileName;

        for(int i = 0; i <= paddingToAdd; i++)
        {
            newFileName += " ";
        }

        newFileName += out[0];

        return newFileName;
    }
}
