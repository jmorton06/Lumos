#pragma once
#include "Core/Types.h"

namespace Lumos
{
    struct UIDockPanelContext;
    struct UIDockSpace;

    // Pixel art editor on the engine UI ("Sprite Editor" dock panel): pencil, eraser, fill, line,
    // rectangle, colour picker, mirror, palette, undo; saves PNG. A Shape mode sketches the 2D
    // collision polygon over the picture and puts it on the selected entity's RigidBody 2D.
    // Mouse + keyboard (Aseprite keys), touch and pencil (one finger draws, two pan / zoom).
    namespace SpriteEditor
    {
        void SetDock(UIDockSpace* space);
        void DrawPanel(const UIDockPanelContext& context);

        // A picture file (VFS or physical path) into the editor, and the panel to the front.
        bool Open(const char* path);
        // A blank picture (saved under Assets/Sprites when first saved).
        void New(u32 width, u32 height);
        // Straight into the collision shape of this picture.
        void SetShapeMode(bool shape);
        bool GetShapeMode();

        // Drawing tools, for buttons outside the panel (the editor's rail): icon + name with its key.
        u32 GetToolCount();
        u32 GetTool();
        void SetTool(u32 tool);
        const char* GetToolIcon(u32 tool);
        const char* GetToolName(u32 tool);

        // Frees the texture: before the renderer goes.
        void Release();
    }
}
