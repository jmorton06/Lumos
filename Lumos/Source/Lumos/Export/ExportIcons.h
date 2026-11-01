#pragma once
#include <string>

namespace Lumos::Export
{
    // App icons from one square PNG (1024px ideally). Each returns false with `error` set.
    namespace Icons
    {
        // size + whether any pixel is see-through (iOS flattens those onto black)
        bool Inspect(const std::string& png, int& width, int& height, bool& transparent);
        bool WriteIcns(const std::string& srcPng, const std::string& outIcns, std::string* error = nullptr);
        bool WriteIco(const std::string& srcPng, const std::string& outIco, std::string* error = nullptr);
        // Xcode asset catalog: <outDir>/AppIcon.appiconset (single 1024 universal image, alpha flattened)
        bool WriteIOSAssetCatalog(const std::string& srcPng, const std::string& outXcassets, std::string* error = nullptr);
        // Xcode asset catalog for a Mac app: <outXcassets>/AppIcon.appiconset, 16..512 @1x and @2x
        bool WriteMacAssetCatalog(const std::string& srcPng, const std::string& outXcassets, std::string* error = nullptr);
        // icon_<n>.png for 16..512 into outDir (hicolor sizes)
        bool WritePngSet(const std::string& srcPng, const std::string& outDir, std::string* error = nullptr);
    }
}
