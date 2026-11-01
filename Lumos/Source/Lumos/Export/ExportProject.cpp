#include "Precompiled.h"
#include "ExportProject.h"
#include "Core/Version.h"

#include <cereal/external/rapidjson/document.h>
#include <cereal/external/rapidjson/prettywriter.h>
#include <cereal/external/rapidjson/stringbuffer.h>
#include <filesystem>
#include <fstream>
#include <sstream>

#if defined(LUMOS_PLATFORM_MACOS)
#include <mach-o/dyld.h>
#elif defined(LUMOS_PLATFORM_WINDOWS)
#include <windows.h>
#elif defined(LUMOS_PLATFORM_LINUX)
#include <unistd.h>
#endif

namespace fs   = std::filesystem;
namespace json = CEREAL_RAPIDJSON_NAMESPACE;

namespace Lumos::Export
{
    // ---------------------------------------------------------------- platforms

    static const char* s_Ids[]   = { "macos", "ios", "windows", "linux" };
    static const char* s_Names[] = { "macOS", "iOS", "Windows", "Linux" };

    const char* PlatformId(Platform p) { return p < Platform::Count ? s_Ids[(int)p] : "?"; }
    const char* PlatformName(Platform p) { return p < Platform::Count ? s_Names[(int)p] : "?"; }

    bool ParsePlatform(const std::string& s, Platform& out)
    {
        std::string l;
        for(char c : s)
            l += (char)tolower((unsigned char)c);
        if(l == "mac" || l == "osx")
            l = "macos";
        if(l == "win")
            l = "windows";
        for(int i = 0; i < (int)Platform::Count; i++)
            if(l == s_Ids[i])
            {
                out = (Platform)i;
                return true;
            }
        return false;
    }

    Platform HostPlatform()
    {
#if defined(LUMOS_PLATFORM_WINDOWS)
        return Platform::Windows;
#elif defined(LUMOS_PLATFORM_LINUX)
        return Platform::Linux;
#else
        return Platform::MacOS;
#endif
    }

    bool CanExportFrom(Platform target, Platform host)
    {
        if(target == Platform::IOS)
            return host == Platform::MacOS;
        return target == host;
    }

    std::string PathJoin(const std::string& a, const std::string& b)
    {
        if(a.empty())
            return b;
        if(a.back() == '/' || a.back() == '\\')
            return a + b;
        return a + "/" + b;
    }

    std::string SanitiseName(const std::string& s)
    {
        std::string out;
        for(char c : s)
            if(isalnum((unsigned char)c) || c == '_' || c == '-')
                out += c;
        return out.empty() ? std::string("App") : out;
    }

    // ---------------------------------------------------------------- json helpers

    static bool ReadJson(const std::string& path, json::Document& doc)
    {
        std::ifstream f(path, std::ios::binary);
        if(!f)
            return false;
        std::stringstream ss;
        ss << f.rdbuf();
        std::string text = ss.str();
        doc.Parse(text.c_str());
        return !doc.HasParseError() && doc.IsObject();
    }

    static void Get(const json::Value& v, const char* key, std::string& out)
    {
        auto it = v.FindMember(key);
        if(it != v.MemberEnd() && it->value.IsString())
            out = it->value.GetString();
    }
    static void Get(const json::Value& v, const char* key, int& out)
    {
        auto it = v.FindMember(key);
        if(it != v.MemberEnd() && it->value.IsInt())
            out = it->value.GetInt();
    }
    static void Get(const json::Value& v, const char* key, bool& out)
    {
        auto it = v.FindMember(key);
        if(it != v.MemberEnd() && it->value.IsBool())
            out = it->value.GetBool();
    }
    static void Get(const json::Value& v, const char* key, float& out)
    {
        auto it = v.FindMember(key);
        if(it != v.MemberEnd() && it->value.IsNumber())
            out = (float)it->value.GetDouble();
    }

    // ---------------------------------------------------------------- project

    bool ProjectInfo::Load(const std::string& path, std::string* error)
    {
        std::error_code ec;
        fs::path p = fs::absolute(fs::path(path), ec);
        if(fs::is_directory(p, ec))
        {
            // prefer <folder>.lmproj, else the first one
            fs::path preferred = p / (p.filename().string() + ".lmproj");
            if(p.filename().empty())
                preferred = p.parent_path() / (p.parent_path().filename().string() + ".lmproj");
            if(fs::exists(preferred, ec))
                p = preferred;
            else
            {
                fs::path found;
                for(auto& e : fs::directory_iterator(p, ec))
                    if(e.path().extension() == ".lmproj")
                    {
                        found = e.path();
                        break;
                    }
                if(found.empty())
                {
                    if(error)
                        *error = "No .lmproj in " + p.string();
                    return false;
                }
                p = found;
            }
        }
        json::Document doc;
        if(!ReadJson(p.string(), doc))
        {
            if(error)
                *error = "Couldn't read " + p.string();
            return false;
        }
        const json::Value* v = &doc;
        auto inner           = doc.FindMember("value0");
        if(inner != doc.MemberEnd() && inner->value.IsObject())
            v = &inner->value;

        Lmproj = p.lexically_normal().string();
        Root   = p.parent_path().lexically_normal().string();
        if(Root.empty() || (Root.back() != '/' && Root.back() != '\\'))
            Root += "/";
        Name = p.stem().string();

        Get(*v, "Title", Title);
        if(Title.empty())
            Title = Name;
        Get(*v, "BundleIdentifier", BundleIdentifier);
        Get(*v, "Version", Version);
        Get(*v, "BuildNumber", BuildNumber);
        Get(*v, "IconPath", IconPath);
        Get(*v, "SplashPath", SplashPath);
        Get(*v, "SplashBGR", SplashColour[0]);
        Get(*v, "SplashBGG", SplashColour[1]);
        Get(*v, "SplashBGB", SplashColour[2]);
        Get(*v, "SplashBGA", SplashColour[3]);
        Get(*v, "Orientation", Orientation);
        Get(*v, "DeviceFamily", DeviceFamily);
        Get(*v, "MinIOSVersion", MinIOSVersion);
        Get(*v, "StatusBarHidden", StatusBarHidden);
        Get(*v, "UsesNonExemptEncryption", UsesNonExemptEncryption);
        Get(*v, "CameraUsage", CameraUsage);
        Get(*v, "MicrophoneUsage", MicrophoneUsage);
        Get(*v, "PhotoLibraryUsage", PhotoLibraryUsage);
        Get(*v, "LocationUsage", LocationUsage);
        if(BundleIdentifier.empty())
        {
            std::string safe;
            for(char c : Title)
                if(isalnum((unsigned char)c))
                    safe += (char)tolower((unsigned char)c);
            BundleIdentifier = "com.lumos." + (safe.empty() ? std::string("app") : safe);
        }
        return true;
    }

    std::string ProjectInfo::Resolve(const std::string& path) const
    {
        if(path.empty())
            return path;
        if(path.rfind("//", 0) == 0)
            return Root + path.substr(2);
        if(fs::path(path).is_absolute())
            return path;
        return Root + path;
    }

    // ---------------------------------------------------------------- presets

    std::string PresetFile::PathFor(const ProjectInfo& project) { return project.Root + "ExportPresets.json"; }

    bool PresetFile::Load(const ProjectInfo& project)
    {
        Presets.Clear();
        json::Document doc;
        if(!ReadJson(PathFor(project), doc))
            return false;
        auto list = doc.FindMember("Presets");
        if(list == doc.MemberEnd() || !list->value.IsArray())
            return false;
        for(auto& v : list->value.GetArray())
        {
            if(!v.IsObject())
                continue;
            ExportPreset p;
            std::string platform;
            Get(v, "Name", p.Name);
            Get(v, "Platform", platform);
            if(!ParsePlatform(platform, p.Target))
                continue;
            std::string config = "release";
            Get(v, "Config", config);
            p.Release = config != "debug";
            Get(v, "OutputDir", p.OutputDir);
            Get(v, "AppName", p.AppName);
            Get(v, "BundleId", p.BundleId);
            Get(v, "IconPath", p.IconPath);
            Get(v, "Version", p.Version);
            Get(v, "BuildNumber", p.BuildNumber);
            Get(v, "SignIdentity", p.SignIdentity);
            Get(v, "Team", p.Team);
            Get(v, "Notarize", p.Notarize);
            Get(v, "NotaryProfile", p.NotaryProfile);
            Get(v, "Zip", p.Zip);
            Get(v, "Device", p.Device);
            Get(v, "Upload", p.Upload);
            Get(v, "Category", p.Category);
            auto ex = v.FindMember("Exclude");
            if(ex != v.MemberEnd() && ex->value.IsArray())
                for(auto& e : ex->value.GetArray())
                    if(e.IsString() && *e.GetString())
                        p.Exclude.PushBack(e.GetString());
            if(p.Name.empty())
                p.Name = PlatformName(p.Target);
            Presets.PushBack(p);
        }
        return true;
    }

    bool PresetFile::Save(const ProjectInfo& project) const
    {
        json::StringBuffer sb;
        json::PrettyWriter<json::StringBuffer> w(sb);
        w.SetIndent(' ', 4);
        auto str = [&](const char* k, const std::string& v, bool always = false)
        {
            if(v.empty() && !always)
                return;
            w.Key(k);
            w.String(v.c_str());
        };
        w.StartObject();
        w.Key("Presets");
        w.StartArray();
        for(auto& p : Presets)
        {
            w.StartObject();
            str("Name", p.Name, true);
            str("Platform", PlatformId(p.Target), true);
            str("Config", p.Release ? "release" : "debug", true);
            str("OutputDir", p.OutputDir);
            str("AppName", p.AppName);
            str("BundleId", p.BundleId);
            str("IconPath", p.IconPath);
            str("Version", p.Version);
            str("BuildNumber", p.BuildNumber);
            if(p.Target == Platform::MacOS || p.Target == Platform::Windows)
                str("SignIdentity", p.SignIdentity);
            str("Team", p.Team);
            if(p.Notarize)
            {
                w.Key("Notarize");
                w.Bool(true);
            }
            str("NotaryProfile", p.NotaryProfile);
            if(p.Zip)
            {
                w.Key("Zip");
                w.Bool(true);
            }
            str("Device", p.Device);
            str("Category", p.Category);
            if(p.Upload)
            {
                w.Key("Upload");
                w.Bool(true);
            }
            if(!p.Exclude.Empty())
            {
                w.Key("Exclude");
                w.StartArray();
                for(auto& e : p.Exclude)
                    w.String(e.c_str());
                w.EndArray();
            }
            w.EndObject();
        }
        w.EndArray();
        w.EndObject();
        std::ofstream f(PathFor(project), std::ios::binary);
        if(!f)
            return false;
        f << sb.GetString() << "\n";
        return (bool)f;
    }

    ExportPreset* PresetFile::Find(const std::string& name)
    {
        for(auto& p : Presets)
            if(p.Name == name)
                return &p;
        // a platform id finds that platform's first preset
        Platform plat;
        if(ParsePlatform(name, plat))
            for(auto& p : Presets)
                if(p.Target == plat)
                    return &p;
        return nullptr;
    }

    ExportPreset PresetFile::Default(const ProjectInfo&, Platform p)
    {
        ExportPreset preset;
        preset.Name   = PlatformName(p);
        preset.Target = p;
        return preset;
    }

    void PresetFile::AddDefaults(const ProjectInfo& project)
    {
        for(int i = 0; i < (int)Platform::Count; i++)
        {
            Platform p = (Platform)i;
            if(!CanExportFrom(p))
                continue;
            bool have = false;
            for(auto& e : Presets)
                if(e.Target == p)
                    have = true;
            if(!have)
                Presets.PushBack(Default(project, p));
        }
    }

    // ---------------------------------------------------------------- resolve

    static std::string FindIcon(const ProjectInfo& project, const ExportPreset& preset)
    {
        std::error_code ec;
        TDArray<std::string> candidates;
        if(!preset.IconPath.empty())
            candidates.PushBack(project.Resolve(preset.IconPath));
        // iOS icons can't have transparency: a project may keep an opaque one beside the main icon
        if(preset.Target == Platform::IOS)
            candidates.PushBack(project.Root + "Assets/Textures/icon_ios.png");
        if(!project.IconPath.empty())
            candidates.PushBack(project.Resolve(project.IconPath));
        for(const char* c : { "Assets/Textures/icon.png", "Assets/Textures/Icon.png", "Assets/icon.png", "icon.png" })
            candidates.PushBack(project.Root + c);
        for(auto& c : candidates)
            if(fs::is_regular_file(c, ec))
                return c;
        return "";
    }

    ResolvedExport Resolve(const ProjectInfo& project, const ExportPreset& preset)
    {
        ResolvedExport r;
        r.AppName     = preset.AppName.empty() ? project.Title : preset.AppName;
        r.BinaryName  = SanitiseName(r.AppName);
        r.BundleId    = preset.BundleId.empty() ? project.BundleIdentifier : preset.BundleId;
        r.Version     = preset.Version.empty() ? project.Version : preset.Version;
        r.BuildNumber = preset.BuildNumber.empty() ? project.BuildNumber : preset.BuildNumber;
        r.IconPng     = FindIcon(project, preset);
        std::string folder = SanitiseName(preset.Name);
        r.OutputDir   = preset.OutputDir.empty() ? project.Root + "Builds/" + folder + "/" : project.Resolve(preset.OutputDir);
        if(!r.OutputDir.empty() && r.OutputDir.back() != '/')
            r.OutputDir += "/";
        r.WorkDir = project.Root + ".lumos/" + folder + "/";
        return r;
    }

    // ---------------------------------------------------------------- templates

    std::string EngineVersion()
    {
        return std::to_string(LumosVersion.major) + "." + std::to_string(LumosVersion.minor) + "." + std::to_string(LumosVersion.patch);
    }

    static std::string ExecutableDir()
    {
        std::string exe;
#if defined(LUMOS_PLATFORM_MACOS)
        char buf[4096];
        uint32_t size = sizeof(buf);
        if(_NSGetExecutablePath(buf, &size) == 0)
            exe = buf;
#elif defined(LUMOS_PLATFORM_WINDOWS)
        char buf[MAX_PATH];
        DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
        exe     = std::string(buf, n);
#elif defined(LUMOS_PLATFORM_LINUX)
        char buf[4096];
        ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if(n > 0)
            exe = std::string(buf, (size_t)n);
#endif
        std::error_code ec;
        fs::path p = fs::weakly_canonical(fs::path(exe), ec);
        return p.parent_path().string();
    }

    static bool IsSourceRoot(const fs::path& p)
    {
        std::error_code ec;
        return fs::exists(p / "Lumos/Source/Lumos/Core/Version.h", ec) && fs::exists(p / "premake5.lua", ec);
    }

    std::string EngineSourceRoot()
    {
        static std::string s_Root = []
        {
            std::error_code ec;
            // act like a distributed editor (testing the bundled templates / docs)
            if(const char* env = getenv("LUMOS_NO_SOURCE"); env && *env == '1')
                return std::string();
            // where this file was compiled from
            fs::path p = fs::path(__FILE__);
            for(int i = 0; i < 5 && p.has_parent_path(); i++)
                p = p.parent_path();
            if(p.is_absolute() && IsSourceRoot(p))
                return fs::weakly_canonical(p, ec).string();
            // else walk up from the binary (bin/<cfg>/Editor.app/Contents/MacOS/...)
            fs::path d = ExecutableDir();
            for(int i = 0; i < 8 && !d.empty(); i++)
            {
                if(IsSourceRoot(d))
                    return d.string();
                if(d == d.parent_path())
                    break;
                d = d.parent_path();
            }
            return std::string();
        }();
        return s_Root;
    }

    static std::string UserDataDir()
    {
#if defined(LUMOS_PLATFORM_WINDOWS)
        const char* appData = getenv("APPDATA");
        return appData ? std::string(appData) + "/Lumos" : std::string();
#elif defined(LUMOS_PLATFORM_MACOS)
        const char* home = getenv("HOME");
        return home ? std::string(home) + "/Library/Application Support/Lumos" : std::string();
#else
        const char* xdg  = getenv("XDG_DATA_HOME");
        const char* home = getenv("HOME");
        if(xdg && *xdg)
            return std::string(xdg) + "/lumos";
        return home ? std::string(home) + "/.local/share/lumos" : std::string();
#endif
    }

    std::string TemplatesRoot()
    {
        std::error_code ec;
        if(const char* env = getenv("LUMOS_TEMPLATES"); env && *env)
            return env;
        std::string src = EngineSourceRoot();
        if(!src.empty())
            return PathJoin(src, "Templates");
        // a distributed editor carries its templates
        fs::path exe = ExecutableDir();
        for(fs::path c : { exe / "Templates", exe / "../Resources/Templates" })
            if(fs::is_directory(c, ec))
                return fs::weakly_canonical(c, ec).string();
        return PathJoin(UserDataDir(), "Templates");
    }

    std::string TemplateDir(Platform p, bool release)
    {
        return PathJoin(TemplatesRoot(), EngineVersion() + "/" + PlatformId(p) + "/" + (release ? "release" : "debug") + "/");
    }

    std::string MacStoreTemplateDir() { return TemplateDir(Platform::MacOS, true) + "store/"; }

    bool MacStoreTemplateInstalled()
    {
        std::error_code ec;
        return fs::exists(MacStoreTemplateDir() + "lib/libRuntime.a", ec) && fs::exists(MacStoreTemplateDir() + "project/LumosApp.xcodeproj", ec);
    }

    bool TemplateInstalled(Platform p, bool release)
    {
        std::error_code ec;
        std::string dir = TemplateDir(p, release);
        switch(p)
        {
        case Platform::MacOS:
            return fs::exists(dir + "Runtime.app/Contents/MacOS/Runtime", ec);
        case Platform::Windows:
            return fs::exists(dir + "Runtime.exe", ec);
        case Platform::Linux:
            return fs::exists(dir + "Runtime", ec);
        case Platform::IOS:
            return fs::exists(dir + "template.json", ec);
        default:
            return false;
        }
    }
}
