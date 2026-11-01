#pragma once
#include "Core/Core.h"

namespace Lumos
{
    // Spotlight-style launcher on the engine UI: one search box over everything
    // a tool can do. Commands are registered once; providers add results that
    // depend on the query or the scene (entities, files) each time it changes.
    namespace CommandPalette
    {
        using RunFn   = void (*)(u64 arg);
        using StateFn = bool (*)(u64 arg);

        struct CommandDesc
        {
            const char* Name     = nullptr;
            const char* Category = nullptr; // shown at the right, and searched
            const char* Hint     = nullptr; // key binding or a short note
            const char* Icon     = nullptr; // icon glyph
            RunFn Run            = nullptr;
            u64 Arg              = 0;
            StateFn IsOn         = nullptr; // makes it a toggle: shows ON / OFF and stays open
            bool KeepOpen        = false;
            // Listed whatever was typed (the provider has already decided it belongs).
            bool Always          = false;
        };

        // Same name + category replaces what was there.
        void Register(const CommandDesc& desc);
        void Unregister(const char* name, const char* category);
        // Runs a registered command as if it had been picked. category = null matches any. false when there is none.
        bool Run(const char* name, const char* category = nullptr);
        void Clear();

        // Called with the query while the palette is open; adds rows with AddResult.
        using ProviderFn = void (*)(const char* query);
        void AddProvider(ProviderFn provider);
        // Only valid inside a provider. Strings are copied. scoreBias lifts or
        // sinks the row against registered commands.
        void AddResult(const CommandDesc& desc, i32 scoreBias = 0);

        // Modes: a query starting with `prefix` searches only that mode's rows
        // ("@lamp" = entities). The provider gets the query without the prefix.
        // '>' is built in and lists commands only.
        void AddMode(char prefix, const char* label, const char* placeholder, ProviderFn provider);

        void Open(const char* query = nullptr);
        void Close();
        void Toggle();
        bool IsOpen();
        // Put the keyboard in the bar drawn by DrawTrigger (its list opens under it).
        void FocusBar();

        // Once a frame, at root level of the UI it should appear in. Opens on
        // Ctrl/Cmd+K or Ctrl/Cmd+P when handleShortcut is set, and owns the
        // keyboard while it is up.
        void Draw(bool handleShortcut = true);

        // The search pill for a toolbar: looks like the field, opens the palette.
        void DrawTrigger(const char* id, float width, const char* placeholder);

        // Subsequence match with bonuses for word starts and runs. mask marks the
        // matched bytes of text (128 max) for highlighting.
        bool Match(const char* pattern, const char* text, i32& outScore, u64 mask[2]);
    }
}
