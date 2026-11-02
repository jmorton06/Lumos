#include "Core.h"
#include "OS/Memory.h"
#include "Precompiled.h"
#include "Undo.h"

namespace Lumos
{
    static UndoData* s_Undo = nullptr;
    static UndoChangedCallback s_ChangedCb = nullptr;
    static void* s_ChangedUserdata = nullptr;
    static UndoResolveFn s_Resolver = nullptr;

    void InitialiseUndo()
    {
        if(s_Undo)
            return;
        s_Undo                  = new UndoData();
        s_Undo->copy            = ArenaAlloc(UNDO_MEMORY);
        s_Undo->copyTempStart = (u8*)s_Undo->copy->Ptr + s_Undo->copy->Position;
        s_Undo->copyRedoStart = (u8*)s_Undo->copy->Ptr + s_Undo->copy->Position;
    }

    void ReleaseUndo()
    {
        if(s_Undo)
        {
            ArenaRelease(s_Undo->copy);
            delete s_Undo;
            s_Undo = nullptr;
        }
    }

    static u8* ResolveSource(const Delta& d)
    {
        if(d.kind != UndoDeltaKind::Snapshot)
            return nullptr;
        if(d.owner != 0 && s_Resolver)
            return (u8*)s_Resolver(d.owner, d.typeKey);
        return d.source;
    }

    static void PushDelta(const Delta& delta)
    {
        if(s_Undo->temp >= MAX_UNDOS - 1)
            return;
        s_Undo->delta[s_Undo->temp] = delta;
        s_Undo->temp++;
        s_Undo->copyTempStart = (u8*)s_Undo->copy->Ptr + s_Undo->copy->Position;
    }

    void UndoPushOwned(void* source, i64 size, u64 owner, u64 typeKey)
    {
        if(!s_Undo || !source || size <= 0)
            return;

        // Already marked this step: a second copy would hold the edited bytes.
        // Only the recent pushes are checked - a paint stroke pushes thousands.
        const i32 firstChecked = s_Undo->temp - 16 > s_Undo->redo ? s_Undo->temp - 16 : s_Undo->redo;
        for(i32 i = firstChecked; i < s_Undo->temp; i++)
        {
            const Delta& c = s_Undo->delta[i];
            if(c.kind == UndoDeltaKind::Snapshot && c.source == (u8*)source && c.size == size)
                return;
        }

        u8* copy = (u8*)ArenaPush(s_Undo->copy, size);
        if(!copy)
            return;
        MemoryCopy(copy, source, size);

        Delta delta   = {};
        delta.copy    = copy;
        delta.size    = size;
        delta.source  = (u8*)source;
        delta.owner   = owner;
        delta.typeKey = (u32)typeKey;
        delta.kind    = UndoDeltaKind::Snapshot;
        PushDelta(delta);
    }

    void UndoPush(void* source, i64 size)
    {
        UndoPushOwned(source, size, 0, 0);
    }

    void UndoPushCommand(UndoCommandFn fn, const void* payload, i64 size)
    {
        if(!s_Undo || !fn)
            return;

        // Never zero bytes: the copy pointer marks where the step starts in the arena.
        const i64 stored = size > 0 ? size : 8;
        u8* copy         = (u8*)ArenaPush(s_Undo->copy, stored);
        if(!copy)
            return;
        if(size > 0 && payload)
            MemoryCopy(copy, payload, size);

        Delta delta   = {};
        delta.copy    = copy;
        delta.size    = stored;
        delta.command = fn;
        delta.kind    = UndoDeltaKind::Command;
        PushDelta(delta);
    }

    static void CommitNow()
    {
        if(s_Undo->undo >= MAX_UNDOS)
            return;

        s_Undo->tag = 0;

        i32 changes = 0;
        u8* new_pos = s_Undo->copyRedoStart;

        for(i32 i = s_Undo->redo; i < s_Undo->temp; i++)
        {
            Delta c      = s_Undo->delta[i];
            bool changed = c.kind == UndoDeltaKind::Command;
            if(c.kind == UndoDeltaKind::Snapshot)
            {
                u8* source = ResolveSource(c);
                changed    = source && MemoryCompare(c.copy, source, c.size) != 0;
                c.source   = source;
            }

            if(changed)
            {
                c.copy                        = (u8*)MemoryMove(new_pos, c.copy, c.size);
                s_Undo->delta[s_Undo->undo++] = c;
                new_pos += c.size;
                changes++;
            }
        }

        if(changes)
        {
            Delta delta                   = {};
            delta.copy                    = new_pos;
            delta.size                    = changes;
            delta.kind                    = UndoDeltaKind::Header;
            delta.label                   = s_Undo->label;
            s_Undo->delta[s_Undo->undo++] = delta;
            s_Undo->redo                  = s_Undo->undo;
            s_Undo->copyRedoStart         = new_pos;
        }

        s_Undo->temp  = s_Undo->redo;
        s_Undo->label = nullptr;
        ArenaPopToPointer(s_Undo->copy, new_pos);
        s_Undo->copyTempStart = new_pos;
    }

    void UndoCommit()
    {
        if(!s_Undo || s_Undo->groupDepth > 0)
            return;
        CommitNow();
    }

    void UndoBeginGroup(const char* label)
    {
        if(!s_Undo)
            return;
        if(s_Undo->groupDepth == 0 && label)
            s_Undo->label = label;
        s_Undo->groupDepth++;
    }

    void UndoEndGroup()
    {
        if(!s_Undo || s_Undo->groupDepth == 0)
            return;
        s_Undo->groupDepth--;
        if(s_Undo->groupDepth == 0)
            CommitNow();
    }

    void UndoSetLabel(const char* label)
    {
        if(s_Undo)
            s_Undo->label = label;
    }

    bool UndoHasPending()
    {
        return s_Undo && s_Undo->temp > s_Undo->redo;
    }

    static void ApplyDelta(const Delta& d, bool undo)
    {
        if(d.kind == UndoDeltaKind::Command)
        {
            if(d.command)
                d.command(d.copy, d.size, undo);
            return;
        }
        if(d.kind != UndoDeltaKind::Snapshot)
            return;

        u8* source = ResolveSource(d);
        if(!source)
            return;
        u8* copy = d.copy;
        for(i64 j = 0; j < d.size; j++)
        {
            u8 temp   = *copy;
            *copy++   = *source;
            *source++ = temp;
        }
    }

    void Undo()
    {
        if(!s_Undo)
            return;
        // an edit still in flight goes first, so it is the thing that gets undone
        if(s_Undo->temp > s_Undo->redo && s_Undo->groupDepth == 0)
            CommitNow();

        if(s_Undo->undo > 0)
        {
            i32 N        = s_Undo->undo - 1;
            Delta header = s_Undo->delta[N];
            i32 changes  = (i32)header.size;
            i32 n        = N - changes;
            for(i32 i = N - 1; i >= n; i--)
            {
                ApplyDelta(s_Undo->delta[i], true);
            }

            Delta first           = s_Undo->delta[n];
            s_Undo->copyRedoStart = first.copy;
            s_Undo->undo          = n;

            if(s_ChangedCb)
                s_ChangedCb(s_ChangedUserdata);
        }
    }

    void Redo()
    {
        if(!s_Undo)
            return;
        if(s_Undo->redo - s_Undo->undo > 0)
        {
            /* Do not redo while temp stack has delta
               this could be fine if they touch disjoint memory, but if
               there is overlap things will break!
            */
            if(s_Undo->redo != s_Undo->temp)
                return;
            /* Find header for first change */
            i32 N = s_Undo->undo;
            for(; N < s_Undo->redo; N++)
            {
                if(s_Undo->delta[N].kind == UndoDeltaKind::Header)
                    break;
            }
            if(N >= s_Undo->redo)
                return;

            Delta header = s_Undo->delta[N];
            i32 changes  = (i32)header.size;
            i32 n        = N - changes;
            for(i32 i = n; i < N; i++)
            {
                ApplyDelta(s_Undo->delta[i], false);
            }

            s_Undo->copyRedoStart = header.copy;
            s_Undo->undo            = N + 1;

            if(s_ChangedCb)
                s_ChangedCb(s_ChangedUserdata);
        }
    }

    void UndoClear()
    {
        if(!s_Undo)
            return;
        ArenaClear(s_Undo->copy);
        s_Undo->undo          = 0;
        s_Undo->redo          = 0;
        s_Undo->temp          = 0;
        s_Undo->tag           = 0;
        s_Undo->groupDepth    = 0;
        s_Undo->label         = nullptr;
        s_Undo->copyTempStart = (u8*)s_Undo->copy->Ptr + s_Undo->copy->Position;
        s_Undo->copyRedoStart = (u8*)s_Undo->copy->Ptr + s_Undo->copy->Position;
    }

    i32 UndoStackDepth()
    {
        return s_Undo ? s_Undo->undo : 0;
    }

    i32 RedoStackDepth()
    {
        return s_Undo ? (s_Undo->redo - s_Undo->undo) : 0;
    }

    i32 UndoStepCount()
    {
        if(!s_Undo)
            return 0;
        i32 count = 0;
        for(i32 i = 0; i < s_Undo->undo; i++)
            if(s_Undo->delta[i].kind == UndoDeltaKind::Header)
                count++;
        return count;
    }

    i32 RedoStepCount()
    {
        if(!s_Undo)
            return 0;
        i32 count = 0;
        for(i32 i = s_Undo->undo; i < s_Undo->redo; i++)
            if(s_Undo->delta[i].kind == UndoDeltaKind::Header)
                count++;
        return count;
    }

    const char* UndoStepLabel(i32 index)
    {
        if(!s_Undo)
            return nullptr;
        for(i32 i = s_Undo->undo - 1; i >= 0; i--)
        {
            if(s_Undo->delta[i].kind != UndoDeltaKind::Header)
                continue;
            if(index-- == 0)
                return s_Undo->delta[i].label;
        }
        return nullptr;
    }

    const char* RedoStepLabel(i32 index)
    {
        if(!s_Undo)
            return nullptr;
        for(i32 i = s_Undo->undo; i < s_Undo->redo; i++)
        {
            if(s_Undo->delta[i].kind != UndoDeltaKind::Header)
                continue;
            if(index-- == 0)
                return s_Undo->delta[i].label;
        }
        return nullptr;
    }

    void SetUndoChangedCallback(UndoChangedCallback cb, void* userdata)
    {
        s_ChangedCb       = cb;
        s_ChangedUserdata = userdata;
    }

    void SetUndoResolver(UndoResolveFn fn)
    {
        s_Resolver = fn;
    }
}
