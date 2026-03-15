#pragma once
#ifdef LUMOS_PLATFORM_IOS

#include <string>
#include <vector>
#include <functional>

namespace Lumos
{
    void OpenNativeFileDialog(bool selectDirectory,
                              const std::vector<std::string>& filters,
                              const std::string& startPath,
                              const std::function<void(const std::string&)>& callback);
}

#endif
