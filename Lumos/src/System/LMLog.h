#pragma once

#include "Core.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace lumos
{
    class LUMOS_EXPORT LMLog
    {
    public:
        static void OnInit();
        
        inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
        inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
    private:
        static std::shared_ptr<spdlog::logger> s_CoreLogger;
        static std::shared_ptr<spdlog::logger> s_ClientLogger;
    };
}

// Core log macros
#define LUMOS_CORE_TRACE(...)    ::lumos::LMLog::GetCoreLogger()->trace(__VA_ARGS__)
#define LUMOS_CORE_INFO(...)     ::lumos::LMLog::GetCoreLogger()->info(__VA_ARGS__)
#define LUMOS_CORE_WARN(...)     ::lumos::LMLog::GetCoreLogger()->warn(__VA_ARGS__)
#define LUMOS_CORE_ERROR(...)    ::lumos::LMLog::GetCoreLogger()->error(__VA_ARGS__)
#define LUMOS_CORE_FATAL(...)    ::lumos::LMLog::GetCoreLogger()->fatal(__VA_ARGS__)

// Client log macros
#define LUMOS_TRACE(...)         ::lumos::LMLog::GetClientLogger()->trace(__VA_ARGS__)
#define LUMOS_INFO(...)          ::lumos::LMLog::GetClientLogger()->info(__VA_ARGS__)
#define LUMOS_WARN(...)          ::lumos::LMLog::GetClientLogger()->warn(__VA_ARGS__)
#define LUMOS_ERROR(...)         ::lumos::LMLog::GetClientLogger()->error(__VA_ARGS__)
#define LUMOS_FATAL(...)         ::lumos::LMLog::GetClientLogger()->fatal(__VA_ARGS__)
