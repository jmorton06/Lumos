#pragma once

namespace Lumos
{
    struct Delta
    {
        i64 size   = 0;
        u8* copy   = nullptr;
        u8* source = nullptr;
    };

#define UNDO_MEMORY Megabytes(10)
#define MAX_UNDOS 0x10000 /* ~3 MB of undos, 7 MB for copied state */
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
    };

    void UndoPush(void* source, i64 size); /* Mark regions that will potentially change */
    void UndoCommit();                     /* Check marked regions and finalize action or discard regions. */
    void Undo();
    void Redo();
    void InitialiseUndo();
    void ReleaseUndo();

    /* Drop all snapshots — call on scene load so stale pointers don't crash swap. */
    void UndoClear();

    /* Diagnostics / UI affordances. UndoStackDepth = how many committed groups
       can be undone; RedoStackDepth = how many are queued for redo. */
    i32 UndoStackDepth();
    i32 RedoStackDepth();

    /* Callback fired after Undo() / Redo() swap memory back into source regions.
       Listeners use it to refresh GPU buffers / collision shapes that mirror
       undone data. Single slot for now — set to nullptr to clear. */
    using UndoChangedCallback = void (*)(void* userdata);
    void SetUndoChangedCallback(UndoChangedCallback cb, void* userdata);
}
