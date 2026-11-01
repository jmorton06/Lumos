#pragma once
#include "Core/DataStructures/TDArray.h"
#include <string>

// Exporting a project: what's being exported (ProjectInfo, read straight from the .lmproj so no
// Application is needed), how (ExportPreset, from <Project>/ExportPresets.json) and with what
// (export templates: prebuilt Runtimes per platform, see Templates).
namespace Lumos::Export
{
    enum class Platform
    {
        MacOS,
        IOS,
        Windows,
        Linux,
        Count
    };

    const char* PlatformId(Platform p);   // "macos"
    const char* PlatformName(Platform p); // "macOS"
    bool ParsePlatform(const std::string& s, Platform& out);
    Platform HostPlatform();
    // iOS + macOS export from a Mac, Windows from Windows, Linux from Linux
    bool CanExportFrom(Platform target, Platform host = HostPlatform());

    struct ProjectInfo
    {
        std::string Root;   // with trailing slash
        std::string Lmproj; // full path
        std::string Name;   // .lmproj file name without extension
        std::string Title;
        std::string BundleIdentifier;
        std::string Version     = "1.0.0";
        std::string BuildNumber = "1";
        std::string IconPath; // VFS path as stored
        std::string SplashPath;
        float SplashColour[4]        = { 40.0f / 256.0f, 42.0f / 256.0f, 54.0f / 256.0f, 1.0f };
        int Orientation              = 0; // 0 all, 1 portrait, 2 landscape
        int DeviceFamily             = 3; // 1 iPhone, 2 iPad, 3 both
        std::string MinIOSVersion    = "16.0";
        bool StatusBarHidden         = true;
        bool UsesNonExemptEncryption = false;
        std::string CameraUsage, MicrophoneUsage, PhotoLibraryUsage, LocationUsage;

        // dir holding a .lmproj (or the .lmproj itself)
        bool Load(const std::string& path, std::string* error = nullptr);
        // "//Assets/x.png" -> "<Root>Assets/x.png"; absolute / relative-to-root paths pass through
        std::string Resolve(const std::string& path) const;
    };

    struct ExportPreset
    {
        std::string Name;
        Platform Target = Platform::MacOS;
        bool Release    = true;
        std::string OutputDir; // relative to the project; empty = Builds/<Name>

        // overrides, empty = from the .lmproj
        std::string AppName;
        std::string BundleId;
        std::string IconPath;
        std::string Version;
        std::string BuildNumber;

        // signing: macOS identity ("-" = ad hoc, or "Developer ID Application: ..."), iOS team id
        std::string SignIdentity = "-";
        std::string Team;
        bool Notarize = false;
        std::string NotaryProfile; // `xcrun notarytool store-credentials` profile name
        bool Zip      = false;
        std::string Device; // iOS: device name for Deploy
        bool Upload = false; // App Store archive: send it to App Store Connect instead of writing the .ipa / .pkg
        std::string Category; // macOS: LSApplicationCategoryType, e.g. public.app-category.music (default: games)
        // left out of Assets.lpak: any //Assets/... path containing one of these (saves, test data)
        TDArray<std::string> Exclude;
    };

    struct PresetFile
    {
        TDArray<ExportPreset> Presets;

        static std::string PathFor(const ProjectInfo& project); // <Root>ExportPresets.json
        bool Load(const ProjectInfo& project);                  // false if missing / unreadable
        bool Save(const ProjectInfo& project) const;
        ExportPreset* Find(const std::string& name);
        // one preset per platform this host can export
        void AddDefaults(const ProjectInfo& project);
        static ExportPreset Default(const ProjectInfo& project, Platform p);
    };

    // A preset merged over the project: what an export actually uses
    struct ResolvedExport
    {
        std::string AppName;    // display name
        std::string BinaryName; // AppName without spaces / punctuation
        std::string BundleId;
        std::string Version;
        std::string BuildNumber;
        std::string IconPng; // absolute, "" = none found
        std::string OutputDir;
        std::string WorkDir; // <Root>.lumos/<preset>/
    };
    ResolvedExport Resolve(const ProjectInfo& project, const ExportPreset& preset);

    // ---------------------------------------------------------------- export templates

    std::string EngineVersion(); // "0.5.0"
    // source checkout this binary was built from, if it's still there ("" otherwise)
    std::string EngineSourceRoot();
    // LUMOS_TEMPLATES env, else <source>/Templates, else beside the editor, else the user data dir
    std::string TemplatesRoot();
    // <root>/<version>/<platform>/<release|debug>/
    std::string TemplateDir(Platform p, bool release);
    bool TemplateInstalled(Platform p, bool release);
    // the Mac App Store part of a macOS release template (prebuilt libs + Xcode project)
    std::string MacStoreTemplateDir();
    bool MacStoreTemplateInstalled();

    std::string PathJoin(const std::string& a, const std::string& b);
    std::string SanitiseName(const std::string& s);
}
