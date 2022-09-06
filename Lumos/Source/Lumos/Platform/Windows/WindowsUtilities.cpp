#include "Precompiled.h"
#include "WindowsUtilities.h"

#include <codecvt>
#include <locale>

namespace Lumos
{
    using convert_t = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_t, wchar_t> strconverter;

    std::string WindowsUtilities::WStringToString(std::wstring wstr)
    {
        return strconverter.to_bytes(wstr);
    }

    std::wstring WindowsUtilities::StringToWString(std::string str)
    {
        return strconverter.from_bytes(str);
    }
}