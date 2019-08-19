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
		static void OnRelease();
        
        inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
        inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
    private:
        static std::shared_ptr<spdlog::logger> s_CoreLogger;
        static std::shared_ptr<spdlog::logger> s_ClientLogger;
    };
}

// Core log macros
#define LUMOS_CORE_TRACE(...)		SPDLOG_LOGGER_CALL(::Lumos::LMLog::GetCoreLogger(), spdlog::level::level_enum::trace, __VA_ARGS__)
#define LUMOS_CORE_INFO(...)		SPDLOG_LOGGER_CALL(::Lumos::LMLog::GetCoreLogger(), spdlog::level::level_enum::info, __VA_ARGS__)
#define LUMOS_CORE_WARN(...)		SPDLOG_LOGGER_CALL(::Lumos::LMLog::GetCoreLogger(), spdlog::level::level_enum::warn, __VA_ARGS__)
#define LUMOS_CORE_ERROR(...)		SPDLOG_LOGGER_CALL(::Lumos::LMLog::GetCoreLogger(), spdlog::level::level_enum::err, __VA_ARGS__)
#define LUMOS_CORE_CRITICAL(...)	SPDLOG_LOGGER_CALL(::Lumos::LMLog::GetCoreLogger(), spdlog::level::level_enum::critical, __VA_ARGS__)

// Client log macros
#define LUMOS_TRACE(...)			SPDLOG_LOGGER_CALL(::Lumos::LMLog::GetClientLogger(), spdlog::level::level_enum::trace, __VA_ARGS__)
#define LUMOS_INFO(...)				SPDLOG_LOGGER_CALL(::Lumos::LMLog::GetClientLogger(), spdlog::level::level_enum::info, __VA_ARGS__)
#define LUMOS_WARN(...)				SPDLOG_LOGGER_CALL(::Lumos::LMLog::GetClientLogger(), spdlog::level::level_enum::warn, __VA_ARGS__)
#define LUMOS_ERROR(...)			SPDLOG_LOGGER_CALL(::Lumos::LMLog::GetClientLogger(), spdlog::level::level_enum::err, __VA_ARGS__)
#define LUMOS_CRITICAL(...)			SPDLOG_LOGGER_CALL(::Lumos::LMLog::GetClientLogger(), spdlog::level::level_enum::critical, __VA_ARGS__)
