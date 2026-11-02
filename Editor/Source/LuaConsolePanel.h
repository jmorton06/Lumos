#pragma once

namespace Lumos
{
    struct UIDockPanelContext;

    // Lua Console panel: a Lua editor run against the open scene (Ctrl / Cmd + Enter), print() captured below it.
    namespace LuaConsolePanel
    {
        void Draw(const UIDockPanelContext& context);
        void Release();
    }
}
