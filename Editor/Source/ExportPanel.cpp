#include "ExportPanel.h"
#include "Editor.h"
#include <Lumos/Core/Application.h>
#include <Lumos/Core/OS/OS.h>
#include <Lumos/Core/OS/Input.h>
#include <Lumos/Core/Engine.h>
#include <Lumos/Utilities/TimeStep.h>
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>
#include <Lumos/Export/ExportIcons.h>
#include <Lumos/Maths/MathsUtilities.h>
#include <Lumos/Core/ToolPanels.h>
#include <Lumos/Graphics/UI.h>
#include <Lumos/Graphics/UIDock.h>
#include <Lumos/Graphics/UITheme.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>

namespace Lumos
{
    using namespace Export;

    namespace
    {
        // "//Assets/x.png" when inside the project, so presets survive moving the checkout
        std::string ProjectRelative(const ProjectInfo& project, const std::string& absolute)
        {
            if(project.Root.empty() || absolute.compare(0, project.Root.size(), project.Root) != 0)
                return absolute;
            std::string rest = absolute.substr(project.Root.size());
            return rest.compare(0, 7, "Assets/") == 0 ? "//" + rest : rest;
        }

        std::string HomeRelative(const std::string& path)
        {
            const char* home = getenv("HOME");
            if(home && *home && path.compare(0, strlen(home), home) == 0)
                return "~" + path.substr(strlen(home));
            return path;
        }

        std::string Quote(const std::string& s) { return s.find(' ') == std::string::npos ? s : "\"" + s + "\""; }
    }

    ExportPanel::ExportPanel()
    {
        m_Name       = ICON_MDI_EXPORT " Export###export";
        m_SimpleName = "Export";
    }

    ExportPanel::~ExportPanel()
    {
        if(m_Job)
            m_Job->Cancel();
        if(m_Thread.joinable())
            m_Thread.join();
        if(m_DeviceThread.joinable())
            m_DeviceThread.join();
    }

    void ExportPanel::OnNewProject() { Reload(); }

    void ExportPanel::Reload()
    {
        auto& settings = Application::Get().GetProjectSettings();
        m_HaveProject  = !settings.m_ProjectRoot.empty() && m_Project.Load(settings.m_ProjectRoot + settings.m_ProjectName + ".lmproj");
        m_Presets.Presets.Clear();
        if(m_HaveProject && !m_Presets.Load(m_Project))
            m_Presets.AddDefaults(m_Project); // saved on the first edit / export
        m_Selected         = 0;
        m_PickedIconPreset = -1;
    }

    void ExportPanel::Save()
    {
        if(m_HaveProject)
            m_Presets.Save(m_Project);
    }

    std::string ExportPanel::UniqueName(const std::string& base, int ignore) const
    {
        std::string name = base;
        for(int n = 2;; n++)
        {
            bool taken = false;
            for(u32 i = 0; i < m_Presets.Presets.Size(); i++)
                taken |= (int)i != ignore && m_Presets.Presets[i].Name == name;
            if(!taken)
                return name;
            name = base + " " + std::to_string(n);
        }
    }

    void ExportPanel::RefreshDevices()
    {
        if(m_DeviceState == DeviceState::Loading)
            return;
        if(m_DeviceThread.joinable())
            m_DeviceThread.join();
        m_DeviceState  = DeviceState::Loading;
        m_DeviceThread = std::thread([this]
            {
                m_DevicesPending = ListIOSDevices();
                m_DeviceState    = DeviceState::Ready;
            });
    }

    void ExportPanel::UpdateIcon(const std::string& png)
    {
        // picks up the file being replaced on disk too
        double now      = Engine::GetTimeStep().GetElapsedSeconds();
        long long stamp = m_IconStamp;
        if(png != m_IconTexturePath || now - m_IconChecked > 1.0)
        {
            std::error_code ec;
            m_IconChecked = now;
            stamp         = png.empty() ? 0 : (long long)std::filesystem::last_write_time(png, ec).time_since_epoch().count();
        }
        if(png == m_IconTexturePath && stamp == m_IconStamp)
            return;
        m_IconTexturePath = png;
        m_IconStamp       = stamp;
        m_IconTexture     = SharedPtr<Graphics::Texture2D>();
        m_IconW = m_IconH = 0;
        m_IconTransparent = false;
        if(png.empty() || !Icons::Inspect(png, m_IconW, m_IconH, m_IconTransparent))
            return;
        Graphics::TextureDesc desc;
        desc.minFilter = Graphics::TextureFilter::LINEAR;
        desc.magFilter = Graphics::TextureFilter::LINEAR;
        desc.wrap      = Graphics::TextureWrap::CLAMP_TO_EDGE;
        m_IconTexture  = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile("ExportIcon", png, desc));
    }

    void ExportPanel::BrowseIcon()
    {
            m_BrowsePreset = m_Selected;
            const std::string start = m_IconTexturePath.empty() ? m_Project.Root + "Assets" : std::filesystem::path(m_IconTexturePath).parent_path().string();
            UIFileDialogDesc dialog;
            dialog.Title       = "Choose an Icon";
            dialog.StartFolder = start.c_str();
            dialog.Extension   = ".png";
            dialog.Accept      = "Choose";
            dialog.OnPicked    = [](const char* picked, void* user)
            {
                ExportPanel* panel        = (ExportPanel*)user;
                panel->m_PickedIcon       = picked;
                panel->m_PickedIconPreset = panel->m_BrowsePreset;
            };
            dialog.User = this;
            UIOpenFileDialog(dialog);
            return;
    }

    void ExportPanel::Launch(std::unique_ptr<Job> job)
    {
        if(m_Thread.joinable())
            m_Thread.join();
        m_Job       = std::move(job);
        m_LogShown  = 0;
        m_FollowLog = true;
        Job* j      = m_Job.get();
        m_Thread    = std::thread([j] { j->Execute(); });
    }

    void ExportPanel::Start(Job::Kind kind)
    {
        if(m_Selected < 0 || m_Selected >= (int)m_Presets.Presets.Size())
            return;
        // the exporter reads the .lmproj from disk: flush what Project Settings holds
        Application::Get().Serialise();
        m_Project.Load(m_Project.Lmproj);
        Save();
        Job::Options options;
        options.JobKind = kind;
        options.Device  = m_Presets.Presets[m_Selected].Device;
        Launch(std::make_unique<Job>(m_Project, m_Presets.Presets[m_Selected], options));
    }

    void ExportPanel::StartTemplateBuild(Platform platform, bool release) { Launch(std::make_unique<Job>(platform, release)); }

    static bool JobRunning(const std::unique_ptr<Job>& job) { return job && !job->Finished(); }

    std::string ExportPanel::CommandFor(const ExportPreset& preset, Job::Kind kind) const
    {
        std::string cmd = kind == Job::Kind::Deploy ? "lumos deploy" : "lumos export";
        cmd += " --project=" + Quote(m_Project.Root.substr(0, m_Project.Root.size() - 1)) + " --preset=" + Quote(preset.Name);
        if(kind == Job::Kind::ExportAndRun)
            cmd += " --run";
        return cmd;
    }


    // ---- engine UI ---------------------------------------------------------

    namespace
    {
        // only glyphs the engine font has
        const char* UIPlatformIcon(Platform platform)
        {
            return platform == Platform::IOS ? ICON_MDI_CELLPHONE : ICON_MDI_EXPORT;
        }

        // text row over a std::string; the fallback shows as the placeholder
        bool UIField(const char* label, std::string& value, const std::string& hint)
        {
            char buffer[512];
            snprintf(buffer, sizeof(buffer), "%s", value.c_str());
            if(!UIPropertyText(label, buffer, sizeof(buffer), hint.c_str()))
                return false;
            value = buffer;
            return true;
        }

        void UINote(const char* id, const std::string& text, const Vec4& colour, float widthLogical)
        {
            UIPushStyle(StyleVar_TextColor, colour);
            UILabelWrapped(id, Str8C((char*)text.c_str()), widthLogical);
            UIPopStyle(StyleVar_TextColor);
        }

        // greyed and inert when it cannot be used
        bool UIAction(const char* label, bool enabled = true)
        {
            if(!enabled)
                UIPushStyle(StyleVar_TextColor, UIGetToolTheme().TextDim);
            const bool pressed = UIButton(label).clicked;
            if(!enabled)
                UIPopStyle(StyleVar_TextColor);
            UISpacer(4.0f);
            return pressed && enabled;
        }

        bool IsPng(const char* path)
        {
            const size_t n = strlen(path);
            return n > 4 && (strcmp(path + n - 4, ".png") == 0 || strcmp(path + n - 4, ".PNG") == 0);
        }
    }

    bool ExportPanel::CheckProject()
    {
        auto& settings = Application::Get().GetProjectSettings();
        if(!m_HaveProject || m_Project.Root.empty() || settings.m_ProjectRoot.empty() || std::filesystem::path(m_Project.Root).lexically_normal() != std::filesystem::path(settings.m_ProjectRoot).lexically_normal())
            Reload();

        if(m_HaveProject && m_PickedIconPreset >= 0)
        {
            if(m_PickedIconPreset < (int)m_Presets.Presets.Size())
            {
                m_Presets.Presets[m_PickedIconPreset].IconPath = ProjectRelative(m_Project, m_PickedIcon);
                Save();
            }
            m_PickedIconPreset = -1;
        }
        return m_HaveProject;
    }

    void ExportPanel::OnEditorUI(const UIDockPanelContext& context)
    {
        const UIToolTheme& theme = UIGetToolTheme();
        const float dpi          = context.DPIScale;
        const float width        = Maths::Max(120.0f, context.Size.x / dpi - 24.0f);

        if(!CheckProject())
        {
            UINote("##exportnone", "Open a project to export it.", theme.TextDim, width);
            return;
        }

        UILabelCStr("##exporttitle", m_Project.Title.c_str());
        UINote("##exportroot", HomeRelative(m_Project.Root), theme.TextDim, width);
        UISpacer(6.0f);

        // ---- which preset ----
        const int count = (int)m_Presets.Presets.Size();
        m_Selected      = count > 0 ? Maths::Clamp(m_Selected, 0, count - 1) : 0;

        static const int kMaxShown = 32;
        std::string labels[kMaxShown];
        const char* names[kMaxShown];
        const int shown = Maths::Min(count, kMaxShown);
        for(int i = 0; i < shown; i++)
        {
            const ExportPreset& p = m_Presets.Presets[i];
            labels[i]             = std::string(UIPlatformIcon(p.Target)) + "  " + p.Name + (p.Release ? "" : "  (debug)");
            names[i]              = labels[i].c_str();
        }

        UIBeginPropertyGrid("##exportpresetpick", 0.3f);
        if(shown > 0)
            UIPropertyDropdown("Preset", &m_Selected, names, shown);
        UIBeginPropertyRow("");
        {
            if(UIIconButton(ICON_MDI_PLUS "###exportadd", 24.0f).clicked)
                UIOpenPopup("##exportaddpreset");
            UITooltip("Add a preset");
            if(UIBeginPopup("##exportaddpreset"))
            {
                for(int i = 0; i < (int)Platform::Count; i++)
                {
                    const Platform platform = (Platform)i;
                    if(UIMenuItem((std::string(UIPlatformIcon(platform)) + "  " + PlatformName(platform)).c_str()))
                    {
                        ExportPreset preset = PresetFile::Default(m_Project, platform);
                        preset.Name         = UniqueName(preset.Name);
                        m_Presets.Presets.PushBack(preset);
                        m_Selected = (int)m_Presets.Presets.Size() - 1;
                        Save();
                    }
                }
                UIEndPopup();
            }
            if(count > 0)
            {
                if(UIIconButton(ICON_MDI_CONTENT_COPY "###exportduplicate", 24.0f).clicked)
                {
                    ExportPreset copy = m_Presets.Presets[m_Selected];
                    copy.Name         = UniqueName(copy.Name + " copy");
                    m_Presets.Presets.PushBack(copy);
                    m_Selected = (int)m_Presets.Presets.Size() - 1;
                    Save();
                }
                UITooltip("Duplicate");
                if(UIIconButton(ICON_MDI_DELETE_OUTLINE "###exportdelete", 24.0f).clicked)
                    UIOpenPopup("##exportdeletepreset");
                UITooltip("Delete");
                if(UIBeginPopup("##exportdeletepreset"))
                {
                    if(UIMenuItem(("Delete \"" + m_Presets.Presets[m_Selected].Name + "\"").c_str()))
                    {
                        TDArray<ExportPreset> kept;
                        for(u32 i = 0; i < m_Presets.Presets.Size(); i++)
                            if((int)i != m_Selected)
                                kept.PushBack(m_Presets.Presets[i]);
                        m_Presets.Presets = kept;
                        m_Selected        = m_Selected > 0 ? m_Selected - 1 : 0;
                        Save();
                    }
                    UIMenuItem("Cancel");
                    UIEndPopup();
                }
            }
        }
        UIEndPropertyRow();
        UIEndPropertyGrid();

        if(m_Selected >= (int)m_Presets.Presets.Size())
        {
            UINote("##exportnopreset", "Add a preset to export.", theme.TextDim, width);
            return;
        }

        ExportPreset& preset    = m_Presets.Presets[m_Selected];
        ResolvedExport resolved = Resolve(m_Project, preset);

        EditorUISettings(preset, resolved);
        UISpacer(6.0f);
        EditorUIActions(preset, resolved, width);
        if(m_Job)
        {
            UISpacer(6.0f);
            EditorUILog();
        }
    }

    void ExportPanel::EditorUISettings(ExportPreset& p, const ResolvedExport& r)
    {
        const UIToolTheme& theme = UIGetToolTheme();
        const float dpi          = GetUIState()->DPIScale;
        bool changed             = false;
        const bool mac           = p.Target == Platform::MacOS;
        const bool ios           = p.Target == Platform::IOS;
        const float fraction     = 0.34f;

        if(UISectionHeader("Preset###exportpreset", true))
        {
            UIBeginPropertyGrid("##exportpresetgrid", fraction);
            changed |= UIField("Name", p.Name, "used with --preset=");
            static const char* kPlatforms[] = { "macOS", "iOS", "Windows", "Linux" };
            int platform                    = (int)p.Target;
            if(UIPropertyDropdown("Platform", &platform, kPlatforms, 4))
                p.Target = (Platform)platform, changed = true;
            static const char* kConfigs[] = { "Release", "Debug" };
            int config                    = p.Release ? 0 : 1;
            if(UIPropertyDropdown("Config", &config, kConfigs, 2))
                p.Release = config == 0, changed = true;
            changed |= UIField("Output", p.OutputDir, "Builds/" + SanitiseName(p.Name) + "/");
            UIEndPropertyGrid();
        }

        if(UISectionHeader("App###exportapp", true))
        {
            UIBeginPropertyGrid("##exportappgrid", fraction);
            // empty = the value from Project Settings, shown as the placeholder
            changed |= UIField("App Name", p.AppName, m_Project.Title);
            changed |= UIField("Bundle Id", p.BundleId, m_Project.BundleIdentifier);
            changed |= UIField("Version", p.Version, m_Project.Version);
            changed |= UIField("Build Number", p.BuildNumber, m_Project.BuildNumber);
            if(mac)
                changed |= UIField("Category", p.Category, "public.app-category.games");

            // the icon the preset resolves to; a .png dropped from Assets replaces it
            UpdateIcon(r.IconPng);
            char dropped[256] = { 0 };
            bool got          = false;
            UIBeginPropertyRow("Icon");
            if(m_IconTexture)
            {
                if(UIImageButton("##exporticon", m_IconTexture.get(), Vec2(44.0f * dpi, 44.0f * dpi)).clicked)
                    BrowseIcon();
                got = UIDropTarget(ToolPanels::kAssetDragType, dropped, sizeof(dropped));
                char info[96];
                snprintf(info, sizeof(info), "%d x %d", m_IconW, m_IconH);
                UIPushStyle(StyleVar_TextColor, theme.TextDim);
                UILabelCStr("##exporticoninfo", info);
                UIPopStyle(StyleVar_TextColor);
            }
            else
            {
                if(UIButton("Choose a PNG###exporticonpick").clicked)
                    BrowseIcon();
                got = UIDropTarget(ToolPanels::kAssetDragType, dropped, sizeof(dropped));
            }
            UIEndPropertyRow();
            if(got && IsPng(dropped))
                p.IconPath = ProjectRelative(m_Project, dropped), changed = true;

            changed |= UIField("Icon Path", p.IconPath, r.IconPng.empty() ? std::string("Assets/Textures/icon.png") : ProjectRelative(m_Project, r.IconPng));
            UIEndPropertyGrid();
        }

        if((mac || ios || p.Target == Platform::Windows) && UISectionHeader("Signing###exportsigning", true))
        {
            UIBeginPropertyGrid("##exportsigninggrid", fraction);
            if(mac)
            {
                changed |= UIField("Sign Identity", p.SignIdentity, "- (ad hoc, this Mac only)");
                changed |= UIField("Team", p.Team, "Apple team id, for Mac App Store archives");
                changed |= UIPropertyBool("Notarise", &p.Notarize);
                if(p.Notarize)
                    changed |= UIField("Notary Profile", p.NotaryProfile, "notarytool keychain profile");
            }
            else if(ios)
                changed |= UIField("Team", p.Team, "Apple team id");
            else
                changed |= UIField("Sign Certificate", p.SignIdentity, "none");
            UIEndPropertyGrid();
        }

        if(UISectionHeader("Package###exportpackage", true))
        {
            UIBeginPropertyGrid("##exportpackagegrid", fraction);
            if(ios)
            {
                if(m_DeviceState == DeviceState::None)
                    RefreshDevices();
                if(m_DeviceState == DeviceState::Ready)
                {
                    m_Devices     = m_DevicesPending;
                    m_DeviceState = DeviceState::Shown;
                }

                static const int kMaxDevices = 15;
                const char* devices[kMaxDevices + 1];
                devices[0]        = m_DeviceState == DeviceState::Loading ? "Looking for devices..." : "First connected";
                int deviceCount   = 1;
                int deviceCurrent = 0;
                for(u32 i = 0; i < m_Devices.Size() && deviceCount <= kMaxDevices; i++)
                {
                    if(m_Devices[i].Name == p.Device)
                        deviceCurrent = deviceCount;
                    devices[deviceCount++] = m_Devices[i].Name.c_str();
                }
                if(UIPropertyDropdown("Device", &deviceCurrent, devices, deviceCount))
                {
                    p.Device = deviceCurrent == 0 ? std::string() : std::string(devices[deviceCurrent]);
                    changed  = true;
                }
                UIBeginPropertyRow("");
                if(UIButton("Look for devices###exportdevices").clicked)
                    RefreshDevices();
                UIEndPropertyRow();
            }

            // comma separated here, a list in the file
            std::string joined;
            for(auto& e : p.Exclude)
                joined += (joined.empty() ? "" : ", ") + e;
            if(UIField("Exclude", joined, "paths kept out of the pack: saves/, .cfg"))
            {
                p.Exclude.Clear();
                size_t at = 0;
                while(at <= joined.size())
                {
                    size_t comma     = joined.find(',', at);
                    std::string part = joined.substr(at, comma == std::string::npos ? std::string::npos : comma - at);
                    size_t a = part.find_first_not_of(' '), b = part.find_last_not_of(' ');
                    if(a != std::string::npos)
                        p.Exclude.PushBack(part.substr(a, b - a + 1));
                    if(comma == std::string::npos)
                        break;
                    at = comma + 1;
                }
                changed = true;
            }

            if(mac || ios)
                changed |= UIPropertyBool("Upload Archives", &p.Upload);
            if(!ios)
                changed |= UIPropertyBool(p.Target == Platform::Linux ? "tar.gz" : "Zip", &p.Zip);
            UIEndPropertyGrid();
        }

        if(changed)
            Save();
    }

    void ExportPanel::EditorUIActions(ExportPreset& p, const ResolvedExport& r, float width)
    {
        const UIToolTheme& theme = UIGetToolTheme();
        const bool busy          = JobRunning(m_Job);
        const bool canExport     = CanExportFrom(p.Target);
        const bool installed     = TemplateInstalled(p.Target, p.Release);
        const bool mac           = p.Target == Platform::MacOS;
        const bool ios           = p.Target == Platform::IOS;

        // things an export would trip over, before running one
        {
            int issue    = 0;
            auto note    = [&](bool bad, const std::string& text)
            {
                char id[32];
                snprintf(id, sizeof(id), "##exportissue%d", issue++);
                UINote(id, std::string(bad ? ICON_MDI_ALERT_CIRCLE "  " : ICON_MDI_ALERT "  ") + text, bad ? theme.Error : theme.Warning, width);
            };
            if(p.Name.empty())
                note(true, "The preset needs a name");
            else if(UniqueName(p.Name, m_Selected) != p.Name)
                note(true, "Another preset is also called \"" + p.Name + "\": they share an output folder");
            if((mac || ios) && r.BundleId.empty())
                note(true, "No bundle id: set one here or in Project Settings");
            if(r.IconPng.empty())
                note(ios, ios ? "iOS apps need an icon: add Assets/Textures/icon.png or set one" : "No icon: the app gets the default one");
            else if(r.IconPng == m_IconTexturePath && m_IconW > 0)
            {
                if(m_IconW != m_IconH)
                    note(false, "The icon isn't square: it'll be stretched");
                else if(m_IconW < 1024)
                    note(false, "The icon is under 1024px: blurry on iOS and at large Mac sizes");
                if(ios && m_IconTransparent)
                    note(false, "The icon has see-through parts: they turn black on iOS");
            }
            if(ios && p.Team.empty())
                note(false, "No Team: signing for a device will fail");
            if(mac && p.Notarize)
            {
                if(p.SignIdentity.empty() || p.SignIdentity == "-")
                    note(true, "Notarising needs a Developer ID sign identity, not ad hoc");
                if(p.NotaryProfile.empty())
                    note(true, "Notarising needs a notary profile");
            }
        }

        // template
        const char* config = p.Release ? "release" : "debug";
        if(!canExport)
            UINote("##exporttemplate", std::string(ICON_MDI_ALERT "  ") + PlatformName(p.Target) + " apps are exported on " + (ios ? "a Mac" : PlatformName(p.Target)), theme.Warning, width);
        else if(installed)
            UINote("##exporttemplate", std::string(ICON_MDI_CHECK_CIRCLE "  ") + PlatformName(p.Target) + " " + config + " template " + EngineVersion(), theme.TextDim, width);
        else
            UINote("##exporttemplate", std::string(ICON_MDI_ALERT "  No ") + PlatformName(p.Target) + " " + config + " template for " + EngineVersion(), theme.Warning, width);

        // what comes out
        UINote("##exportsummary", r.AppName + " " + r.Version + " (" + r.BuildNumber + ")   " + r.BundleId + "   >   " + ProjectRelative(m_Project, r.OutputDir), theme.TextDim, width);
        UISpacer(4.0f);

        const bool ready = !busy && canExport && installed;
        UIBeginRowFullWidth();
        if(UIAction(ICON_MDI_EXPORT "  Export###exportgo", ready))
            Start(Job::Kind::Export);
        if(ios)
        {
            if(UIAction(ICON_MDI_CELLPHONE "  Deploy###exportdeploy", ready))
                Start(Job::Kind::Deploy);
            if(UIAction(p.Upload ? ICON_MDI_UPLOAD "  Upload to App Store###exportarchive" : ICON_MDI_EXPORT "  App Store Archive###exportarchive", ready))
                Start(Job::Kind::Archive);
            // the editor itself on the device, opening this project: built from engine source, no template
            if(!EngineSourceRoot().empty() && UIAction(ICON_MDI_PENCIL "  Editor on Device###exporteditor", !busy && canExport))
                Start(Job::Kind::DeployEditor);
        }
        else if(UIAction(ICON_MDI_PLAY "  Export & Run###exportrun", ready))
            Start(Job::Kind::ExportAndRun);
        UIEndRow();

        UISpacer(4.0f);
        UIBeginRowFullWidth();
        if(mac && UIAction(p.Upload ? ICON_MDI_UPLOAD "  Upload to Mac App Store###exportarchive" : ICON_MDI_EXPORT "  Mac App Store Archive###exportarchive", ready && MacStoreTemplateInstalled()))
            Start(Job::Kind::Archive);
        UIEndRow();

        UISpacer(4.0f);
        UIBeginRowFullWidth();
        if(canExport && !EngineSourceRoot().empty() && UIAction(installed ? ICON_MDI_HAMMER "  Rebuild Template###exporttemplatebuild" : ICON_MDI_HAMMER "  Build Template###exporttemplatebuild", !busy))
            StartTemplateBuild(p.Target, p.Release);
        if(UIAction(ICON_MDI_FOLDER_OPEN "  Open Folder###exportfolder"))
        {
            std::error_code ec;
            std::filesystem::create_directories(r.OutputDir, ec);
            OS::Get().OpenFileLocation(r.OutputDir);
        }
        if(busy && UIAction(ICON_MDI_STOP "  Cancel###exportcancel"))
            m_Job->Cancel();
        UIEndRow();

        // the same thing from a terminal
        UISpacer(4.0f);
        const std::string command = CommandFor(p, ios ? Job::Kind::Deploy : Job::Kind::Export);
        UIBeginRowFullWidth();
        if(UIIconButton(ICON_MDI_CONTENT_COPY "###exportcopycommand", 22.0f).clicked)
            Input::Get().SetClipboard(command.c_str());
        UITooltip("Copy the command");
        UIPushStyle(StyleVar_TextColor, theme.TextDim);
        UILabelEllipsis("##exportcommand", Str8C((char*)command.c_str()), Maths::Max(60.0f, width - 30.0f));
        UIPopStyle(StyleVar_TextColor);
        UIEndRow();
    }

    void ExportPanel::EditorUILog()
    {
        const UIToolTheme& theme = UIGetToolTheme();
        const float dpi          = GetUIState()->DPIScale;
        const bool finished      = m_Job->Finished();
        const std::string summary = m_Job->Summary();
        const u32 count          = m_Job->LineCount();
        bool close               = false;

        if(UISectionHeader("Log###exportlog", true))
        {
            UIProgressBar("##exportprogress", m_Job->Progress(), 240.0f, 4.0f);

            UIBeginRowFullWidth();
            UIPushStyle(StyleVar_TextColor, !finished ? theme.Text : (m_Job->Succeeded() ? theme.Success : theme.Error));
            UILabelCStr("##exportstate", summary.c_str());
            UIPopStyle(StyleVar_TextColor);
            UIFlexSpacer();
            if(finished && m_Job->Succeeded() && !m_Job->ResultPath().empty())
            {
                if(UIIconButton(ICON_MDI_FOLDER_OPEN "###exportreveal", 22.0f).clicked)
                    OS::Get().OpenFileLocation(m_Job->ResultPath());
                UITooltip("Show the result");
            }
            if(UIIconButton(ICON_MDI_CONTENT_COPY "###exportcopylog", 22.0f).clicked)
            {
                std::string all;
                for(u32 i = 0; i < count; i++)
                    all += m_Job->Line(i).Text + "\n";
                Input::Get().SetClipboard(all.c_str());
            }
            UITooltip("Copy the whole log");
            if(finished)
            {
                close = UIIconButton(ICON_MDI_CLOSE "###exportcloselog", 22.0f).clicked;
                UITooltip("Close the log");
            }
            UIEndRow();

            // the tail: the panel itself scrolls, and copy gives everything
            const u32 kTail = 60;
            UIPushStyle(StyleVar_FontSize, 12.0f * dpi);
            for(u32 i = count > kTail ? count - kTail : 0; i < count; i++)
            {
                const LogLine line = m_Job->Line(i);
                Vec4 colour        = theme.Text;
                std::string text   = line.Text;
                switch(line.Level)
                {
                case Export::LogLevel::Step:
                    colour = theme.Accent;
                    text   = std::string(ICON_MDI_PLAY " ") + text;
                    break;
                case Export::LogLevel::Warn: colour = theme.Warning; break;
                case Export::LogLevel::Error: colour = theme.Error; break;
                case Export::LogLevel::Tool:
                    colour = theme.TextDim;
                    text   = "    " + text;
                    break;
                default: break;
                }
                // compiler command lines run to tens of thousands of characters; the
                // copy button still gives the whole thing
                const size_t kMaxShown = 300;
                if(text.size() > kMaxShown)
                    text = text.substr(0, kMaxShown) + " ...";
                char id[40];
                snprintf(id, sizeof(id), "###exportline%u", i);
                UIPushStyle(StyleVar_TextColor, colour);
                UILabel(id, Str8C((char*)text.c_str()));
                UIPopStyle(StyleVar_TextColor);
            }
            UIPopStyle(StyleVar_FontSize);
        }

        if(close)
        {
            if(m_Thread.joinable())
                m_Thread.join();
            m_Job.reset();
        }
    }
}
