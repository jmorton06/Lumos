#include "Precompiled.h"
#include "Thread.h"

namespace Lumos
{
    ThreadContext ThreadContextAlloc()
    {
        ThreadContext tctx = { 0 };
        for(int i = 0; i < 2; i++)
        {
            tctx.ScratchArenas[i] = ArenaAlloc(Megabytes(1));
        }
        return tctx;
    }

    void ThreadContextRelease(ThreadContext* tctx)
    {
        for(int i = 0; i < 2; i++)
        {
            ArenaRelease(tctx->ScratchArenas[i]);
        }
    }

    PerThread ThreadContext ThreadLocalContext;
    void SetThreadContext(ThreadContext* tctx)
    {
        ThreadLocalContext = *tctx;
    }

    ThreadContext* GetThreadContext()
    {
        return &ThreadLocalContext;
    }

    ArenaTemp ScratchBegin(Arena** conflicts, uint64_t conflict_count)
    {
        ThreadContext* tctx = GetThreadContext();

        if(tctx->ScratchArenas[0] == nullptr)
        {
            ThreadLocalContext = ThreadContextAlloc();
            tctx               = &ThreadLocalContext;
        }

        ArenaTemp scratch = { 0 };
        for(uint32_t tctx_idx = 0; tctx_idx < 2; tctx_idx += 1)
        {
            bool is_conflicting = false;
            for(Arena** conflict = conflicts; conflict < conflicts + conflict_count; conflict += 1)
            {
                if(*conflict == tctx->ScratchArenas[tctx_idx])
                {
                    is_conflicting = 1;
                    break;
                }
            }
            if(is_conflicting == 0)
            {
                scratch.arena = tctx->ScratchArenas[tctx_idx];
                scratch.pos   = scratch.arena->Position;
                break;
            }
        }
        return scratch;
    }

    void SetThreadName(String8 string)
    {
        ThreadContext* tctx  = GetThreadContext();
        tctx->ThreadNameSize = string.size < 32 ? string.size : 31;
        MemoryCopy(tctx->ThreadName, string.str, tctx->ThreadNameSize);
    }

    String8 GetThreadName()
    {
        ThreadContext* tctx = GetThreadContext();
        String8 result      = Str8(tctx->ThreadName, tctx->ThreadNameSize);
        return result;
    }

    bool IsMainThread()
    {
        ThreadContext* tctx = GetThreadContext();
        return tctx->MainThread;
    }
}
