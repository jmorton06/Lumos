#pragma once
#include "Core.h"

#ifdef LUMOS_PRODUCTION
#define LUMOS_ENABLE_LOG 1
#else
#define LUMOS_ENABLE_LOG 1
#endif

#if LUMOS_ENABLE_LOG
#ifdef LUMOS_PLATFORM_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX // For windows.h
#endif
#endif

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

// Core log macros
#define LUMOS_LOG_TRACE(...) SPDLOG_LOGGER_CALL(::Lumos::Debug::Log::GetCoreLogger(), spdlog::level::level_enum::trace, __VA_ARGS__)
#define LUMOS_LOG_INFO(...) SPDLOG_LOGGER_CALL(::Lumos::Debug::Log::GetCoreLogger(), spdlog::level::level_enum::info, __VA_ARGS__)
#define LUMOS_LOG_WARN(...) SPDLOG_LOGGER_CALL(::Lumos::Debug::Log::GetCoreLogger(), spdlog::level::level_enum::warn, __VA_ARGS__)
#define LUMOS_LOG_ERROR(...) SPDLOG_LOGGER_CALL(::Lumos::Debug::Log::GetCoreLogger(), spdlog::level::level_enum::err, __VA_ARGS__)
#define LUMOS_LOG_CRITICAL(...) SPDLOG_LOGGER_CALL(::Lumos::Debug::Log::GetCoreLogger(), spdlog::level::level_enum::critical, __VA_ARGS__)

#else
namespace spdlog
{
    namespace sinks
    {
        class sink;
    }
    class logger;
}
#define LUMOS_LOG_TRACE(...) ((void)0)
#define LUMOS_LOG_INFO(...) ((void)0)
#define LUMOS_LOG_WARN(...) ((void)0)
#define LUMOS_LOG_ERROR(...) ((void)0)
#define LUMOS_LOG_CRITICAL(...) ((void)0)

#endif

namespace Lumos
{
    namespace Debug
    {
        class LUMOS_EXPORT Log
        {
        public:
            static void OnInit();
            static void OnRelease();

#if LUMOS_ENABLE_LOG
            inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
            static void AddSink(std::shared_ptr<spdlog::sinks::sink>& sink);
#endif

        private:
#if LUMOS_ENABLE_LOG
            static std::shared_ptr<spdlog::logger> s_CoreLogger;
#endif
        };
    }
}
