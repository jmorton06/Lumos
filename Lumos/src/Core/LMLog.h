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
        
        _FORCE_INLINE_ static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
        _FORCE_INLINE_ static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
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
#define LUMOS_CLIENT_TRACE(...)		SPDLOG_LOGGER_CALL(::Lumos::LMLog::GetClientLogger(), spdlog::level::level_enum::trace, __VA_ARGS__)
#define LUMOS_CLIENT_INFO(...)		SPDLOG_LOGGER_CALL(::Lumos::LMLog::GetClientLogger(), spdlog::level::level_enum::info, __VA_ARGS__)
#define LUMOS_CLIENT_WARN(...)		SPDLOG_LOGGER_CALL(::Lumos::LMLog::GetClientLogger(), spdlog::level::level_enum::warn, __VA_ARGS__)
#define LUMOS_CLIENT_ERROR(...)		SPDLOG_LOGGER_CALL(::Lumos::LMLog::GetClientLogger(), spdlog::level::level_enum::err, __VA_ARGS__)
#define LUMOS_CLIENT_CRITICAL(...)	SPDLOG_LOGGER_CALL(::Lumos::LMLog::GetClientLogger(), spdlog::level::level_enum::critical, __VA_ARGS__)

#ifdef LUMOS_ENGINE
#define LUMOS_LOG_TRACE	LUMOS_CORE_TRACE	
#define LUMOS_LOG_INFO	LUMOS_CORE_INFO
#define LUMOS_LOG_WARN	LUMOS_CORE_WARN
#define LUMOS_LOG_ERROR	LUMOS_CORE_ERROR
#define LUMOS_LOG_CRITICAL LUMOS_CORE_CRITICAL
#else
#define LUMOS_LOG_TRACE LUMOS_CLIENT_TRACE
#define LUMOS_LOG_INFO	LUMOS_CLIENT_INFO
#define LUMOS_LOG_WARN	LUMOS_CLIENT_WARN
#define LUMOS_LOG_ERROR LUMOS_CLIENT_ERROR
#define LUMOS_LOG_CRITICAL LUMOS_CLIENT_CRITICAL
#endif
