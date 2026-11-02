#pragma once
#include "ExportProject.h"
#include "Core/OS/Process.h"
#include <atomic>
#include <functional>
#include <mutex>

namespace Lumos::Export
{
    enum class LogLevel
    {
        Info,
        Step,
        Warn,
        Error,
        Tool // raw output of a child process
    };

    struct LogLine
    {
        std::string Text;
        LogLevel Level;
    };

    // One export (or template build) run start to finish on the calling thread; poll the getters
    // from another (the editor runs it on the job system). Cancel kills any child process.
    class Job
    {
    public:
        enum class Kind
        {
            Export,        // pack + assemble the app
            ExportAndRun,  // ... then launch it here
            Deploy,        // iOS: ... then install + launch on Options.Device
            Archive,       // iOS: App Store archive + .ipa (or upload when the preset says so)
            BuildTemplate, // compile the Runtime for a platform from engine source
            DeployEditor,  // iOS: build the editor with this project inside it, install + launch (needs engine source)
        };

        struct Options
        {
            Kind JobKind = Kind::Export;
            std::string Device; // iOS device name or id, "" = the first connected one
        };

        Job(const ProjectInfo& project, const ExportPreset& preset, const Options& options);
        // template build only needs the platform + config
        Job(Platform platform, bool release);

        bool Execute();
        void Cancel();

        // called for every line as it's logged (CLI prints these), from the executing thread
        std::function<void(const LogLine&)> OnLog;

        // thread-safe state for a UI
        u32 LineCount() const;
        LogLine Line(u32 i) const;
        float Progress() const { return m_Progress.load(); }
        std::string CurrentStep() const;
        bool Finished() const { return m_Finished.load(); }
        bool Succeeded() const { return m_Succeeded.load(); }
        bool Cancelled() const { return m_Cancel.load(); }
        std::string ResultPath() const;
        double Seconds() const;
        std::string Summary() const; // "Exported LumosTunes.app in 12.3s"

    private:
        void Log(LogLevel level, const std::string& text);
        void Step(const std::string& name, float progress);
        bool Fail(const std::string& text);
        // runs a tool, streaming its output into the log. Returns the exit code (-1 = couldn't start / cancelled)
        int Tool(const TDArray<std::string>& args, const std::string& cwd = "", std::string* capture = nullptr);
        bool ToolOk(const TDArray<std::string>& args, const std::string& what, const std::string& cwd = "");

        bool Pack();
        bool ExportMacOS();
        bool ArchiveMacStore();
        bool CollectMacStoreTemplate(const std::string& src, const std::string& dest);
        bool ExportIOS();
        bool ExportWindows();
        bool ExportLinux();
        bool BuildTemplate();
        bool DeployEditor();
        bool BuildTemplateMacOS(const std::string& src, const std::string& dest);
        bool BuildTemplateIOS(const std::string& src, const std::string& dest);
        bool BuildTemplateDesktop(const std::string& src, const std::string& dest);
        bool CopyProjectFiles(const std::string& destDir);
        bool WritePrivacyManifest(const std::string& workDir);

        ProjectInfo m_Project;
        ExportPreset m_Preset;
        Options m_Options;
        ResolvedExport m_Resolved;
        Platform m_TemplatePlatform = Platform::MacOS;
        bool m_TemplateRelease      = true;

        mutable std::mutex m_Lock;
        TDArray<LogLine> m_Lines;
        std::string m_Step;
        std::string m_Result;
        std::atomic<float> m_Progress { 0.0f };
        std::atomic<bool> m_Cancel { false };
        std::atomic<bool> m_Finished { false };
        std::atomic<bool> m_Succeeded { false };
        Process* m_Running = nullptr; // guarded by m_Lock
        double m_Start = 0.0, m_End = 0.0;
    };

    // Connected iOS devices via devicectl: name, CoreDevice id (devicectl) + hardware UDID (xcodebuild)
    struct Device
    {
        std::string Name;
        std::string Identifier;
        std::string UDID;
        std::string State;
    };
    TDArray<Device> ListIOSDevices(std::string* error = nullptr);
}
