#pragma once

namespace Lumos
{
    enum class UndoDeltaKind : u32
    {
        Snapshot = 0, // bytes swapped with `source`
        Header,       // closes a step; size = number of deltas in it
        Command,      // fn(payload) run on undo / redo
    };

    // undo == true reverts the action, false re-applies it. The action itself has
    // already been done by whoever pushed the command.
    using UndoCommandFn = void (*)(const void* payload, i64 size, bool undo);

    struct Delta
    {
        i64 size = 0;
        u8* copy = nullptr;
        union
        {
            u8* source = nullptr;  // Snapshot
            UndoCommandFn command; // Command
            const char* label;     // Header; string literal
        };

        // Non-zero: `source` is looked up again through the resolver before it is
        // touched (component storage moves when entities come and go).
        u64 owner          = 0;
        u32 typeKey        = 0;
        UndoDeltaKind kind = UndoDeltaKind::Snapshot;
    };

#define UNDO_MEMORY Megabytes(16)
#define MAX_UNDOS 0x10000

    struct UndoData
    {
        Arena* copy;
        Delta delta[MAX_UNDOS];
        i32 undo            = 0;
        i32 redo            = 0;
        i32 temp            = 0;
        u8* copyRedoStart   = nullptr;
        u8* copyTempStart   = nullptr;
        i32 tag             = 0;
        i32 groupDepth      = 0;
        const char* label   = nullptr;
    };

    void UndoPush(void* source, i64 size); /* Mark regions that will potentially change */
    // Same, for memory that can move: `owner` + `typeKey` go to the resolver to find it again.
    void UndoPushOwned(void* source, i64 size, u64 owner, u64 typeKey);
    // Record an action that has just been done. The payload is copied.
    void UndoPushCommand(UndoCommandFn fn, const void* payload, i64 size);
    void UndoCommit(); /* Check marked regions and finalize action or discard regions. */
    void Undo();
    void Redo();

    // Everything pushed between Begin and End becomes one step. Nests.
    void UndoBeginGroup(const char* label = nullptr);
    void UndoEndGroup();
    // Name for the step being built (string literal).
    void UndoSetLabel(const char* label);
    // true when something has been pushed and not committed yet
    bool UndoHasPending();

    void InitialiseUndo();
    void ReleaseUndo();

    /* Drop all snapshots — call on scene load so stale pointers don't crash swap. */
    void UndoClear();

    i32 UndoStackDepth();
    i32 RedoStackDepth();
    // Number of steps that can be undone / redone, and their labels (0 = most recent).
    i32 UndoStepCount();
    i32 RedoStepCount();
    const char* UndoStepLabel(i32 index);
    const char* RedoStepLabel(i32 index);

    using UndoChangedCallback = void (*)(void* userdata);
    void SetUndoChangedCallback(UndoChangedCallback cb, void* userdata);

    using UndoResolveFn = void* (*)(u64 owner, u64 typeKey);
    void SetUndoResolver(UndoResolveFn fn);
}
