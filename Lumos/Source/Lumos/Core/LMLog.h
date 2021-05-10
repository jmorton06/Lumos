#pragma once
#include "Core.h"

#ifdef LUMOS_PLATFORM_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX // For windows.h
#endif
#endif

#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

// Core log macros
#define LUMOS_LOG_TRACE(...) SPDLOG_LOGGER_CALL(::Lumos::Debug::Log::GetCoreLogger(), spdlog::level::level_enum::trace, __VA_ARGS__)
#define LUMOS_LOG_INFO(...) SPDLOG_LOGGER_CALL(::Lumos::Debug::Log::GetCoreLogger(), spdlog::level::level_enum::info, __VA_ARGS__)
#define LUMOS_LOG_WARN(...) SPDLOG_LOGGER_CALL(::Lumos::Debug::Log::GetCoreLogger(), spdlog::level::level_enum::warn, __VA_ARGS__)
#define LUMOS_LOG_ERROR(...) SPDLOG_LOGGER_CALL(::Lumos::Debug::Log::GetCoreLogger(), spdlog::level::level_enum::err, __VA_ARGS__)
#define LUMOS_LOG_CRITICAL(...) SPDLOG_LOGGER_CALL(::Lumos::Debug::Log::GetCoreLogger(), spdlog::level::level_enum::critical, __VA_ARGS__)

namespace Lumos
{
    namespace Debug
    {
        class LUMOS_EXPORT Log
        {
        public:
            static void OnInit();
            static void OnRelease();

            inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }

            static void AddSink(spdlog::sink_ptr& sink);

        private:
            static std::shared_ptr<spdlog::logger> s_CoreLogger;
        };
    }
}
