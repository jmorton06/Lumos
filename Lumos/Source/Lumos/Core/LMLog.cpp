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

        if(!message)
            return;

        ArenaTemp scratch        = ScratchBegin(nullptr, 0);
        String8 formattedMessage = { 0 };

        va_list args;
        va_start(args, message);
        formattedMessage = PushStr8FV(scratch.arena, message, args);
        va_end(args);

        formattedMessage = PushStr8F(scratch.arena, " %s%s", levelStrs[(u8)level], formattedMessage.str);
        OS::ConsoleWrite((const char*)formattedMessage.str, u8(level));

        if(s_LogFunction)
        {
            s_LogFunction(level, (const char*)formattedMessage.str, file, line);
        }

        ScratchEnd(scratch);
    }

    void Log::OnInit()
    {
    }

    void Log::OnRelease()
    {
    }
}
