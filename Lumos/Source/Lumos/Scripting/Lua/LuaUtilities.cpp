#include "Precompiled.h"
#include "LuaUtilities.h"

#include <sol/sol.hpp>

#if __has_include(<filesystem>)
#include <filesystem>
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
#endif

#include <fstream>
#include <iostream>

namespace Lumos
{
    void GenerateLuaStubs(sol::state& lua, const char* outDir)
    {
        const std::string nsName = "LumosEngine";

        sol::table ns = lua.globals();

#if defined(__cpp_lib_filesystem) || defined(__cpp_lib_experimental_filesystem)
        std::filesystem::create_directories(outDir);
#else
        std::string cmd = std::string("mkdir -p ") + outDir;
        std::system(cmd.c_str());
#endif

        std::string outPath = std::string(outDir) + "/" + nsName + ".lua";
        std::ofstream file(outPath, std::ios::out | std::ios::trunc);
        if(!file)
        {
            std::cerr << "Error: cannot open " << outPath << " for writing\n";
            return;
        }

        file << "-- AUTO-GENERATED: " << outPath << "\n";
        file << "---@module " << "Globals" << "\n\n";

        for(auto& kv : ns)
        {
            sol::object value = kv.second;
            if(value.get_type() == sol::type::function)
            {
                std::string name = kv.first.as<std::string>();
                std::cout << name << " is a function\n";
                file << name << " is a function\n";
            }
        }

        lua.open_libraries(sol::lib::debug);
        sol::function getmetatable = lua["debug"]["getmetatable"];

        file << "-- Module-level fields\n";
        for(const auto& kv : ns)
        {
            sol::object keyObj = kv.first;
            sol::object valObj = kv.second;

            if(!keyObj.is<std::string>())
                continue;

            std::string key = keyObj.as<std::string>();
            if(key.empty() || key.find_first_of(" \t\n\r,()[]{}") != std::string::npos)
                continue;

            sol::type t = valObj.get_type();
            switch(t)
            {
            case sol::type::function:
                file << "---@field " << key << " fun(... )\n";
                break;
            case sol::type::number:
                file << "---@field " << key << " number\n";
                break;
            case sol::type::string:
                file << "---@field " << key << " string\n";
                break;
            case sol::type::boolean:
                file << "---@field " << key << " boolean\n";
                break;
            case sol::type::table:
                break;
            default:
                file << "---@field " << key << " any\n";
                break;
            }
        }

        file << "\n-- Classes / types\n";

        for(const auto& kv : ns)
        {
            sol::object keyObj = kv.first;
            sol::object valObj = kv.second;

            if(!keyObj.is<std::string>())
                continue;

            std::string className = keyObj.as<std::string>();
            if(className.empty() || className.find_first_of(" \t\n\r,()[]{}") != std::string::npos)
                continue;

            if(valObj.get_type() != sol::type::table)
                continue;

            sol::table classTbl = valObj.as<sol::table>();

            file << "---@class " << nsName << "." << className << "\n";
            file << "local " << className << " = {}\n";
            file << "---@field new fun(...):" << nsName << "." << className << "\n";

            for(const auto& mkv : classTbl)
            {
                sol::object mkey = mkv.first;
                sol::object mval = mkv.second;
                if(!mkey.is<std::string>())
                    continue;
                std::string mname = mkey.as<std::string>();
                if(mname.empty() || mname.find_first_of(" \t\n\r,()[]{}") != std::string::npos)
                    continue;
                if(mval.get_type() == sol::type::function)
                {
                    file << "---@field " << mname << " fun(self:" << nsName << "." << className << ", ...)\n";
                }
                else
                {
                    file << "---@field " << mname << " any\n";
                }
            }

            file << "\n";
        }

        file << "local " << nsName << " = {}\n";
        file << "return " << nsName << "\n";

        file.close();
        std::cout << "Generated " << outPath << "\n";
    }
}
