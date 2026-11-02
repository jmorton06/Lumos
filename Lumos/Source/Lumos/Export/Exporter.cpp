#include "Precompiled.h"
#include "Exporter.h"
#include "ExportIcons.h"
#include "Core/Asset/AssetPacker.h"

#include <cereal/external/rapidjson/document.h>
#include <cereal/external/rapidjson/stringbuffer.h>
#include <cereal/external/rapidjson/writer.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>

#ifdef LUMOS_PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace fs   = std::filesystem;
namespace json = CEREAL_RAPIDJSON_NAMESPACE;

namespace Lumos::Export
{
    namespace
    {
        double Now()
        {
            using namespace std::chrono;
            return duration<double>(steady_clock::now().time_since_epoch()).count();
        }

        bool ReadText(const std::string& path, std::string& out)
        {
            std::ifstream f(path, std::ios::binary);
            if(!f)
                return false;
            std::stringstream ss;
            ss << f.rdbuf();
            out = ss.str();
            return true;
        }

        bool WriteText(const std::string& path, const std::string& text)
        {
            std::error_code ec;
            fs::create_directories(fs::path(path).parent_path(), ec);
            std::ofstream f(path, std::ios::binary);
            f << text;
            return (bool)f;
        }

        bool RemoveAll(const std::string& path, std::string* error)
        {
            std::error_code ec;
            fs::remove_all(path, ec);
            if(ec && error)
                *error = "Couldn't remove " + path + ": " + ec.message();
            return !ec;
        }

        bool CopyTree(const std::string& from, const std::string& to, std::string* error)
        {
            std::error_code ec;
            fs::create_directories(fs::path(to).parent_path(), ec);
            fs::copy(from, to, fs::copy_options::recursive | fs::copy_options::copy_symlinks | fs::copy_options::overwrite_existing, ec);
            if(ec && error)
                *error = "Couldn't copy " + from + " -> " + to + ": " + ec.message();
            return !ec;
        }

        bool CopyFile(const std::string& from, const std::string& to, std::string* error)
        {
            std::error_code ec;
            fs::create_directories(fs::path(to).parent_path(), ec);
            fs::copy_file(from, to, fs::copy_options::overwrite_existing, ec);
            if(ec && error)
                *error = "Couldn't copy " + from + ": " + ec.message();
            return !ec;
        }

        // <Project>/Package.json DocumentTypes -> CFBundleDocumentTypes as JSON for plutil
        std::string DocumentTypesJson(const ProjectInfo& project)
        {
            std::string text;
            if(!ReadText(project.Root + "Package.json", text))
                return "";
            json::Document doc;
            doc.Parse(text.c_str());
            if(doc.HasParseError() || !doc.IsObject())
                return "";
            auto types = doc.FindMember("DocumentTypes");
            if(types == doc.MemberEnd() || !types->value.IsArray() || types->value.Empty())
                return "";
            auto str = [](const json::Value& v, const char* k, const char* def) -> std::string
            {
                auto it = v.FindMember(k);
                return it != v.MemberEnd() && it->value.IsString() ? it->value.GetString() : def;
            };
            auto list = [](const json::Value& v, const char* k) -> std::string
            {
                auto it = v.FindMember(k);
                if(it == v.MemberEnd() || !it->value.IsArray() || it->value.Empty())
                    return "";
                std::string s = "[";
                for(auto& e : it->value.GetArray())
                    if(e.IsString())
                        s += (s.size() > 1 ? "," : "") + std::string("\"") + e.GetString() + "\"";
                return s + "]";
            };
            std::string out = "[";
            for(auto& d : types->value.GetArray())
            {
                if(!d.IsObject())
                    continue;
                if(out.size() > 1)
                    out += ",";
                out += "{\"CFBundleTypeName\":\"" + str(d, "Name", "Document") + "\",\"CFBundleTypeRole\":\"" + str(d, "Role", "Viewer") +
                       "\",\"LSHandlerRank\":\"" + str(d, "Rank", "Alternate") + "\"";
                std::string ext = list(d, "Extensions"), types = list(d, "ContentTypes");
                if(!ext.empty())
                    out += ",\"CFBundleTypeExtensions\":" + ext;
                if(!types.empty())
                    out += ",\"LSItemContentTypes\":" + types;
                out += "}";
            }
            return out + "]";
        }

        struct PlistEntry
        {
            std::string Key, Json;
        };

        // <Project>/Package.json InfoPlist: { "Key": any json } -> entries for plutil -replace
        TDArray<PlistEntry> InfoPlistExtras(const ProjectInfo& project)
        {
            TDArray<PlistEntry> out;
            std::string text;
            if(!ReadText(project.Root + "Package.json", text))
                return out;
            json::Document doc;
            doc.Parse(text.c_str());
            if(doc.HasParseError() || !doc.IsObject())
                return out;
            auto extras = doc.FindMember("InfoPlist");
            if(extras == doc.MemberEnd() || !extras->value.IsObject())
                return out;
            for(auto& m : extras->value.GetObject())
            {
                json::StringBuffer buf;
                json::Writer<json::StringBuffer> writer(buf);
                m.value.Accept(writer);
                out.PushBack({ m.name.GetString(), buf.GetString() });
            }
            return out;
        }

        std::string FirstEntitlements(const ProjectInfo& project)
        {
            std::error_code ec;
            std::string dir = project.Root + "macOS/";
            std::string preferred = dir + project.Name + ".entitlements";
            if(fs::exists(preferred, ec))
                return preferred;
            if(fs::is_directory(dir, ec))
                for(auto& e : fs::directory_iterator(dir, ec))
                    if(e.path().extension() == ".entitlements")
                        return e.path().string();
            return "";
        }

        std::string LaunchStoryboard(const float colour[4])
        {
            char rgb[160];
            snprintf(rgb, sizeof(rgb), "red=\"%.4f\" green=\"%.4f\" blue=\"%.4f\"", colour[0], colour[1], colour[2]);
            // centred splash at half the width, like the engine splash that follows it
            return std::string(R"(<?xml version="1.0" encoding="UTF-8"?>
<document type="com.apple.InterfaceBuilder3.CocoaTouch.Storyboard.XIB" version="3.0" toolsVersion="17506" targetRuntime="iOS.CocoaTouch" propertyAccessControl="none" useAutolayout="YES" launchScreen="YES" useTraitCollections="YES" useSafeAreas="YES" colorMatched="YES" initialViewController="01J-lp-oVM">
    <device id="retina6_1" orientation="portrait" appearance="light"/>
    <dependencies>
        <deployment identifier="iOS"/>
        <plugIn identifier="com.apple.InterfaceBuilder.IBCocoaTouchPlugin" version="17505"/>
        <capability name="Safe area layout guides" minToolsVersion="9.0"/>
        <capability name="documents saved in the Xcode 8 format" minToolsVersion="8.0"/>
    </dependencies>
    <scenes>
        <scene sceneID="EHf-IW-A2E">
            <objects>
                <viewController id="01J-lp-oVM" sceneMemberID="viewController">
                    <view key="view" contentMode="scaleToFill" id="Ze5-6b-2t3">
                        <rect key="frame" x="0.0" y="0.0" width="414" height="896"/>
                        <autoresizingMask key="autoresizingMask" widthSizable="YES" heightSizable="YES"/>
                        <subviews>
                            <imageView clipsSubviews="YES" userInteractionEnabled="NO" contentMode="scaleAspectFit" image="LaunchSplash.png" translatesAutoresizingMaskIntoConstraints="NO" id="QVp-xv-xe6">
                                <rect key="frame" x="103.5" y="344.5" width="207" height="207"/>
                                <constraints>
                                    <constraint firstAttribute="width" secondItem="QVp-xv-xe6" secondAttribute="height" multiplier="1:1" id="WtK-RP-Ypo"/>
                                </constraints>
                            </imageView>
                        </subviews>
                        <viewLayoutGuide key="safeArea" id="Bcu-3y-fUS"/>
                        <color key="backgroundColor" )") +
                   rgb + R"( alpha="1" colorSpace="custom" customColorSpace="sRGB"/>
                        <constraints>
                            <constraint firstItem="QVp-xv-xe6" firstAttribute="centerX" secondItem="Ze5-6b-2t3" secondAttribute="centerX" id="cx1-aa-001"/>
                            <constraint firstItem="QVp-xv-xe6" firstAttribute="centerY" secondItem="Ze5-6b-2t3" secondAttribute="centerY" id="cy1-aa-002"/>
                            <constraint firstItem="QVp-xv-xe6" firstAttribute="width" secondItem="Ze5-6b-2t3" secondAttribute="width" multiplier="0.5" priority="750" id="w1a-aa-003"/>
                            <constraint firstItem="QVp-xv-xe6" firstAttribute="width" relation="lessThanOrEqual" secondItem="Ze5-6b-2t3" secondAttribute="height" multiplier="0.5" id="w2a-aa-004"/>
                        </constraints>
                    </view>
                </viewController>
                <placeholder placeholderIdentifier="IBFirstResponder" id="iYj-Kq-Ea1" userLabel="First Responder" sceneMemberID="firstResponder"/>
            </objects>
            <point key="canvasLocation" x="52" y="375"/>
        </scene>
    </scenes>
    <resources>
        <image name="LaunchSplash.png" width="512" height="512"/>
    </resources>
</document>
)";
        }

        int CpuCount()
        {
            unsigned n = std::thread::hardware_concurrency();
            return n ? (int)n : 4;
        }

        // newest <dir>/<pattern-ish> match under bin/
        std::string FindBuilt(const std::string& binDir, const std::string& cfgPrefix, const std::string& system, const std::string& leaf)
        {
            std::error_code ec;
            std::string best;
            fs::file_time_type bestTime {};
            if(!fs::is_directory(binDir, ec))
                return "";
            for(auto& e : fs::directory_iterator(binDir, ec))
            {
                std::string name = e.path().filename().string();
                if(name.rfind(cfgPrefix + "-" + system, 0) != 0)
                    continue;
                fs::path p = e.path() / leaf;
                if(fs::exists(p, ec))
                {
                    auto t = fs::last_write_time(p, ec);
                    if(best.empty() || t > bestTime)
                        best = p.string(), bestTime = t;
                }
            }
            return best;
        }
    }

    // ---------------------------------------------------------------- job

    Job::Job(const ProjectInfo& project, const ExportPreset& preset, const Options& options)
        : m_Project(project)
        , m_Preset(preset)
        , m_Options(options)
    {
        m_Resolved         = Resolve(project, preset);
        m_TemplatePlatform = preset.Target;
        m_TemplateRelease  = preset.Release;
    }

    Job::Job(Platform platform, bool release)
    {
        m_Options.JobKind  = Kind::BuildTemplate;
        m_TemplatePlatform = platform;
        m_TemplateRelease  = release;
    }

    u32 Job::LineCount() const
    {
        std::lock_guard<std::mutex> l(m_Lock);
        return m_Lines.Size();
    }

    LogLine Job::Line(u32 i) const
    {
        std::lock_guard<std::mutex> l(m_Lock);
        return i < m_Lines.Size() ? m_Lines[i] : LogLine {};
    }

    std::string Job::CurrentStep() const
    {
        std::lock_guard<std::mutex> l(m_Lock);
        return m_Step;
    }

    std::string Job::ResultPath() const
    {
        std::lock_guard<std::mutex> l(m_Lock);
        return m_Result;
    }

    double Job::Seconds() const { return (m_Finished ? m_End : Now()) - m_Start; }

    std::string Job::Summary() const
    {
        char t[32];
        snprintf(t, sizeof(t), "%.1fs", Seconds());
        if(!m_Finished)
            return CurrentStep() + "...";
        if(m_Cancel)
            return std::string("Cancelled after ") + t;
        if(!m_Succeeded)
            return std::string("Failed after ") + t + ": " + CurrentStep();
        std::string result = ResultPath();
        std::string name   = fs::path(result).filename().string();
        if(m_Options.JobKind == Kind::DeployEditor)
            return "Installed the editor with " + m_Project.Title + " in " + t;
        if(m_Options.JobKind == Kind::BuildTemplate)
            return std::string("Built the ") + PlatformName(m_TemplatePlatform) + " template in " + t;
        if(m_Options.JobKind == Kind::Deploy)
            return "Installed " + m_Resolved.AppName + " on " + result + " in " + t;
        if(m_Options.JobKind == Kind::Archive)
            return (m_Preset.Upload ? "Uploaded " + m_Resolved.AppName + " to App Store Connect in " : "Archived " + name + " in ") + t;
        return "Exported " + name + " in " + t;
    }

    void Job::Log(LogLevel level, const std::string& text)
    {
        LogLine line { text, level };
        {
            std::lock_guard<std::mutex> l(m_Lock);
            m_Lines.PushBack(line);
        }
        if(OnLog)
            OnLog(line);
    }

    void Job::Step(const std::string& name, float progress)
    {
        {
            std::lock_guard<std::mutex> l(m_Lock);
            m_Step = name;
        }
        m_Progress = progress;
        Log(LogLevel::Step, name);
    }

    bool Job::Fail(const std::string& text)
    {
        Log(LogLevel::Error, text);
        return false;
    }

    void Job::Cancel()
    {
        m_Cancel = true;
        std::lock_guard<std::mutex> l(m_Lock);
        if(m_Running)
            m_Running->Kill();
    }

    int Job::Tool(const TDArray<std::string>& args, const std::string& cwd, std::string* capture)
    {
        if(m_Cancel)
            return -1;
        Log(LogLevel::Info, "$ " + Process::Describe(args));
        Process p;
        if(!p.Start(args, cwd))
        {
            Log(LogLevel::Error, "Couldn't run " + args[0]);
            return -1;
        }
        {
            std::lock_guard<std::mutex> l(m_Lock);
            m_Running = &p;
        }
        std::string pending;
        auto flush = [&](bool all)
        {
            size_t nl;
            while((nl = pending.find('\n')) != std::string::npos)
            {
                std::string line = pending.substr(0, nl);
                if(!line.empty() && line.back() == '\r')
                    line.pop_back();
                if(capture)
                    *capture += line + "\n";
                else
                    Log(LogLevel::Tool, line);
                pending.erase(0, nl + 1);
            }
            if(all && !pending.empty())
            {
                if(capture)
                    *capture += pending;
                else
                    Log(LogLevel::Tool, pending);
                pending.clear();
            }
        };
        while(p.IsRunning())
        {
            p.ReadOutput(pending);
            flush(false);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        int code = p.Wait(&pending);
        flush(true);
        {
            std::lock_guard<std::mutex> l(m_Lock);
            m_Running = nullptr;
        }
        return m_Cancel ? -1 : code;
    }

    bool Job::ToolOk(const TDArray<std::string>& args, const std::string& what, const std::string& cwd)
    {
        int code = Tool(args, cwd);
        if(code == 0)
            return true;
        if(m_Cancel)
            return false;
        return Fail(what + " failed (exit " + std::to_string(code) + ")");
    }

    bool Job::Execute()
    {
        m_Start = Now();
        bool ok = false;
        if(m_Options.JobKind == Kind::BuildTemplate)
            ok = BuildTemplate();
        else if(m_Options.JobKind == Kind::DeployEditor)
            ok = DeployEditor();
        else
        {
            Log(LogLevel::Info, std::string("Exporting ") + m_Project.Title + " for " + PlatformName(m_Preset.Target) + " (" + m_Preset.Name + ", " +
                                    (m_Preset.Release ? "release" : "debug") + ")");
            if(!CanExportFrom(m_Preset.Target))
                ok = Fail(std::string(PlatformName(m_Preset.Target)) + " apps can only be exported on " +
                          (m_Preset.Target == Platform::IOS ? "a Mac" : PlatformName(m_Preset.Target)));
            else if(!TemplateInstalled(m_Preset.Target, m_Preset.Release))
                ok = Fail(std::string("No ") + PlatformName(m_Preset.Target) + " " + (m_Preset.Release ? "release" : "debug") + " template in " +
                          TemplateDir(m_Preset.Target, m_Preset.Release) + "\nBuild it: lumos templates build --platform=" + PlatformId(m_Preset.Target) +
                          (m_Preset.Release ? "" : " --config=debug") + "  (or Export window > Build Template)");
            else
            {
                switch(m_Preset.Target)
                {
                case Platform::MacOS: ok = ExportMacOS(); break;
                case Platform::IOS: ok = ExportIOS(); break;
                case Platform::Windows: ok = ExportWindows(); break;
                case Platform::Linux: ok = ExportLinux(); break;
                default: ok = Fail("Unknown platform");
                }
            }
        }
        if(m_Cancel)
            ok = false;
        m_End       = Now();
        m_Progress  = ok ? 1.0f : m_Progress.load();
        m_Succeeded = ok;
        m_Finished  = true;
        Log(ok ? LogLevel::Step : LogLevel::Error, Summary());
        return ok;
    }

    // ---------------------------------------------------------------- shared steps

    bool Job::Pack()
    {
        Step("Packing assets", 0.1f);
        std::error_code ec;
        fs::create_directories(m_Resolved.WorkDir, ec);
        std::string assets = m_Project.Root + "Assets/";
        if(!fs::is_directory(assets, ec))
            return Fail("No Assets folder in " + m_Project.Root);
        PackSettings settings;
        settings.ExcludePaths.PushBack(Str8Lit("Cache/"));
        for(auto& e : m_Preset.Exclude)
            settings.ExcludePaths.PushBack(Str8StdS(e));
        if(!m_Preset.Exclude.Empty())
            Log(LogLevel::Info, "Leaving out paths containing: " + [&] { std::string s; for(auto& e : m_Preset.Exclude) s += (s.empty() ? "" : ", ") + e; return s; }());
        std::string out = m_Resolved.WorkDir + "Assets.lpak";
        if(!AssetPacker::Pack(Str8StdS(out), Str8StdS(assets), settings))
            return Fail("Packing " + assets + " failed");
        Log(LogLevel::Info, "Packed " + std::to_string(fs::file_size(out, ec) / 1024) + " KB -> " + out);
        return !m_Cancel;
    }

    // .lmproj + Assets.lpak (+ privacy manifest on macOS, where this is Contents/Resources) into the app's data folder
    bool Job::CopyProjectFiles(const std::string& destDir)
    {
        std::string err;
        std::error_code ec;
        if(!CopyFile(m_Project.Lmproj, destDir + m_Project.Name + ".lmproj", &err) || !CopyFile(m_Resolved.WorkDir + "Assets.lpak", destDir + "Assets.lpak", &err))
            return Fail(err);
        if(fs::exists(m_Project.Root + "PrivacyInfo.xcprivacy", ec) && m_Preset.Target == Platform::MacOS)
            CopyFile(m_Project.Root + "PrivacyInfo.xcprivacy", destDir + "PrivacyInfo.xcprivacy", &err);
        return true;
    }

    // The privacy manifest the Xcode project bundles at the app's root (the App Store only reads it
    // there): the project's own, or one that declares nothing collected.
    bool Job::WritePrivacyManifest(const std::string& workDir)
    {
        std::string err;
        std::error_code ec;
        const std::string dest = workDir + "PrivacyInfo.xcprivacy";
        if(fs::exists(m_Project.Root + "PrivacyInfo.xcprivacy", ec))
            return CopyFile(m_Project.Root + "PrivacyInfo.xcprivacy", dest, &err) || Fail(err);
        const char* blank = R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>NSPrivacyTracking</key>
	<false/>
	<key>NSPrivacyTrackingDomains</key>
	<array/>
	<key>NSPrivacyCollectedDataTypes</key>
	<array/>
	<key>NSPrivacyAccessedAPITypes</key>
	<array/>
</dict>
</plist>
)";
        return WriteText(dest, blank) || Fail("Couldn't write PrivacyInfo.xcprivacy");
    }

    // ---------------------------------------------------------------- macOS

    bool Job::ExportMacOS()
    {
        std::string err;
        std::error_code ec;
        if(m_Options.JobKind == Kind::Archive)
            return ArchiveMacStore();
        if(!Pack())
            return false;

        Step("Assembling " + m_Resolved.AppName + ".app", 0.4f);
        std::string app = m_Resolved.OutputDir + m_Resolved.AppName + ".app";
        if(!RemoveAll(app, &err) || !CopyTree(TemplateDir(Platform::MacOS, m_Preset.Release) + "Runtime.app", app, &err))
            return Fail(err);
        std::string contents = app + "/Contents/";
        std::string res      = contents + "Resources/";
        fs::rename(contents + "MacOS/Runtime", contents + "MacOS/" + m_Resolved.BinaryName, ec);
        if(ec)
            return Fail("Couldn't rename the executable: " + ec.message());
        // the template's own icon catalogue would win over ours
        fs::remove(res + "Assets.car", ec);
        if(!CopyProjectFiles(res))
            return false;

        if(!m_Resolved.IconPng.empty())
        {
            Step("Icon", 0.5f);
            if(!Icons::WriteIcns(m_Resolved.IconPng, res + "AppIcon.icns", &err))
                return Fail(err);
        }
        else
            Log(LogLevel::Warn, "No icon found - set IconPath in Project Settings or add Assets/Textures/icon.png");

        Step("Info.plist", 0.55f);
        std::string plist = contents + "Info.plist";
        auto set          = [&](const char* key, const std::string& value)
        { return Tool({ "plutil", "-replace", key, "-string", value, plist }) == 0; };
        bool ok = set("CFBundleExecutable", m_Resolved.BinaryName) && set("CFBundleIdentifier", m_Resolved.BundleId) &&
                  set("CFBundleName", m_Resolved.AppName) && set("CFBundleDisplayName", m_Resolved.AppName) &&
                  set("CFBundleShortVersionString", m_Resolved.Version) && set("CFBundleVersion", m_Resolved.BuildNumber);
        if(!ok)
            return Fail("Couldn't write Info.plist");
        if(!m_Resolved.IconPng.empty())
        {
            set("CFBundleIconFile", "AppIcon");
            Tool({ "plutil", "-remove", "CFBundleIconName", plist }, "", &err); // fine if it wasn't there
        }
        set("LSApplicationCategoryType", m_Preset.Category.empty() ? "public.app-category.games" : m_Preset.Category);
        std::string docTypes = DocumentTypesJson(m_Project);
        if(!docTypes.empty() && Tool({ "plutil", "-replace", "CFBundleDocumentTypes", "-json", docTypes, plist }) != 0)
            Log(LogLevel::Warn, "Couldn't add the document types from Package.json");
        for(auto& e : InfoPlistExtras(m_Project))
            if(Tool({ "plutil", "-replace", e.Key, "-json", e.Json, plist }) != 0)
                Log(LogLevel::Warn, "Couldn't set " + e.Key + " from Package.json");

        Step("Signing", 0.7f);
        TDArray<std::string> sign = { "codesign", "--force", "--deep", "--sign", m_Preset.SignIdentity.empty() ? "-" : m_Preset.SignIdentity };
        std::string ent = FirstEntitlements(m_Project);
        bool adhoc      = m_Preset.SignIdentity.empty() || m_Preset.SignIdentity == "-";
        if(!adhoc)
        {
            // distribution: hardened runtime + the project's entitlements
            sign.PushBack("--options");
            sign.PushBack("runtime");
            sign.PushBack("--timestamp");
            if(!ent.empty())
            {
                sign.PushBack("--entitlements");
                sign.PushBack(ent);
            }
        }
        sign.PushBack(app);
        if(!ToolOk(sign, "codesign"))
            return false;

        std::string result = app;
        if(m_Preset.Zip || m_Preset.Notarize)
        {
            Step("Zipping", 0.8f);
            std::string zip = m_Resolved.OutputDir + m_Resolved.BinaryName + "-" + m_Resolved.Version + ".zip";
            fs::remove(zip, ec);
            if(!ToolOk({ "ditto", "-c", "-k", "--keepParent", app, zip }, "ditto"))
                return false;
            result = zip;
            if(m_Preset.Notarize)
            {
                if(adhoc)
                    return Fail("Notarising needs a Developer ID signing identity (SignIdentity)");
                if(m_Preset.NotaryProfile.empty())
                    return Fail("Notarising needs NotaryProfile: run `xcrun notarytool store-credentials <name>` once and put <name> in the preset");
                Step("Notarising (this can take a few minutes)", 0.85f);
                if(!ToolOk({ "xcrun", "notarytool", "submit", zip, "--keychain-profile", m_Preset.NotaryProfile, "--wait" }, "notarytool") ||
                   !ToolOk({ "xcrun", "stapler", "staple", app }, "stapler"))
                    return false;
                // the stapled app goes in a fresh zip
                fs::remove(zip, ec);
                if(!ToolOk({ "ditto", "-c", "-k", "--keepParent", app, zip }, "ditto"))
                    return false;
            }
        }
        {
            std::lock_guard<std::mutex> l(m_Lock);
            m_Result = result;
        }
        if(m_Options.JobKind == Kind::ExportAndRun)
        {
            Step("Launching", 0.95f);
            Tool({ "open", "-n", app });
        }
        return true;
    }

    // ---------------------------------------------------------------- Mac App Store

    // Like the iOS export: a small Xcode project against the prebuilt engine libraries, so Xcode does
    // the App Store signing, provisioning profile and upload.
    bool Job::ArchiveMacStore()
    {
        std::string err;
        std::error_code ec;
        if(!MacStoreTemplateInstalled())
            return Fail("No Mac App Store template in " + MacStoreTemplateDir() + "\nIt comes with the macOS release template: lumos templates build --platform=macos");
        if(m_Preset.Team.empty())
            return Fail("Mac App Store archives need the Team set in the preset");
        if(!Pack())
            return false;

        std::string tmpl = MacStoreTemplateDir();
        std::string work = m_Resolved.WorkDir + "macstore/";
        Step("Generating the Xcode project", 0.25f);
        RemoveAll(work + "LumosApp.xcodeproj", nullptr);
        RemoveAll(work + "Game", nullptr);
        RemoveAll(work + "Assets.xcassets", nullptr);
        if(!CopyTree(tmpl + "project/LumosApp.xcodeproj", work + "LumosApp.xcodeproj", &err) || !CopyFile(tmpl + "project/Stub.swift", work + "Stub.swift", &err) ||
           !CopyFile(tmpl + "project/Info.plist", work + "Info.plist", &err))
            return Fail(err);
        if(!CopyProjectFiles(work + "Game/") || !WritePrivacyManifest(work))
            return false;

        std::string plist = work + "Info.plist";
        Tool({ "plutil", "-replace", "CFBundleShortVersionString", "-string", "$(MARKETING_VERSION)", plist });
        Tool({ "plutil", "-replace", "CFBundleVersion", "-string", "$(CURRENT_PROJECT_VERSION)", plist });
        Tool({ "plutil", "-replace", "CFBundleDisplayName", "-string", m_Resolved.AppName, plist });
        Tool({ "plutil", "-replace", "LSApplicationCategoryType", "-string", m_Preset.Category.empty() ? "public.app-category.games" : m_Preset.Category, plist });
        Tool({ "plutil", "-replace", "NSHumanReadableCopyright", "-string", "", plist });
        std::string docTypes = DocumentTypesJson(m_Project);
        if(!docTypes.empty())
            Tool({ "plutil", "-replace", "CFBundleDocumentTypes", "-json", docTypes, plist });
        for(auto& e : InfoPlistExtras(m_Project))
            Tool({ "plutil", "-replace", e.Key, "-json", e.Json, plist });

        Step("Icon", 0.3f);
        if(m_Resolved.IconPng.empty())
            return Fail("The Mac App Store needs an icon: set IconPath in Project Settings or add Assets/Textures/icon.png");
        if(!Icons::WriteMacAssetCatalog(m_Resolved.IconPng, work + "Assets.xcassets", &err))
            return Fail(err);

        // the store requires the sandbox: the project's own entitlements, else just the sandbox
        std::string ent = FirstEntitlements(m_Project);
        if(ent.empty())
        {
            ent = tmpl + "project/Default.entitlements";
            Log(LogLevel::Warn, "No macOS/" + m_Project.Name + ".entitlements in the project - signing with the sandbox only (no network, no user files)");
        }
        std::string cfg;
        auto line = [&](const std::string& k, const std::string& v) { cfg += k + " = " + v + "\n"; };
        line("PRODUCT_NAME", m_Resolved.AppName);
        line("PRODUCT_BUNDLE_IDENTIFIER", m_Resolved.BundleId);
        line("MARKETING_VERSION", m_Resolved.Version);
        line("CURRENT_PROJECT_VERSION", m_Resolved.BuildNumber);
        line("DEVELOPMENT_TEAM", m_Preset.Team);
        line("LUMOS_TEMPLATE_LIB", tmpl + "lib");
        line("LUMOS_ENTITLEMENTS", ent);
        if(!WriteText(work + "App.xcconfig", cfg))
            return Fail("Couldn't write App.xcconfig");

        Step("Archiving for the Mac App Store (Release, with symbols)", 0.4f);
        std::string archive = work + m_Resolved.BinaryName + ".xcarchive";
        RemoveAll(archive, nullptr);
        if(!ToolOk({ "xcodebuild", "archive", "-project", work + "LumosApp.xcodeproj", "-scheme", "LumosApp", "-configuration", "Release",
                     "-destination", "generic/platform=macOS", "-archivePath", archive, "-allowProvisioningUpdates", "-quiet",
                     "DEBUG_INFORMATION_FORMAT=dwarf-with-dsym", "COMPILER_INDEX_STORE_ENABLE=NO" },
                   "xcodebuild archive"))
            return false;

        Step(m_Preset.Upload ? "Uploading to App Store Connect" : "Exporting the App Store package", 0.8f);
        std::string opts = work + "ExportOptions.plist";
        WriteText(opts, std::string(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>method</key><string>app-store-connect</string>
    <key>teamID</key><string>)") + m_Preset.Team + R"(</string>
    <key>signingStyle</key><string>automatic</string>
    <key>destination</key><string>)" + (m_Preset.Upload ? "upload" : "export") + R"(</string>
    <key>uploadSymbols</key><true/>
</dict>
</plist>
)");
        std::string outDir = m_Resolved.OutputDir + "AppStore/";
        RemoveAll(outDir, nullptr);
        if(!ToolOk({ "xcodebuild", "-exportArchive", "-archivePath", archive, "-exportOptionsPlist", opts, "-exportPath", outDir, "-allowProvisioningUpdates" },
                   m_Preset.Upload ? "Upload" : "Export"))
            return false;
        std::string pkg;
        for(auto& e : fs::directory_iterator(outDir, ec))
            if(e.path().extension() == ".pkg")
                pkg = e.path().string();
        {
            std::lock_guard<std::mutex> l(m_Lock);
            m_Result = pkg.empty() ? archive : pkg;
        }
        Log(LogLevel::Info, m_Preset.Upload ? "Processing on App Store Connect takes a few minutes"
                                            : "Upload the .pkg with Transporter, or set Upload in the preset to send it straight to App Store Connect");
        return true;
    }

    // ---------------------------------------------------------------- iOS

    TDArray<Device> ListIOSDevices(std::string* error)
    {
        TDArray<Device> out;
#ifdef LUMOS_PLATFORM_MACOS
        std::error_code ec;
        std::string tmp = (fs::temp_directory_path(ec) / "lumos-devices.json").string();
        std::string log;
        if(Process::Run({ "xcrun", "devicectl", "list", "devices", "--json-output", tmp }, &log) != 0)
        {
            if(error)
                *error = "devicectl failed (is Xcode installed?)\n" + log;
            return out;
        }
        std::string text;
        ReadText(tmp, text);
        fs::remove(tmp, ec);
        json::Document doc;
        doc.Parse(text.c_str());
        if(doc.HasParseError() || !doc.IsObject())
            return out;
        auto result = doc.FindMember("result");
        if(result == doc.MemberEnd() || !result->value.IsObject())
            return out;
        auto devices = result->value.FindMember("devices");
        if(devices == result->value.MemberEnd() || !devices->value.IsArray())
            return out;
        auto str = [](const json::Value& v, const char* obj, const char* key) -> std::string
        {
            const json::Value* p = &v;
            if(obj)
            {
                auto o = v.FindMember(obj);
                if(o == v.MemberEnd() || !o->value.IsObject())
                    return "";
                p = &o->value;
            }
            auto it = p->FindMember(key);
            return it != p->MemberEnd() && it->value.IsString() ? it->value.GetString() : "";
        };
        for(auto& d : devices->value.GetArray())
        {
            if(!d.IsObject() || str(d, "hardwareProperties", "platform") != "iOS")
                continue;
            Device dev;
            dev.Identifier = str(d, nullptr, "identifier");
            dev.Name       = str(d, "deviceProperties", "name");
            dev.UDID       = str(d, "hardwareProperties", "udid");
            dev.State      = str(d, "connectionProperties", "tunnelState");
            out.PushBack(dev);
        }
#else
        if(error)
            *error = "iOS devices need a Mac";
#endif
        return out;
    }

    bool Job::ExportIOS()
    {
        std::string err;
        std::error_code ec;
        if(!Pack())
            return false;

        std::string tmpl = TemplateDir(Platform::IOS, m_Preset.Release);
        std::string work = m_Resolved.WorkDir + "ios/";
        Step("Generating the Xcode project", 0.25f);
        // keep DerivedData between exports: the second one only re-links
        RemoveAll(work + "LumosApp.xcodeproj", nullptr);
        RemoveAll(work + "Game", nullptr);
        RemoveAll(work + "Assets.xcassets", nullptr);
        if(!CopyTree(tmpl + "project/LumosApp.xcodeproj", work + "LumosApp.xcodeproj", &err) || !CopyFile(tmpl + "project/Stub.swift", work + "Stub.swift", &err))
            return Fail(err);
        if(!CopyProjectFiles(work + "Game/") || !WritePrivacyManifest(work))
            return false;

        // Info.plist: the project's own (background audio etc.) or the stock one
        std::string basePlist = fs::exists(m_Project.Root + "iOS/Info.plist", ec) ? m_Project.Root + "iOS/Info.plist" : tmpl + "project/Info.plist";
        if(!CopyFile(basePlist, work + "Info.plist", &err))
            return Fail(err);
        std::string plist = work + "Info.plist";
        Tool({ "plutil", "-replace", "CFBundleShortVersionString", "-string", "$(MARKETING_VERSION)", plist });
        Tool({ "plutil", "-replace", "CFBundleVersion", "-string", "$(CURRENT_PROJECT_VERSION)", plist });
        Tool({ "plutil", "-replace", "CFBundleDisplayName", "-string", m_Resolved.AppName, plist });
        Tool({ "plutil", "-replace", "UILaunchStoryboardName", "-string", "Launch Screen", plist });
        std::string docTypes = DocumentTypesJson(m_Project);
        if(!docTypes.empty())
            Tool({ "plutil", "-replace", "CFBundleDocumentTypes", "-json", docTypes, plist });
        for(auto& e : InfoPlistExtras(m_Project))
            Tool({ "plutil", "-replace", e.Key, "-json", e.Json, plist });

        Step("Icon + launch screen", 0.3f);
        if(m_Resolved.IconPng.empty())
            return Fail("iOS apps need an icon: set IconPath in Project Settings or add Assets/Textures/icon.png");
        if(!Icons::WriteIOSAssetCatalog(m_Resolved.IconPng, work + "Assets.xcassets", &err))
            return Fail(err);
        std::string splash = m_Project.Resolve(m_Project.SplashPath);
        if(splash.empty() || !fs::exists(splash, ec))
            splash = tmpl + "project/LaunchSplash.png";
        if(!CopyFile(splash, work + "LaunchSplash.png", &err) || !WriteText(work + "Launch Screen.storyboard", LaunchStoryboard(m_Project.SplashColour)))
            return Fail(err.empty() ? "Couldn't write the launch screen" : err);

        // everything per-app is a build setting
        const char* orientPhone = "UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
        const char* orientPad   = "UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
        if(m_Project.Orientation == 1)
            orientPhone = orientPad = "UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown";
        else if(m_Project.Orientation == 2)
            orientPhone = orientPad = "UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
        std::string family = m_Project.DeviceFamily == 1 ? "1" : (m_Project.DeviceFamily == 2 ? "2" : "1,2");
        std::string cfg;
        auto line = [&](const std::string& k, const std::string& v) { cfg += k + " = " + v + "\n"; };
        line("PRODUCT_NAME", m_Resolved.AppName);
        line("PRODUCT_BUNDLE_IDENTIFIER", m_Resolved.BundleId);
        line("MARKETING_VERSION", m_Resolved.Version);
        line("CURRENT_PROJECT_VERSION", m_Resolved.BuildNumber);
        line("DEVELOPMENT_TEAM", m_Preset.Team);
        line("TARGETED_DEVICE_FAMILY", family);
        line("IPHONEOS_DEPLOYMENT_TARGET", m_Project.MinIOSVersion);
        line("LUMOS_TEMPLATE_LIB", tmpl + "lib");
        // a dSYM of the whole engine is most of the build time; only App Store archives need one
        line("DEBUG_INFORMATION_FORMAT", "dwarf");
        line("INFOPLIST_KEY_UISupportedInterfaceOrientations_iPhone", orientPhone);
        line("INFOPLIST_KEY_UISupportedInterfaceOrientations_iPad", orientPad);
        line("INFOPLIST_KEY_UIStatusBarHidden", m_Project.StatusBarHidden ? "YES" : "NO");
        line("INFOPLIST_KEY_ITSAppUsesNonExemptEncryption", m_Project.UsesNonExemptEncryption ? "YES" : "NO");
        if(!m_Project.CameraUsage.empty())
            line("INFOPLIST_KEY_NSCameraUsageDescription", m_Project.CameraUsage);
        if(!m_Project.MicrophoneUsage.empty())
            line("INFOPLIST_KEY_NSMicrophoneUsageDescription", m_Project.MicrophoneUsage);
        if(!m_Project.PhotoLibraryUsage.empty())
            line("INFOPLIST_KEY_NSPhotoLibraryUsageDescription", m_Project.PhotoLibraryUsage);
        if(!m_Project.LocationUsage.empty())
            line("INFOPLIST_KEY_NSLocationWhenInUseUsageDescription", m_Project.LocationUsage);
        if(!WriteText(work + "App.xcconfig", cfg))
            return Fail("Couldn't write App.xcconfig");

        if(m_Preset.Team.empty())
            Log(LogLevel::Warn, "No Team set in the preset: signing for a device will fail (Xcode > Settings > Accounts shows your team id)");

        if(m_Options.JobKind == Kind::Archive)
        {
            if(m_Preset.Team.empty())
                return Fail("App Store archives need the Team set in the preset");
            if(!m_Preset.Release)
                Log(LogLevel::Warn, "Archiving with the debug template - set Config to release for the store");
            Step("Archiving (Release, with symbols)", 0.4f);
            std::string archive = work + m_Resolved.BinaryName + ".xcarchive";
            RemoveAll(archive, nullptr);
            if(!ToolOk({ "xcodebuild", "archive", "-project", work + "LumosApp.xcodeproj", "-scheme", "LumosApp", "-configuration", "Release",
                         "-destination", "generic/platform=iOS", "-archivePath", archive, "-allowProvisioningUpdates", "-quiet",
                         "DEBUG_INFORMATION_FORMAT=dwarf-with-dsym", "COMPILER_INDEX_STORE_ENABLE=NO" },
                       "xcodebuild archive"))
                return false;

            Step(m_Preset.Upload ? "Uploading to App Store Connect" : "Exporting the App Store package", 0.8f);
            std::string opts = work + "ExportOptions.plist";
            WriteText(opts, std::string(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>method</key><string>app-store-connect</string>
    <key>teamID</key><string>)") + m_Preset.Team + R"(</string>
    <key>signingStyle</key><string>automatic</string>
    <key>destination</key><string>)" + (m_Preset.Upload ? "upload" : "export") + R"(</string>
    <key>uploadSymbols</key><true/>
</dict>
</plist>
)");
            std::string outDir = m_Resolved.OutputDir + "AppStore/";
            RemoveAll(outDir, nullptr);
            if(!ToolOk({ "xcodebuild", "-exportArchive", "-archivePath", archive, "-exportOptionsPlist", opts, "-exportPath", outDir, "-allowProvisioningUpdates" },
                       m_Preset.Upload ? "Upload" : "Export"))
                return false;
            std::string ipa;
            for(auto& e : fs::directory_iterator(outDir, ec))
                if(e.path().extension() == ".ipa")
                    ipa = e.path().string();
            {
                std::lock_guard<std::mutex> l(m_Lock);
                m_Result = ipa.empty() ? archive : ipa;
            }
            if(m_Preset.Upload)
                Log(LogLevel::Info, "Processing on App Store Connect takes a few minutes; it then shows up in TestFlight");
            else
                Log(LogLevel::Info, "Upload the .ipa with Transporter, or set Upload in the preset to send it straight to App Store Connect");
            return true;
        }

        Step("Building with Xcode", 0.4f);
        const char* config = m_Preset.Release ? "Release" : "Debug";
        if(!ToolOk({ "xcodebuild", "-project", work + "LumosApp.xcodeproj", "-target", "LumosApp", "-configuration", config, "-sdk", "iphoneos",
                     "-allowProvisioningUpdates", "-quiet", "SYMROOT=" + work + "build", "COMPILER_INDEX_STORE_ENABLE=NO" },
                   "xcodebuild"))
            return false;

        std::string built = work + "build/" + config + "-iphoneos/" + m_Resolved.AppName + ".app";
        std::string app   = m_Resolved.OutputDir + m_Resolved.AppName + ".app";
        if(!fs::exists(built, ec))
            return Fail("Xcode finished but " + built + " isn't there");
        if(!RemoveAll(app, &err) || !CopyTree(built, app, &err))
            return Fail(err);
        {
            std::lock_guard<std::mutex> l(m_Lock);
            m_Result = app;
        }
        if(m_Options.JobKind != Kind::Deploy && m_Options.JobKind != Kind::ExportAndRun)
            return true;

        Step("Finding the device", 0.8f);
        std::string want = m_Options.Device.empty() ? m_Preset.Device : m_Options.Device;
        TDArray<Device> devices = ListIOSDevices(&err);
        const Device* target    = nullptr;
        for(auto& d : devices)
            if(want.empty() || d.Name == want || d.Identifier == want || d.UDID == want)
            {
                target = &d;
                break;
            }
        if(!target)
        {
            std::string names;
            for(auto& d : devices)
                names += "\n  " + d.Name;
            return Fail(want.empty() ? "No iOS device connected" : ("No device called \"" + want + "\". Connected:" + (names.empty() ? std::string(" none") : names)));
        }
        Step("Installing on " + target->Name, 0.85f);
        std::string out;
        int code = Tool({ "xcrun", "devicectl", "device", "install", "app", "--device", target->Identifier, app }, "", &out);
        if(code != 0)
        {
            Log(LogLevel::Tool, out);
            if(out.find("Locked") != std::string::npos || out.find("locked") != std::string::npos)
                return Fail(target->Name + " is locked - unlock it and try again");
            return Fail("Install failed (exit " + std::to_string(code) + ")");
        }
        Step("Launching", 0.95f);
        out.clear();
        code = Tool({ "xcrun", "devicectl", "device", "process", "launch", "--terminate-existing", "--device", target->Identifier, m_Resolved.BundleId }, "", &out);
        if(code != 0)
        {
            Log(LogLevel::Tool, out);
            if(out.find("ocked") != std::string::npos)
                Log(LogLevel::Warn, "Installed, but " + target->Name + " is locked - unlock it and open the app");
            else
                Log(LogLevel::Warn, "Installed, but launching failed");
        }
        {
            std::lock_guard<std::mutex> l(m_Lock);
            m_Result = target->Name;
        }
        return true;
    }

    // ---------------------------------------------------------------- Windows / Linux

#ifdef LUMOS_PLATFORM_WINDOWS
    namespace
    {
        // .ico file -> RT_ICON entries + an RT_GROUP_ICON directory, the way the resource compiler lays them out
        bool SetExeIcon(const std::string& exe, const std::string& ico, std::string* error)
        {
            std::string data;
            if(!ReadText(ico, data) || data.size() < 6)
            {
                *error = "Couldn't read " + ico;
                return false;
            }
            auto rd16  = [&](size_t at) { return (u16)((u8)data[at] | ((u8)data[at + 1] << 8)); };
            auto rd32  = [&](size_t at) { return (u32)rd16(at) | ((u32)rd16(at + 2) << 16); };
            u16 count  = rd16(4);
            HANDLE h  = BeginUpdateResourceA(exe.c_str(), FALSE);
            if(!h)
            {
                *error = "BeginUpdateResource failed";
                return false;
            }
            std::string group(6 + count * 14, '\0');
            memcpy(&group[0], data.data(), 6);
            for(u16 i = 0; i < count; i++)
            {
                size_t e    = 6 + i * 16;
                u32 size    = rd32(e + 8);
                u32 offset  = rd32(e + 12);
                UpdateResourceA(h, MAKEINTRESOURCEA(3), MAKEINTRESOURCEA(i + 1), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (void*)(data.data() + offset), size);
                char* g = &group[6 + i * 14];
                memcpy(g, data.data() + e, 12); // width .. bytes in res
                u16 id = (u16)(i + 1);
                memcpy(g + 12, &id, 2);
            }
            UpdateResourceA(h, MAKEINTRESOURCEA(14), MAKEINTRESOURCEA(1), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), group.data(), (DWORD)group.size());
            if(!EndUpdateResourceA(h, FALSE))
            {
                *error = "EndUpdateResource failed";
                return false;
            }
            return true;
        }
    }
#endif

    bool Job::ExportWindows()
    {
#ifdef LUMOS_PLATFORM_WINDOWS
        std::string err;
        std::error_code ec;
        if(!Pack())
            return false;
        Step("Assembling " + m_Resolved.AppName, 0.4f);
        std::string dir = m_Resolved.OutputDir + m_Resolved.BinaryName + "/";
        if(!RemoveAll(dir, &err) || !CopyTree(TemplateDir(Platform::Windows, m_Preset.Release), dir, &err))
            return Fail(err);
        std::string exe = dir + m_Resolved.BinaryName + ".exe";
        fs::rename(dir + "Runtime.exe", exe, ec);
        if(ec)
            return Fail("Couldn't rename the executable: " + ec.message());
        if(!CopyProjectFiles(dir + "data/"))
            return false;
        if(!m_Resolved.IconPng.empty())
        {
            Step("Icon", 0.6f);
            std::string ico = m_Resolved.WorkDir + "AppIcon.ico";
            if(!Icons::WriteIco(m_Resolved.IconPng, ico, &err) || !SetExeIcon(exe, ico, &err))
                Log(LogLevel::Warn, "Icon: " + err);
        }
        if(!m_Preset.SignIdentity.empty() && m_Preset.SignIdentity != "-")
        {
            Step("Signing", 0.7f);
            ToolOk({ "signtool", "sign", "/fd", "SHA256", "/n", m_Preset.SignIdentity, "/tr", "http://timestamp.digicert.com", "/td", "SHA256", exe }, "signtool");
        }
        std::string result = exe;
        if(m_Preset.Zip)
        {
            Step("Zipping", 0.85f);
            std::string zip = m_Resolved.OutputDir + m_Resolved.BinaryName + "-" + m_Resolved.Version + ".zip";
            fs::remove(zip, ec);
            if(ToolOk({ "powershell", "-NoProfile", "-Command", "Compress-Archive -Path '" + dir + "*' -DestinationPath '" + zip + "'" }, "Compress-Archive"))
                result = zip;
        }
        {
            std::lock_guard<std::mutex> l(m_Lock);
            m_Result = result;
        }
        if(m_Options.JobKind == Kind::ExportAndRun)
        {
            Process run;
            run.Start({ exe }, dir);
        }
        return true;
#else
        return Fail("Windows apps are exported on Windows");
#endif
    }

    bool Job::ExportLinux()
    {
#ifdef LUMOS_PLATFORM_LINUX
        std::string err;
        std::error_code ec;
        if(!Pack())
            return false;
        Step("Assembling " + m_Resolved.AppName, 0.4f);
        std::string dir = m_Resolved.OutputDir + m_Resolved.BinaryName + "/";
        if(!RemoveAll(dir, &err) || !CopyTree(TemplateDir(Platform::Linux, m_Preset.Release), dir, &err))
            return Fail(err);
        std::string exe = dir + m_Resolved.BinaryName;
        fs::rename(dir + "Runtime", exe, ec);
        if(ec)
            return Fail("Couldn't rename the executable: " + ec.message());
        fs::permissions(exe, fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec, fs::perm_options::add, ec);
        if(!CopyProjectFiles(dir + "data/"))
            return false;
        if(!m_Resolved.IconPng.empty())
        {
            Step("Icon", 0.6f);
            if(!Icons::WritePngSet(m_Resolved.IconPng, dir + "icons", &err))
                Log(LogLevel::Warn, "Icon: " + err);
            CopyFile(dir + "icons/icon_256.png", dir + m_Resolved.BinaryName + ".png", &err);
        }
        WriteText(dir + m_Resolved.BinaryName + ".desktop", "[Desktop Entry]\nType=Application\nName=" + m_Resolved.AppName + "\nExec=" + m_Resolved.BinaryName +
                                                                "\nIcon=" + m_Resolved.BinaryName + "\nCategories=Game;\nTerminal=false\n");
        std::string result = exe;
        if(m_Preset.Zip)
        {
            Step("Archiving", 0.85f);
            std::string tar = m_Resolved.OutputDir + m_Resolved.BinaryName + "-" + m_Resolved.Version + ".tar.gz";
            if(ToolOk({ "tar", "-czf", tar, "-C", m_Resolved.OutputDir, m_Resolved.BinaryName }, "tar"))
                result = tar;
        }
        {
            std::lock_guard<std::mutex> l(m_Lock);
            m_Result = result;
        }
        if(m_Options.JobKind == Kind::ExportAndRun)
        {
            Process run;
            run.Start({ exe }, dir);
        }
        return true;
#else
        return Fail("Linux apps are exported on Linux");
#endif
    }

    // ---------------------------------------------------------------- templates from source

    bool Job::DeployEditor()
    {
        const std::string src = EngineSourceRoot();
        Log(LogLevel::Info, "Building the iOS editor with " + m_Project.Title + " inside it");
        if(src.empty())
            return Fail("The iOS editor is built from engine source, which this editor does not have");
        if(!CanExportFrom(Platform::IOS))
            return Fail("The iOS editor can only be built on a Mac");
        Step("Building the editor", 0.1f);
        std::string root = m_Project.Root;
        while(!root.empty() && root.back() == '/')
            root.pop_back();
        TDArray<std::string> args;
        args.PushBack("/bin/bash");
        args.PushBack(src + "/Scripts/MacOS/DeployEditor-iOS.sh");
        args.PushBack("--project=" + root);
        if(!m_Options.Device.empty())
            args.PushBack("--device=" + m_Options.Device);
        return ToolOk(args, "Editor deploy", src);
    }

    bool Job::BuildTemplate()
    {
        std::string src = EngineSourceRoot();
        const char* cfg = m_TemplateRelease ? "release" : "debug";
        Log(LogLevel::Info, std::string("Building the ") + PlatformName(m_TemplatePlatform) + " " + cfg + " template");
        if(src.empty())
            return Fail("Building templates needs the engine source. Without it, install templates for " + EngineVersion() + " into " + TemplatesRoot());
        if(!CanExportFrom(m_TemplatePlatform))
            return Fail(std::string(PlatformName(m_TemplatePlatform)) + " templates are built on " +
                        (m_TemplatePlatform == Platform::IOS ? "a Mac" : PlatformName(m_TemplatePlatform)));
        std::string dest = TemplateDir(m_TemplatePlatform, m_TemplateRelease);
        Log(LogLevel::Info, "Engine source: " + src);
        Log(LogLevel::Info, "Template: " + dest);
        bool ok = false;
        switch(m_TemplatePlatform)
        {
        case Platform::MacOS: ok = BuildTemplateMacOS(src + "/", dest); break;
        case Platform::IOS: ok = BuildTemplateIOS(src + "/", dest); break;
        default: ok = BuildTemplateDesktop(src + "/", dest); break;
        }
        if(ok)
        {
            std::lock_guard<std::mutex> l(m_Lock);
            m_Result = dest;
        }
        return ok;
    }

    // "release" templates are the Production config (no dev tooling), "debug" ones Release (optimised, logs + asserts)
    static const char* TemplateConfig(bool release) { return release ? "Production" : "Release"; }

    bool Job::BuildTemplateMacOS(const std::string& src, const std::string& dest)
    {
        std::string err;
        const char* config = TemplateConfig(m_TemplateRelease);
        Step("Generating the Xcode project", 0.05f);
        // release templates ship Apple Silicon + Intel; debug ones just this Mac
        TDArray<std::string> premake = { src + "Tools/premake5", "xcode4" };
        if(m_TemplateRelease)
            premake.PushBack("--arch=universal");
        if(!ToolOk(premake, "premake", src))
            return false;
        Step(std::string("Compiling the Runtime (") + config + ") - the first build takes a while", 0.1f);
        TDArray<std::string> build = { "xcodebuild", "-project", "Runtime/Runtime.xcodeproj", "-configuration", config, "-parallelizeTargets", "-jobs",
                                       std::to_string(CpuCount()), "-quiet", "CODE_SIGN_IDENTITY=", "CODE_SIGNING_REQUIRED=NO", "CODE_SIGNING_ALLOWED=NO",
                                       "COMPILER_INDEX_STORE_ENABLE=NO" };
        if(m_TemplateRelease)
        {
            build.PushBack("-destination");
            build.PushBack("generic/platform=macOS");
        }
        else
        {
            // just this Mac's architecture (the default would build both, and x86 SIMD only builds for one)
            build.PushBack("-sdk");
            build.PushBack("macosx");
            build.PushBack("-arch");
#if defined(__x86_64__)
            build.PushBack("x86_64");
#else
            build.PushBack("arm64");
#endif
        }
        if(!ToolOk(build, "xcodebuild", src))
            return false;
        std::string app = FindBuilt(src + "bin", config, "macosx", "Runtime.app");
        if(app.empty())
            return Fail(std::string("Built, but no bin/") + config + "-macosx-*/Runtime.app");
        Step("Installing the template", 0.9f);
        if(!RemoveAll(dest + "Runtime.app", &err) || !CopyTree(app, dest + "Runtime.app", &err))
            return Fail(err);
        // dev leftovers that shouldn't ship in every app
        for(const char* junk : { "Tests", "imgui.ini", "pipeline_cache.bin" })
            RemoveAll(dest + "Runtime.app/Contents/Resources/" + junk, nullptr);
        WriteText(dest + "template.json", "{ \"version\": \"" + EngineVersion() + "\", \"platform\": \"macos\", \"config\": \"" + config + "\" }\n");
        // the same build's libraries make the Mac App Store template
        if(m_TemplateRelease && !CollectMacStoreTemplate(src, dest))
            Log(LogLevel::Warn, "The Mac App Store part of the template couldn't be put together - regular exports still work");
        return true;
    }

    bool Job::CollectMacStoreTemplate(const std::string& src, const std::string& dest)
    {
        std::string err;
        std::error_code ec;
        const char* config = TemplateConfig(true);
        std::string libDir = FindBuilt(src + "bin", config, "macosx", "libLumos.a");
        if(libDir.empty())
            return Fail(std::string("No bin/") + config + "-macosx-*/libLumos.a");
        libDir           = fs::path(libDir).parent_path().string() + "/";
        std::string store = dest + "store/";
        Step("Mac App Store template", 0.95f);
        RemoveAll(store, nullptr);
        fs::create_directories(store + "lib", ec);
        for(auto& e : fs::directory_iterator(libDir, ec))
            if(e.path().extension() == ".a")
                CopyFile(e.path().string(), store + "lib/" + e.path().filename().string(), &err);
        CopyFile(src + "Lumos/External/vulkan/libs/macOS/libMoltenVK.a", store + "lib/libMoltenVK.a", &err);

        // the Runtime's own objects, one static lib per architecture, then joined
        std::string objRoot = src + "bin-int/" + fs::path(libDir).parent_path().filename().string() + "/";
        TDArray<std::string> archLibs;
        for(const char* arch : { "arm64", "x86_64" })
        {
            TDArray<std::string> objects;
            for(auto it = fs::recursive_directory_iterator(objRoot, ec); !ec && it != fs::recursive_directory_iterator(); it.increment(ec))
            {
                std::string p = it->path().string();
                if(it->path().extension() == ".o" && p.find("/Runtime.build/") != std::string::npos && p.find(std::string("/") + arch + "/") != std::string::npos)
                    objects.PushBack(p);
            }
            if(objects.Empty())
                continue;
            std::string out = store + "lib/libRuntime-" + arch + ".a";
            TDArray<std::string> libtool = { "libtool", "-static", "-o", out };
            for(auto& o : objects)
                libtool.PushBack(o);
            if(!ToolOk(libtool, "libtool"))
                return false;
            archLibs.PushBack(out);
        }
        if(archLibs.Empty())
            return Fail("No Runtime objects under " + objRoot);
        TDArray<std::string> lipo = { "lipo", "-create", "-output", store + "lib/libRuntime.a" };
        for(auto& l : archLibs)
            lipo.PushBack(l);
        if(!ToolOk(lipo, "lipo"))
            return false;
        for(auto& l : archLibs)
            fs::remove(l, ec);

        std::string skel = src + "Lumos/Assets/ExportTemplates/macos/";
        if(!CopyTree(skel, store + "project", &err) || !CopyFile(src + "Lumos/Source/Lumos/Platform/macOS/Info.plist", store + "project/Info.plist", &err))
            return Fail(err);
        return true;
    }

    bool Job::BuildTemplateIOS(const std::string& src, const std::string& dest)
    {
        std::string err;
        std::error_code ec;
        const char* config = TemplateConfig(m_TemplateRelease);
        Step("Generating the iOS Xcode project", 0.05f);
        if(!ToolOk({ src + "Tools/premake5", "xcode4", "--os=ios" }, "premake", src))
            return false;
        // the Xcode projects are shared with the macOS editor/runtime: put them back
        // whichever way this returns, or the next macOS build quietly targets iOS
        struct RestoreMacProjects
        {
            Job* JobPtr;
            std::string Src;
            ~RestoreMacProjects()
            {
                JobPtr->Log(LogLevel::Info, "Restoring the macOS Xcode projects");
                std::error_code ec;
                const std::string script = Src + "Scripts/MacOS/GenerateXCode.sh";
                if(fs::exists(script, ec))
                    JobPtr->ToolOk({ "/bin/bash", script }, "premake (macOS)", Src);
                else
                    JobPtr->ToolOk({ Src + "Tools/premake5", "xcode4" }, "premake (macOS)", Src);
            }
        } restoreMac { this, src };
        Step(std::string("Compiling the engine for iOS (") + config + ")", 0.1f);
        std::string objRoot = src + "bin-int/lumos-ios-template-" + config + "/";
        if(!ToolOk({ "xcodebuild", "-project", "Runtime/Runtime.xcodeproj", "-target", "Runtime", "-configuration", config, "-sdk", "iphoneos",
                     "-parallelizeTargets", "-jobs", std::to_string(CpuCount()), "-quiet", "CODE_SIGN_IDENTITY=", "CODE_SIGNING_REQUIRED=NO",
                     "CODE_SIGNING_ALLOWED=NO", "COMPILER_INDEX_STORE_ENABLE=NO", "OBJROOT=" + objRoot },
                   "xcodebuild", src))
            return false;

        Step("Collecting libraries", 0.85f);
        std::string libDir = FindBuilt(src + "bin", config, "ios", "libLumos.a");
        if(libDir.empty())
            return Fail(std::string("Built, but no bin/") + config + "-ios-*/libLumos.a");
        libDir = fs::path(libDir).parent_path().string() + "/";
        RemoveAll(dest, nullptr);
        fs::create_directories(dest + "lib", ec);
        for(auto& e : fs::directory_iterator(libDir, ec))
            if(e.path().extension() == ".a")
                CopyFile(e.path().string(), dest + "lib/" + e.path().filename().string(), &err);
        CopyFile(src + "Lumos/External/vulkan/libs/iOS/libMoltenVK.a", dest + "lib/libMoltenVK.a", &err);

        // the Runtime's own objects (main + app class) become libRuntime.a
        TDArray<std::string> objects;
        for(auto it = fs::recursive_directory_iterator(objRoot, ec); it != fs::recursive_directory_iterator(); it.increment(ec))
        {
            std::string p = it->path().string();
            if(it->path().extension() == ".o" && p.find("/Runtime.build/") != std::string::npos && p.find("/arm64/") != std::string::npos)
                objects.PushBack(p);
        }
        if(objects.Empty())
            return Fail("No Runtime objects under " + objRoot);
        TDArray<std::string> libtool = { "libtool", "-static", "-o", dest + "lib/libRuntime.a" };
        for(auto& o : objects)
            libtool.PushBack(o);
        if(!ToolOk(libtool, "libtool"))
            return false;

        Step("Installing the template", 0.95f);
        std::string skel = src + "Lumos/Assets/ExportTemplates/ios/";
        if(!CopyTree(skel, dest + "project", &err) || !CopyFile(src + "Lumos/Source/Lumos/Platform/iOS/Client/Info.plist", dest + "project/Info.plist", &err))
            return Fail(err);
        std::string splash = src + "ExampleProject/Assets/Textures/splash.png";
        if(fs::exists(splash, ec))
            CopyFile(splash, dest + "project/LaunchSplash.png", &err);
        WriteText(dest + "template.json", "{ \"version\": \"" + EngineVersion() + "\", \"platform\": \"ios\", \"config\": \"" + config + "\" }\n");
        return true;
    }

    bool Job::BuildTemplateDesktop(const std::string& src, const std::string& dest)
    {
        std::string err;
        std::error_code ec;
        const char* config = TemplateConfig(m_TemplateRelease);
#if defined(LUMOS_PLATFORM_LINUX)
        Step("Generating makefiles", 0.05f);
        if(!ToolOk({ src + "Tools/linux/premake5", "gmake2" }, "premake", src))
            return false;
        std::string cfgLower = config;
        for(auto& c : cfgLower)
            c = (char)tolower((unsigned char)c);
        Step(std::string("Compiling the Runtime (") + config + ")", 0.1f);
        if(!ToolOk({ "make", "-j" + std::to_string(CpuCount()), "config=" + cfgLower, "Runtime" }, "make", src))
            return false;
        std::string exe = FindBuilt(src + "bin", config, "linux", "Runtime");
        if(exe.empty())
            return Fail(std::string("Built, but no bin/") + config + "-linux-*/Runtime");
        Step("Installing the template", 0.9f);
        RemoveAll(dest, nullptr);
        if(!CopyFile(exe, dest + "Runtime", &err))
            return Fail(err);
        fs::permissions(dest + "Runtime", fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec, fs::perm_options::add, ec);
        // shared libs that sit beside the binary
        for(auto& e : fs::directory_iterator(fs::path(exe).parent_path(), ec))
            if(e.path().extension() == ".so" || e.path().string().find(".so.") != std::string::npos)
                CopyFile(e.path().string(), dest + e.path().filename().string(), &err);
#elif defined(LUMOS_PLATFORM_WINDOWS)
        Step("Generating the Visual Studio solution", 0.05f);
        if(!ToolOk({ src + "Tools/premake5.exe", "vs2022" }, "premake", src))
            return false;
        Step(std::string("Compiling the Runtime (") + config + ")", 0.1f);
        if(!ToolOk({ "msbuild", "Runtime/Runtime.vcxproj", "/m", "/p:Configuration=" + std::string(config), "/p:Platform=x64", "/v:minimal" }, "msbuild", src))
            return false;
        std::string exe = FindBuilt(src + "bin", config, "windows", "Runtime.exe");
        if(exe.empty())
            return Fail(std::string("Built, but no bin/") + config + "-windows-*/Runtime.exe");
        Step("Installing the template", 0.9f);
        RemoveAll(dest, nullptr);
        if(!CopyFile(exe, dest + "Runtime.exe", &err))
            return Fail(err);
        for(auto& e : fs::directory_iterator(fs::path(exe).parent_path(), ec))
            if(e.path().extension() == ".dll")
                CopyFile(e.path().string(), dest + e.path().filename().string(), &err);
#else
        return Fail("Desktop templates are built on their own platform");
#endif
        WriteText(dest + "template.json", "{ \"version\": \"" + EngineVersion() + "\", \"platform\": \"" + PlatformId(m_TemplatePlatform) + "\", \"config\": \"" + config + "\" }\n");
        return true;
    }
}
