#include "Precompiled.h"
#include "LMLog.h"
#include "Core/OS/OS.h"
#include "String.h"

namespace Lumos::Debug
{
#if LUMOS_ENABLE_LOG
    static LoggerFunction s_LogFunction = 0;
#endif

    void Log::SetLoggerFunction(LoggerFunction func)
    {
        s_LogFunction = func;
    }

    void Log::LogOutput(LogLevel level, const char* file, int line, const char* message, ...)
    {
        static const char* levelStrs[5] = { "[INFO]  : ", "[TRACE] : ", "[WARN]  : ", "[ERROR] : ", "[FATAL] : " };

        ArenaTemp scratch        = ScratchBegin(nullptr, 0);
        String8 formattedMessage = { 0 };
        va_list args;
        va_start(args, message);
        formattedMessage = PushStr8FV(scratch.arena, message, args);
        va_end(args);

        formattedMessage = PushStr8F(scratch.arena, "%s%s \n", levelStrs[(u8)level], formattedMessage.str);
        OS::ConsoleWrite((const char*)formattedMessage.str, u8(level));

        if(s_LogFunction)
        {
            s_LogFunction(level, (const char*)formattedMessage.str, file, line);
        }

        ScratchEnd(scratch);
    }

    void Log::OnInit()
    {
        // #if LUMOS_ENABLE_LOG
        //         sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>()); // debug
        //                                                                                      // sinks.emplace_back(std::make_shared<ImGuiConsoleSink_mt>()); // ImGuiConsole
        //
        // #ifdef LOG_TO_TEXT_FILE
        //         auto logFileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("LumosLog.txt", 1048576 * 5, 3);
        //         sinks.emplace_back(logFileSink); // Log file
        // #endif

        // create the loggers
        //        s_CoreLogger = std::make_shared<spdlog::logger>("Lumos", begin(sinks), end(sinks));
        //        spdlog::register_logger(s_CoreLogger);
        //
        //        // configure the loggers
        //        spdlog::set_pattern("%^[%T] %v%$");
        //        s_CoreLogger->set_level(spdlog::level::trace);
        // #endif
    }

    // #if LUMOS_ENABLE_LOG
    //     void Log::AddSink(spdlog::sink_ptr& sink)
    //     {
    //         s_CoreLogger->sinks().push_back(sink);
    //         s_CoreLogger->set_pattern("%v%$");
    //     }
    // #endif

    void Log::OnRelease()
    {
        // #if LUMOS_ENABLE_LOG
        //         s_CoreLogger.reset();
        //         spdlog::shutdown();
        // #endif
    }
}
