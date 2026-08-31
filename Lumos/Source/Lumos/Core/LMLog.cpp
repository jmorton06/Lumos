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

#if LUMOS_ENABLE_LOG
    // Fixed ring so logging never allocates. Lines longer than the slot are cut.
    static constexpr u32 kLogRingSize = 256;
    static constexpr u32 kLogLineSize = 220;
    static char s_LogRing[kLogRingSize][kLogLineSize];
    static LogLevel s_LogRingLevels[kLogRingSize];
    static u64 s_LogRingWritten = 0;

    static void PushLogRing(LogLevel level, const char* message)
    {
        u32 slot = (u32)(s_LogRingWritten % kLogRingSize);
        strncpy(s_LogRing[slot], message, kLogLineSize - 1);
        s_LogRing[slot][kLogLineSize - 1] = 0;
        s_LogRingLevels[slot]             = level;
        s_LogRingWritten++;
    }
#endif

    u32 Log::RecentCount()
    {
#if LUMOS_ENABLE_LOG
        return s_LogRingWritten < (u64)kLogRingSize ? (u32)s_LogRingWritten : kLogRingSize;
#else
        return 0;
#endif
    }

    LogRecord Log::GetRecent(u32 i)
    {
#if LUMOS_ENABLE_LOG
        u32 count = RecentCount();
        if(i >= count)
            return { LogLevel::Info, "", 0 };
        u64 first = s_LogRingWritten - count;
        u64 idx   = first + i;
        return { s_LogRingLevels[idx % kLogRingSize], s_LogRing[idx % kLogRingSize], idx };
#else
        return { LogLevel::Info, "", 0 };
#endif
    }

    void Log::ClearRecent()
    {
#if LUMOS_ENABLE_LOG
        s_LogRingWritten = 0;
#endif
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

#if LUMOS_ENABLE_LOG
        PushLogRing(level, (const char*)formattedMessage.str);
#endif

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
