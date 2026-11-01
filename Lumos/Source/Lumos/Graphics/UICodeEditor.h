#pragma once
#include "Graphics/UI.h"

namespace Lumos
{
    // Multi-line code editor on the engine UI: selection, undo, clipboard, auto indent,
    // syntax colours and a completion list (Lua: keywords, the binding registry, words
    // already in the file). One per open file; the text lives in the editor.
    struct UICodeEditor;

    enum UICodeLanguage : u8
    {
        UICodeLanguage_Plain = 0,
        UICodeLanguage_Lua,
        UICodeLanguage_Config, // json / ini / toml / yaml...: keys, strings, numbers, # ; // comments
        UICodeLanguage_Shader, // GLSL: // comments, numbers, types and keywords
    };

    UICodeEditor* UICodeEditorCreate();
    void UICodeEditorDestroy(UICodeEditor* editor);

    // Replaces everything and forgets the undo history.
    void UICodeEditorSetText(UICodeEditor* editor, const String8& text);
    // Points into the editor: valid until the next edit.
    String8 UICodeEditorGetText(UICodeEditor* editor);
    void UICodeEditorSetLanguage(UICodeEditor* editor, UICodeLanguage language);

    // Changed since the text was set or marked saved.
    bool UICodeEditorIsDirty(UICodeEditor* editor);
    void UICodeEditorMarkSaved(UICodeEditor* editor);
    // Ctrl/Cmd+S was pressed in the editor since the last call.
    bool UICodeEditorTakeSaveRequest(UICodeEditor* editor);
    // 1-based.
    void UICodeEditorGetCursor(UICodeEditor* editor, u32* line, u32* column);
    void UICodeEditorFocus(UICodeEditor* editor);

    // Builds it at the current place in the UI. Size in layout pixels. True when the text changed.
    bool UICodeEditorDraw(const char* id, UICodeEditor* editor, const Vec2& size);
}
