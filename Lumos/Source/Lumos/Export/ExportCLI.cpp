#include "Precompiled.h"
#include "ExportCLI.h"
#include "Exporter.h"
#include "ExportIcons.h"
#include "Core/Asset/AssetPacker.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;

namespace Lumos::Export
{
    namespace
    {
        struct Args
        {
            std::string Command;
            TDArray<std::string> Positional;
            TDArray<std::pair<std::string, std::string>> Options;

            std::string Get(const char* key, const std::string& def = "") const
            {
                for(auto& o : Options)
                    if(o.first == key)
                        return o.second;
                return def;
            }
            bool Has(const char* key) const
            {
                for(auto& o : Options)
                    if(o.first == key)
                        return true;
                return false;
            }
        };

        const char* s_Commands[] = { "export", "deploy", "archive", "run", "pack", "icons", "devices", "presets", "templates", "help" };

        void PrintLine(const LogLine& l)
        {
            switch(l.Level)
            {
            case LogLevel::Step: printf("==> %s\n", l.Text.c_str()); break;
            case LogLevel::Warn: printf("warning: %s\n", l.Text.c_str()); break;
            case LogLevel::Error: fprintf(stderr, "error: %s\n", l.Text.c_str()); break;
            case LogLevel::Tool: printf("    %s\n", l.Text.c_str()); break;
            default: printf("%s\n", l.Text.c_str()); break;
            }
            fflush(stdout);
        }

        void Usage()
        {
            printf(R"(Lumos %s - export / package projects

Usage: lumos <command> [options]

  export    --project=<dir> --preset=<name>        Export using a preset from ExportPresets.json
            --project=<dir> --platform=<p>          ... or the platform's default (macos ios windows linux)
            [--config=debug] [--out=<dir>] [--run]
  deploy    --project=<dir> [--preset=iOS] [--device="Joe's iPhone"]
                                                    iOS: export, install and launch on a device
  archive   --project=<dir> [--preset=iOS|macOS] [--upload]
                                                    App Store archive: .ipa (iOS) / .pkg (Mac App Store); --upload sends it to App Store Connect
  run       --project=<dir>                         Run the project from its loose files with the template Runtime
  pack      --project=<dir> [--out=<file>]          Just build Assets.lpak
  icons     --project=<dir> [--out=<dir>]           Write .icns / .ico / iOS + Linux icon sets
  devices                                           List connected iOS devices
  presets   --project=<dir> [--init]                List presets (--init writes defaults for this machine)
  templates list | path | build --platform=<p> [--config=debug]
                                                    Export templates (prebuilt Runtimes). build needs the engine source

Templates: %s
Docs: Help > Packaging Guide in the editor, or Docs/Packaging.md
)",
                   EngineVersion().c_str(), TemplatesRoot().c_str());
        }

        bool LoadProject(const Args& a, ProjectInfo& project)
        {
            std::string dir = a.Get("project", a.Positional.Empty() ? "" : a.Positional[0]);
            if(dir.empty())
                dir = fs::current_path().string();
            std::string err;
            if(!project.Load(dir, &err))
            {
                fprintf(stderr, "error: %s\n", err.c_str());
                return false;
            }
            return true;
        }

        bool PickPreset(const Args& a, const ProjectInfo& project, ExportPreset& preset)
        {
            PresetFile file;
            file.Load(project);
            std::string name = a.Get("preset");
            std::string plat = a.Get("platform");
            if(!name.empty())
            {
                if(ExportPreset* p = file.Find(name))
                    preset = *p;
                else if(Platform pl; ParsePlatform(name, pl))
                    preset = PresetFile::Default(project, pl);
                else
                {
                    fprintf(stderr, "error: no preset \"%s\" in %s\n", name.c_str(), PresetFile::PathFor(project).c_str());
                    for(auto& p : file.Presets)
                        fprintf(stderr, "  %s (%s)\n", p.Name.c_str(), PlatformId(p.Target));
                    return false;
                }
            }
            else
            {
                Platform pl = HostPlatform();
                if(!plat.empty() && !ParsePlatform(plat, pl))
                {
                    fprintf(stderr, "error: unknown platform \"%s\" (macos ios windows linux)\n", plat.c_str());
                    return false;
                }
                ExportPreset* p = file.Find(PlatformId(pl));
                preset          = p ? *p : PresetFile::Default(project, pl);
            }
            if(a.Has("config"))
                preset.Release = a.Get("config") != "debug";
            if(a.Has("out"))
                preset.OutputDir = fs::absolute(a.Get("out")).string();
            if(a.Has("device"))
                preset.Device = a.Get("device");
            if(a.Has("team"))
                preset.Team = a.Get("team");
            return true;
        }

        int RunJob(Job& job)
        {
            job.OnLog = PrintLine;
            bool ok   = job.Execute();
            if(ok && !job.ResultPath().empty())
                printf("\n%s\n", job.ResultPath().c_str());
            return ok ? 0 : 1;
        }
    }

    bool IsCommand(int argc, char** argv)
    {
        if(argc < 2)
            return false;
        for(const char* c : s_Commands)
            if(strcmp(argv[1], c) == 0)
                return true;
        return strcmp(argv[1], "--help-export") == 0;
    }

    bool RunCommandLine(int argc, char** argv, int& exitCode)
    {
        if(argc < 2)
            return false;
        Args a;
        a.Command  = argv[1];
        bool known = false;
        for(const char* c : s_Commands)
            if(a.Command == c)
                known = true;
        if(a.Command == "--help-export")
            known = true, a.Command = "help";
        if(!known)
            return false;

        for(int i = 2; i < argc; i++)
        {
            std::string s = argv[i];
            if(s.rfind("--", 0) == 0)
            {
                size_t eq = s.find('=');
                a.Options.PushBack({ s.substr(2, eq == std::string::npos ? std::string::npos : eq - 2), eq == std::string::npos ? "" : s.substr(eq + 1) });
            }
            else
                a.Positional.PushBack(s);
        }

        exitCode = 1;
        if(getenv("APP_SANDBOX_CONTAINER_ID"))
        {
            fprintf(stderr, "error: this editor build is sandboxed (App Store) and can't run the export tools - use a Release / Debug editor build\n");
            return true;
        }
        if(a.Command == "help")
        {
            Usage();
            exitCode = 0;
            return true;
        }

        if(a.Command == "devices")
        {
            std::string err;
            auto devices = ListIOSDevices(&err);
            if(!err.empty())
                fprintf(stderr, "error: %s\n", err.c_str());
            for(auto& d : devices)
                printf("%-28s %-10s %s\n", d.Name.c_str(), d.State.c_str(), d.Identifier.c_str());
            if(devices.Empty() && err.empty())
                printf("No iOS devices connected\n");
            exitCode = err.empty() ? 0 : 1;
            return true;
        }

        if(a.Command == "templates")
        {
            std::string sub = a.Positional.Empty() ? "list" : a.Positional[0];
            if(sub == "path")
            {
                printf("%s\n", TemplatesRoot().c_str());
                exitCode = 0;
            }
            else if(sub == "list")
            {
                printf("Templates for %s in %s\n", EngineVersion().c_str(), TemplatesRoot().c_str());
                for(int i = 0; i < (int)Platform::Count; i++)
                    for(bool release : { true, false })
                    {
                        Platform p = (Platform)i;
                        printf("  %-8s %-8s %s%s\n", PlatformId(p), release ? "release" : "debug", TemplateInstalled(p, release) ? "installed" : "-",
                               CanExportFrom(p) ? "" : "  (not on this OS)");
                    }
                printf("Engine source: %s\n", EngineSourceRoot().empty() ? "not found" : EngineSourceRoot().c_str());
                exitCode = 0;
            }
            else if(sub == "build")
            {
                Platform p = HostPlatform();
                if(a.Has("platform") && !ParsePlatform(a.Get("platform"), p))
                {
                    fprintf(stderr, "error: unknown platform \"%s\"\n", a.Get("platform").c_str());
                    return true;
                }
                std::string config = a.Get("config", "release");
                if(config == "all")
                {
                    Job release(p, true), debug(p, false);
                    exitCode = RunJob(release) == 0 && RunJob(debug) == 0 ? 0 : 1;
                }
                else
                {
                    Job job(p, config != "debug");
                    exitCode = RunJob(job);
                }
            }
            else
                fprintf(stderr, "error: templates list | path | build\n");
            return true;
        }

        ProjectInfo project;
        if(!LoadProject(a, project))
            return true;

        if(a.Command == "presets")
        {
            PresetFile file;
            bool had = file.Load(project);
            if(a.Has("init"))
            {
                file.AddDefaults(project);
                file.Save(project);
                printf("Wrote %s\n", PresetFile::PathFor(project).c_str());
            }
            else if(!had)
                printf("No %s yet (lumos presets --init writes one); defaults would be:\n", PresetFile::PathFor(project).c_str()), file.AddDefaults(project);
            for(auto& p : file.Presets)
            {
                ResolvedExport r = Resolve(project, p);
                printf("  %-12s %-8s %-8s -> %s%s\n", p.Name.c_str(), PlatformId(p.Target), p.Release ? "release" : "debug", r.OutputDir.c_str(),
                       CanExportFrom(p.Target) ? "" : "  (not on this OS)");
            }
            exitCode = 0;
            return true;
        }

        if(a.Command == "pack")
        {
            std::string out = a.Get("out", project.Root + ".lumos/Assets.lpak");
            std::error_code ec;
            fs::create_directories(fs::path(out).parent_path(), ec);
            PackSettings settings;
            settings.ExcludePaths.PushBack(Str8Lit("Cache/"));
            std::string assets = project.Root + "Assets/";
            bool ok            = AssetPacker::Pack(Str8StdS(out), Str8StdS(assets), settings);
            printf("%s %s\n", ok ? "Packed" : "Packing failed:", out.c_str());
            exitCode = ok ? 0 : 1;
            return true;
        }

        if(a.Command == "icons")
        {
            ExportPreset preset = PresetFile::Default(project, HostPlatform());
            ResolvedExport r    = Resolve(project, preset);
            if(r.IconPng.empty())
            {
                fprintf(stderr, "error: no icon - set IconPath in the .lmproj or add Assets/Textures/icon.png\n");
                return true;
            }
            std::string out = a.Get("out", project.Root + ".lumos/icons/");
            std::string err;
            bool ok = Icons::WriteIcns(r.IconPng, out + "/AppIcon.icns", &err) && Icons::WriteIco(r.IconPng, out + "/AppIcon.ico", &err) &&
                      Icons::WriteIOSAssetCatalog(r.IconPng, out + "/ios/Assets.xcassets", &err) && Icons::WritePngSet(r.IconPng, out + "/linux", &err);
            printf("%s %s\n", ok ? "Icons in" : ("Failed: " + err).c_str(), out.c_str());
            exitCode = ok ? 0 : 1;
            return true;
        }

        if(a.Command == "run")
        {
            Platform host = HostPlatform();
            if(!TemplateInstalled(host, false) && !TemplateInstalled(host, true))
            {
                fprintf(stderr, "error: no %s template - lumos templates build\n", PlatformId(host));
                return true;
            }
            bool release    = !TemplateInstalled(host, false);
            std::string dir = TemplateDir(host, release);
            std::string exe = host == Platform::MacOS ? dir + "Runtime.app/Contents/MacOS/Runtime" : (host == Platform::Windows ? dir + "Runtime.exe" : dir + "Runtime");
            TDArray<std::string> cmd = { exe, "--project=" + project.Root };
            for(auto& o : a.Options)
                if(o.first != "project")
                    cmd.PushBack("--" + o.first + (o.second.empty() ? "" : "=" + o.second));
            Process p;
            if(!p.Start(cmd))
            {
                fprintf(stderr, "error: couldn't start %s\n", exe.c_str());
                return true;
            }
            std::string out;
            while(p.IsRunning() || p.ReadOutput(out))
            {
                p.ReadOutput(out);
                if(!out.empty())
                {
                    fputs(out.c_str(), stdout);
                    fflush(stdout);
                    out.clear();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            exitCode = p.ExitCode();
            return true;
        }

        // export / deploy
        ExportPreset preset;
        if((a.Command == "deploy" || a.Command == "archive") && !a.Has("preset") && !a.Has("platform"))
            a.Options.PushBack({ "platform", "ios" });
        if(!PickPreset(a, project, preset))
            return true;
        if(a.Has("upload"))
            preset.Upload = true;
        Job::Options options;
        options.Device  = a.Get("device");
        options.JobKind = a.Command == "deploy"    ? Job::Kind::Deploy
                          : a.Command == "archive" ? Job::Kind::Archive
                          : (a.Has("run") ? Job::Kind::ExportAndRun : Job::Kind::Export);
        Job job(project, preset, options);
        exitCode = RunJob(job);
        return true;
    }
}
