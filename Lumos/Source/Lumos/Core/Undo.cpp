#include "Core.h"
#include "OS/Memory.h"
#include "Precompiled.h"
#include "Undo.h"

namespace Lumos
{
    static UndoData* s_Undo = nullptr;

    void InitialiseUndo()
    {
        s_Undo                  = new UndoData();
        s_Undo->copy            = ArenaAlloc(UNDO_MEMORY);
        s_Undo->copyTempStart = (u8*)s_Undo->copy->Ptr + s_Undo->copy->Position;
        s_Undo->copyRedoStart = (u8*)s_Undo->copy->Ptr + s_Undo->copy->Position;
    }

    void UndoPush(void* source, i64 size)
    {
        u8* copy = (u8*)ArenaPush(s_Undo->copy, size);
        MemoryCopy(copy, source, size);

        Delta delta  = {};
        delta.copy   = copy;
        delta.size   = size;
        delta.source = (u8*)source;

        s_Undo->delta[s_Undo->temp] = delta;

        s_Undo->temp++;
        s_Undo->copyTempStart = (u8*)s_Undo->copy->Ptr + s_Undo->copy->Position;
    }

    void UndoCommit()
    {
        if(s_Undo->undo >= MAX_UNDOS)
            return;

        s_Undo->tag = 0;

        i32 changes = 0;
        u8* new_pos = s_Undo->copyRedoStart;

        for(i32 i = s_Undo->redo; i < s_Undo->temp; i++)
        {
            Delta c = s_Undo->delta[i];
            if(MemoryCompare(c.copy, c.source, c.size) != 0)
            {
                c.copy                    = (u8*)MemoryMove(new_pos, c.copy, c.size);
                s_Undo->delta[s_Undo->undo++] = c;
                new_pos += c.size;
                changes++;
            }
        }

        if(changes)
        {
            Delta delta               = {};
            delta.copy                = new_pos;
            delta.size                = changes;
            delta.source              = 0;
            s_Undo->delta[s_Undo->undo++] = delta;
            s_Undo->redo                = s_Undo->undo;
            s_Undo->copyRedoStart     = new_pos;
        }

        s_Undo->temp = s_Undo->redo;
        ArenaPopToPointer(s_Undo->copy, new_pos);
        s_Undo->copyTempStart = new_pos;
    }

    void SwapDelta(Delta d)
    {
        if(!d.source)
            return; // skip header

        for(i64 j = 0; j < d.size; j++)
        {
            u8 temp     = *d.copy;
            *d.copy++   = *d.source;
            *d.source++ = temp;
        }
    }
    void Undo()
    {
        if(s_Undo->undo > 0)
        {
            i32 N        = s_Undo->undo - 1;
            Delta header = s_Undo->delta[N];
            i32 changes  = header.size;
            i32 n        = N - changes;
            for(i32 i = N - 1; i >= n; i--)
            {
                SwapDelta(s_Undo->delta[i]);
            }

            Delta first           = s_Undo->delta[n];
            s_Undo->copyRedoStart = first.copy;
            s_Undo->undo          = n;
        }
    }

    void Redo()
    {
        if(s_Undo->redo - s_Undo->undo > 0)
        {
            /* Do not redo while temp stack has delta
               this could be fine if they touch disjoint memory, but if
               there is overlap things will break!
            */
            ASSERT(s_Undo->redo == s_Undo->temp);
            /* Find header for first change */
            i32 N = s_Undo->undo;
            for(; N < s_Undo->redo; N++)
            {
                Delta c = s_Undo->delta[N];
                if(!c.source)
                {
                    break;
                }
            }

            Delta header = s_Undo->delta[N];
            i32 changes  = header.size;
            i32 n        = N - changes;
            for(i32 i = n; i < N; i++)
            {
                SwapDelta(s_Undo->delta[i]);
            }

            s_Undo->copyRedoStart = header.copy;
            s_Undo->undo            = N + 1;
        }
    }
}
