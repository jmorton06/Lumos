#pragma once

#include "Core/Core.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace Lumos
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
#define LUMOS_CORE_TRACE(...)    ::Lumos::LMLog::GetCoreLogger()->trace(__VA_ARGS__)
#define LUMOS_CORE_INFO(...)     ::Lumos::LMLog::GetCoreLogger()->info(__VA_ARGS__)
#define LUMOS_CORE_WARN(...)     ::Lumos::LMLog::GetCoreLogger()->warn(__VA_ARGS__)
#define LUMOS_CORE_ERROR(...)    ::Lumos::LMLog::GetCoreLogger()->error(__VA_ARGS__)
#define LUMOS_CORE_CRITICAL(...) ::Lumos::LMLog::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define LUMOS_TRACE(...)         ::Lumos::LMLog::GetClientLogger()->trace(__VA_ARGS__)
#define LUMOS_INFO(...)          ::Lumos::LMLog::GetClientLogger()->info(__VA_ARGS__)
#define LUMOS_WARN(...)          ::Lumos::LMLog::GetClientLogger()->warn(__VA_ARGS__)
#define LUMOS_ERROR(...)         ::Lumos::LMLog::GetClientLogger()->error(__VA_ARGS__)
#define LUMOS_CRITICAL(...)      ::Lumos::LMLog::GetClientLogger()->critical(__VA_ARGS__)
