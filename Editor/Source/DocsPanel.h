#pragma once

namespace Lumos
{
    struct UIDockPanelContext;

    // Help > Packaging Guide: Docs/Packaging.md as a dock panel (headings, lists, tables, code blocks with copy).
    namespace DocsPanel
    {
        void Draw(const UIDockPanelContext& context);
        void Release();
    }
}
