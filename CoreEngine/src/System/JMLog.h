#pragma once

#include "Core.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace jm
{
    class JM_EXPORT JMLog
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
#define JM_CORE_TRACE(...)    ::jm::JMLog::GetCoreLogger()->trace(__VA_ARGS__)
#define JM_CORE_INFO(...)     ::jm::JMLog::GetCoreLogger()->info(__VA_ARGS__)
#define JM_CORE_WARN(...)     ::jm::JMLog::GetCoreLogger()->warn(__VA_ARGS__)
#define JM_CORE_ERROR(...)    ::jm::JMLog::GetCoreLogger()->error(__VA_ARGS__)
#define JM_CORE_FATAL(...)    ::jm::JMLog::GetCoreLogger()->fatal(__VA_ARGS__)

// Client log macrosJM
#define JM_TRACE(...)         ::jm::JMLog::GetClientLogger()->trace(__VA_ARGS__)
#define JM_INFO(...)          ::jm::JMLog::GetClientLogger()->info(__VA_ARGS__)
#define JM_WARN(...)          ::jm::JMLog::GetClientLogger()->warn(__VA_ARGS__)
#define JM_ERROR(...)         ::jm::JMLog::GetClientLogger()->error(__VA_ARGS__)
#define JM_FATAL(...)         ::jm::JMLog::GetClientLogger()->fatal(__VA_ARGS__)
