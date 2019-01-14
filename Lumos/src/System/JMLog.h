#pragma once

#include "Core.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace Lumos
{
    class LUMOS_EXPORT JMLog
    {
    public:
        static void Init();
        
        inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
        inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
    private:
        static std::shared_ptr<spdlog::logger> s_CoreLogger;
        static std::shared_ptr<spdlog::logger> s_ClientLogger;
    };
}

// Core log macros
#define LUMOS_CORE_TRACE(...)    ::Lumos::JMLog::GetCoreLogger()->trace(__VA_ARGS__)
#define LUMOS_CORE_INFO(...)     ::Lumos::JMLog::GetCoreLogger()->info(__VA_ARGS__)
#define LUMOS_CORE_WARN(...)     ::Lumos::JMLog::GetCoreLogger()->warn(__VA_ARGS__)
#define LUMOS_CORE_ERROR(...)    ::Lumos::JMLog::GetCoreLogger()->error(__VA_ARGS__)
#define LUMOS_CORE_FATAL(...)    ::Lumos::JMLog::GetCoreLogger()->fatal(__VA_ARGS__)

// Client log macros
#define LUMOS_TRACE(...)         ::Lumos::JMLog::GetClientLogger()->trace(__VA_ARGS__)
#define LUMOS_INFO(...)          ::Lumos::JMLog::GetClientLogger()->info(__VA_ARGS__)
#define LUMOS_WARN(...)          ::Lumos::JMLog::GetClientLogger()->warn(__VA_ARGS__)
#define LUMOS_ERROR(...)         ::Lumos::JMLog::GetClientLogger()->error(__VA_ARGS__)
#define LUMOS_FATAL(...)         ::Lumos::JMLog::GetClientLogger()->fatal(__VA_ARGS__)
