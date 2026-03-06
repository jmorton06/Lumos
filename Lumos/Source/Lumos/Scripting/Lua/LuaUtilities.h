#pragma once

#include "Sol2Config.h"
#include <sol/sol.hpp>
#include <string>

namespace Lumos
{
    void GenerateLuaStubs(sol::state& lua, const char* outputPath);
}
