#pragma once
#include "EditorPanel.h"
#include <Lumos/Export/Exporter.h>
#include <Lumos/Core/DataStructures/TDArray.h>
#include <Lumos/Core/Reference.h>
#include <Lumos/Graphics/RHI/Texture.h>
#include <atomic>
#include <memory>
#include <thread>

namespace Lumos
{
    struct UIDockPanelContext;
    // Project > Export...: presets (ExportPresets.json), template status, export / run / deploy with a
    // live log. Runs the same Export::Job as `lumos export`.
    class ExportPanel : public EditorPanel
    {
    public:
        ExportPanel();
        ~ExportPanel();

        // The same panel on the engine UI (one scrolling column).
        void OnEditorUI(const UIDockPanelContext& context);
        void OnNewProject() override;

    private:
        bool CheckProject();
        void EditorUISettings(Export::ExportPreset& preset, const Export::ResolvedExport& resolved);
        void EditorUIActions(Export::ExportPreset& preset, const Export::ResolvedExport& resolved, float width);
        void EditorUILog();
        void Reload();
        void Save();
        void Start(Export::Job::Kind kind);
        void StartTemplateBuild(Export::Platform platform, bool release);
        void Launch(std::unique_ptr<Export::Job> job);
        void DrawPresetList(float width);
        void DrawPresetSettings(Export::ExportPreset& preset);
        bool DrawIcon(Export::ExportPreset& preset, const Export::ResolvedExport& resolved, float size);
        void DrawChecks(const Export::ExportPreset& preset, const Export::ResolvedExport& resolved);
        void DrawActions(Export::ExportPreset& preset);
        void DrawLog(float height);
        void UpdateIcon(const std::string& png);
        void BrowseIcon();
        void RefreshDevices();
        std::string UniqueName(const std::string& base, int ignore = -1) const;
        std::string CommandFor(const Export::ExportPreset& preset, Export::Job::Kind kind) const;

        Export::ProjectInfo m_Project;
        Export::PresetFile m_Presets;
        bool m_HaveProject = false;
        int m_Selected     = 0;

        std::unique_ptr<Export::Job> m_Job;
        std::thread m_Thread;
        u32 m_LogShown   = 0;
        bool m_FollowLog = true;
        float m_LogSplit = 0.38f; // log's share of the panel height

        // the icon the selected preset resolves to
        SharedPtr<Graphics::Texture2D> m_IconTexture;
        std::string m_IconTexturePath;
        long long m_IconStamp  = 0;
        double m_IconChecked   = 0.0;
        int m_IconW            = 0;
        int m_IconH            = 0;
        bool m_IconTransparent = false;
        // set by the file dialog, applied on the next frame
        std::string m_PickedIcon;
        int m_PickedIconPreset = -1;
        int m_BrowsePreset     = -1; // preset the open picker is for

        // devicectl takes a second or two: off the UI thread
        enum class DeviceState
        {
            None,
            Loading,
            Ready,
            Shown
        };
        TDArray<Export::Device> m_Devices, m_DevicesPending;
        std::thread m_DeviceThread;
        std::atomic<DeviceState> m_DeviceState { DeviceState::None };
    };
}
