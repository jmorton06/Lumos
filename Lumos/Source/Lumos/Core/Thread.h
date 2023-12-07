#pragma once
#include "String.h"

namespace Lumos
{
    struct ThreadContext
    {
        Arena* ScratchArenas[2];
        uint8_t ThreadName[32];
        uint64_t ThreadNameSize;
        bool MainThread;
    };

    ThreadContext ThreadContextAlloc();
    void ThreadContextRelease(ThreadContext* tctx);
    void SetThreadContext(ThreadContext* tctx);
    ThreadContext* GetThreadContext();

    void SetThreadName(String8 string);
    String8 GetThreadName();
    bool IsMainThread();

    ArenaTemp ScratchBegin(Arena** conflicts, uint64_t conflict_count);
#define ScratchEnd(temp) ArenaTempEnd(temp)
}
