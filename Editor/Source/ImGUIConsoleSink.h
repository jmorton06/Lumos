#pragma once
#include "ConsolePanel.h"

namespace Lumos
{
    static ConsoleLogLevel GetMessageLevel(const LogLevel level)
    {
        switch(level)
        {
        case LogLevel::Trace:
            return ConsoleLogLevel::Trace;
        case LogLevel::Info:
            return ConsoleLogLevel::Info;
        case LogLevel::Warning:
            return ConsoleLogLevel::Warn;
        case LogLevel::Error:
            return ConsoleLogLevel::Error;
        case LogLevel::Fatal:
            return ConsoleLogLevel::Fatal;
        }
        return ConsoleLogLevel::Trace;
    }

    static void ConsoleLoggerFunction(LogLevel level, const char* message, const char* file, int line)
    {
#if LOG_TIME
        std::string source = fmt::format("File : {0} | Function : {1} | Line : {2}", msg.source.filename, msg.source.funcname, msg.source.line);
        const auto time    = fmt::localtime(std::chrono::system_clock::to_time_t(msg.time));
        auto processed     = fmt::format("{:%H:%M:%S}", time);
#endif
        ConsolePanel::AddMessage(ConsoleMessage(message, GetMessageLevel(level), file));
    }
}
