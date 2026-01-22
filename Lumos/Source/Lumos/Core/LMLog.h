#pragma once
#include "Core.h"

#ifdef LUMOS_PRODUCTION
#define LUMOS_ENABLE_LOG 1
#else
#define LUMOS_ENABLE_LOG 1
#endif

#if LUMOS_ENABLE_LOG

// Core log macros
#define LTRACE(...) ::Lumos::Debug::Log::LogOutput(::Lumos::LogLevel::Trace, __FILE__, __LINE__, __VA_ARGS__)
#define LINFO(...) ::Lumos::Debug::Log::LogOutput(::Lumos::LogLevel::Info, __FILE__, __LINE__, __VA_ARGS__)
#define LWARN(...) ::Lumos::Debug::Log::LogOutput(::Lumos::LogLevel::Warning, __FILE__, __LINE__, __VA_ARGS__)
#define LERROR(...) ::Lumos::Debug::Log::LogOutput(::Lumos::LogLevel::Error, __FILE__, __LINE__, __VA_ARGS__)
#define LFATAL(...) ::Lumos::Debug::Log::LogOutput(::Lumos::LogLevel::Fatal, __FILE__, __LINE__, __VA_ARGS__)

#else
#define LTRACE(...) ((void)0)
#define LINFO(...) ((void)0)
#define LWARN(...) ((void)0)
#define LERROR(...) ((void)0)
#define LFATAL(...) ((void)0)

#endif

namespace Lumos
{
    enum LogLevel : u8
    {
        Info    = 0,
        Trace   = 1,
        Warning = 2,
        Error   = 3,
        Fatal   = 4
    };

    typedef void (*LoggerFunction)(LogLevel level, const char* message, const char* file, int line);

    namespace Debug
    {
        struct Log
        {
            static void OnInit();
            static void OnRelease();
            static void SetLoggerFunction(LoggerFunction func);
            static void LogOutput(LogLevel level, const char* file, int line, const char* message, ...);
        };
    }
}
