#include "ProjectSettingsPanel.h"
#include "Editor.h"
#include <Lumos/Core/Profiler.h>
#include <Lumos/Scene/SceneManager.h>
#include <Lumos/Core/String.h>
#include <Lumos/Core/Thread.h>
#include <Lumos/Maths/Vector4.h>
#if __has_include(<filesystem>)
#include <filesystem>
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
#endif

namespace Lumos
{
    ProjectSettingsPanel::ProjectSettingsPanel()
    {
        m_Name       = "Project Settings###projectsettings";
        m_SimpleName = "Project Settings";
    }

    static bool BeginSection(const char* label)
    {
        ImGui::Columns(1);
        ImGui::Spacing();
        bool open = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen);
        if(open)
        {
            ImGui::Indent(8.0f);
            ImGui::Columns(2);
        }
        return open;
    }

    static void EndSection(bool wasOpen)
    {
        if(wasOpen)
        {
            ImGui::Columns(1);
            ImGui::Unindent(8.0f);
        }
    }

    static void LabelColumn(const char* text, const char* tooltip = nullptr)
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(text);
        if(tooltip && ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", tooltip);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
    }

    static void EndProperty()
    {
        ImGui::PopItemWidth();
        ImGui::NextColumn();
    }

    void ProjectSettingsPanel::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();

        if(!ImGuiUtilities::BeginPanel(m_Name.c_str(), nullptr, 0))
        {
            ImGui::End();
            return;
        }

        ImGuiUtilities::PushID();

        auto& projectSettings = Application::Get().GetProjectSettings();

        // Header — quick actions
        {
            ImGui::Spacing();
            if(ImGui::Button("Save"))
                Application::Get().Serialise();
            ImGui::SameLine();
            ImGui::TextDisabled("project version %d", projectSettings.ProjectVersion);
            ImGui::Separator();
        }

        // ---------------- General ----------------
        {
            bool open = BeginSection("General");
            if(open)
            {
                auto projectName = m_NameUpdated ? m_ProjectName : projectSettings.m_ProjectName;
                if(ImGuiUtilities::Property("Project Name", projectName, ImGuiUtilities::PropertyFlag::None))
                {
                    m_NameUpdated = true;
                    m_ProjectName = projectName;
                }
                if(!ImGui::IsItemActive() && m_NameUpdated)
                {
                    m_NameUpdated   = false;
                    auto fullPath   = projectSettings.m_ProjectRoot + projectSettings.m_ProjectName + std::string(".lmproj");
                    if(std::filesystem::exists(fullPath))
                    {
                        projectSettings.m_ProjectName = m_ProjectName;
                        std::filesystem::rename(fullPath, projectSettings.m_ProjectRoot + projectSettings.m_ProjectName + std::string(".lmproj"));
                    }
                    else
                        projectSettings.m_ProjectName = m_ProjectName;
                }

                ImGuiUtilities::Property("Title", projectSettings.Title);
                ImGuiUtilities::PropertyConst("Project Root", projectSettings.m_ProjectRoot.c_str());
                ImGuiUtilities::PropertyConst("Engine Asset Path", projectSettings.m_EngineAssetPath.c_str());
                ImGuiUtilities::Property("Project Version", projectSettings.ProjectVersion, 0, 0, ImGuiUtilities::PropertyFlag::ReadOnly);
                ImGuiUtilities::Property("Render API", projectSettings.RenderAPI, 0, 1);
                ImGuiUtilities::Property("Auto Import Meshes", projectSettings.AutoImportMeshes);

                // Start Scene dropdown
                LabelColumn("Start Scene", "Scene loaded on launch. Default = first scene in the list.");
                {
                    ArenaTemp scratch = ScratchBegin(0, 0);
                    auto sceneNames   = Application::Get().GetSceneManager()->GetSceneNames(scratch.arena);

                    std::string currentStart = projectSettings.StartScene;
                    if(currentStart.empty())
                        currentStart = "(Default — first scene)";

                    if(ImGui::BeginCombo("##StartScene", currentStart.c_str()))
                    {
                        if(ImGui::Selectable("(Default — first scene)", projectSettings.StartScene.empty()))
                            projectSettings.StartScene.clear();
                        for(size_t i = 0; i < sceneNames.Size(); i++)
                        {
                            const char* name = (const char*)sceneNames[i].str;
                            bool isSelected  = (projectSettings.StartScene == name);
                            if(ImGui::Selectable(name, isSelected))
                                projectSettings.StartScene = name;
                            if(isSelected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    ScratchEnd(scratch);
                }
                EndProperty();
            }
            EndSection(open);
        }

        // ---------------- Window ----------------
        {
            bool open = BeginSection("Window");
            if(open)
            {
                ImGuiUtilities::Property("App Width", (int&)projectSettings.Width, 0, 0, ImGuiUtilities::PropertyFlag::ReadOnly);
                ImGuiUtilities::Property("App Height", (int&)projectSettings.Height, 0, 0, ImGuiUtilities::PropertyFlag::ReadOnly);
                ImGuiUtilities::Property("Fullscreen", projectSettings.Fullscreen);
                ImGuiUtilities::Property("VSync", projectSettings.VSync);
                ImGuiUtilities::Property("Borderless", projectSettings.Borderless);
                ImGuiUtilities::Property("Hide Title Bar", projectSettings.HideTitleBar);
                ImGuiUtilities::Property("Show Console", projectSettings.ShowConsole);
            }
            EndSection(open);
        }

        // ---------------- Distribution ----------------
        {
            bool open = BeginSection("Distribution");
            if(open)
            {
                ImGuiUtilities::Property("Bundle Identifier", projectSettings.BundleIdentifier);
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("Reverse-DNS, e.g. com.studio.mygame");
                ImGuiUtilities::Property("Version", projectSettings.Version);
                ImGuiUtilities::Property("Build Number", projectSettings.BuildNumber);
                ImGuiUtilities::Property("Icon Path", projectSettings.IconPath);
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("VFS path to source icon — e.g. //Assets/Textures/icon.png");
                ImGuiUtilities::Property("Splash Image", projectSettings.SplashPath);
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("Optional custom splash screen image (VFS path)");

                // Splash background colour
                Vec4 splashColour(projectSettings.SplashBGColour[0],
                                  projectSettings.SplashBGColour[1],
                                  projectSettings.SplashBGColour[2],
                                  projectSettings.SplashBGColour[3]);
                if(ImGuiUtilities::Property("Splash Background", splashColour, -1.0f, 1.0f, true, ImGuiUtilities::PropertyFlag::ColourProperty))
                {
                    projectSettings.SplashBGColour[0] = splashColour.x;
                    projectSettings.SplashBGColour[1] = splashColour.y;
                    projectSettings.SplashBGColour[2] = splashColour.z;
                    projectSettings.SplashBGColour[3] = splashColour.w;
                }
            }
            EndSection(open);
        }

        // ---------------- Mobile ----------------
        {
            bool open = BeginSection("Mobile (iOS / Android)");
            if(open)
            {
                LabelColumn("Orientation", "Maps to UISupportedInterfaceOrientations in iOS Info.plist");
                {
                    const char* orientations[] = { "All", "Portrait Only", "Landscape Only" };
                    int orient = projectSettings.Orientation;
                    if(orient < 0 || orient > 2) orient = 0;
                    if(ImGui::Combo("##Orientation", &orient, orientations, IM_ARRAYSIZE(orientations)))
                        projectSettings.Orientation = orient;
                }
                EndProperty();

                LabelColumn("Device Family", "TARGETED_DEVICE_FAMILY");
                {
                    const char* families[] = { "iPhone", "iPad", "Both" };
                    int fam    = projectSettings.DeviceFamily;
                    int famIdx = (fam == 1) ? 0 : (fam == 2 ? 1 : 2);
                    if(ImGui::Combo("##DeviceFamily", &famIdx, families, IM_ARRAYSIZE(families)))
                        projectSettings.DeviceFamily = (famIdx == 0) ? 1 : (famIdx == 1 ? 2 : 3);
                }
                EndProperty();

                ImGuiUtilities::Property("Min iOS Version", projectSettings.MinIOSVersion);
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("IPHONEOS_DEPLOYMENT_TARGET, e.g. 16.0");
                ImGuiUtilities::Property("Status Bar Hidden", projectSettings.StatusBarHidden);
                LabelColumn("Safe Area", "Notch / home-indicator layout");
                {
                    const char* modes[] = { "Fullscreen", "Safe Area", "Fullscreen + Safe UI" };
                    int mode = projectSettings.SafeAreaMode;
                    if(mode < 0 || mode > 2) mode = (int)Application::SafeAreaLayout::FullscreenSafeUI;
                    if(ImGui::Combo("##SafeAreaMode", &mode, modes, IM_ARRAYSIZE(modes)))
                        projectSettings.SafeAreaMode = mode;
                    if(ImGui::IsItemHovered())
                        ImGui::SetTooltip("Fullscreen: scene + UI edge-to-edge, under system UI.\n"
                                          "Safe Area: scene render AND UI inset (black bars).\n"
                                          "Fullscreen + Safe UI: scene edge-to-edge, only UI respects insets.");
                }
                EndProperty();
                ImGuiUtilities::Property("Non-Exempt Encryption", projectSettings.UsesNonExemptEncryption);
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("ITSAppUsesNonExemptEncryption. Leave off unless you ship custom encryption.");
            }
            EndSection(open);
        }

        // ---------------- Privacy (iOS) ----------------
        {
            bool open = BeginSection("Privacy Usage (iOS)");
            if(open)
            {
                ImGui::Columns(1);
                ImGui::TextDisabled("Leave blank to omit the Info.plist key.");
                ImGui::TextDisabled("Required when the matching API is used.");
                ImGui::Columns(2);
                ImGuiUtilities::Property("Camera", projectSettings.CameraUsage);
                ImGuiUtilities::Property("Microphone", projectSettings.MicrophoneUsage);
                ImGuiUtilities::Property("Photo Library", projectSettings.PhotoLibraryUsage);
                ImGuiUtilities::Property("Location (When In Use)", projectSettings.LocationUsage);
            }
            EndSection(open);
        }

        // ---------------- Scripting ----------------
        {
            bool open = BeginSection("Scripting");
            if(open)
            {
                ImGuiUtilities::Property("App Script", projectSettings.AppScript);
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("VFS path to a Lua script auto-attached to the start scene (tool/runtime only)");
            }
            EndSection(open);
        }

        ImGui::Columns(1);
        ImGuiUtilities::PopID();
        ImGui::End();
    }
}
