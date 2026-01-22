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
}
