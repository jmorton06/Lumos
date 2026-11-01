#include "Precompiled.h"
#include "CommandPalette.h"

#include "Core/OS/Input.h"
#include "Graphics/UI.h"
#include "Graphics/UIInternal.h"
#include "Graphics/UITheme.h"
#include "Maths/MathsUtilities.h"
#include "ImGui/IconsMaterialDesignIcons.h"

namespace Lumos
{
    namespace CommandPalette
    {
        namespace
        {
            char s_KeyHint[16] = { 0 };

            // Open from the bar's own field: the list hangs under the bar and there is no second field.
            bool s_Inline         = false;
            Vec4 s_InlineRect     = Vec4(0.0f); // the bar, x y w h
            u64 s_InlineField     = 0;          // its text field, to hand the keyboard back to
            bool s_InlineRefocus  = false;
            f32 s_InlineListHeight = 0.0f;

            struct Entry
            {
                char Name[80];
                char Category[24];
                char Hint[24];
                char Icon[8];
                RunFn Run;
                u64 Arg;
                StateFn IsOn;
                bool KeepOpen;
                bool Always;
                u64 LastUsed;
                i32 Bias;
            };

            struct Mode
            {
                char Prefix;
                char Label[24];
                char Placeholder[64];
                ProviderFn Provider;
            };

            struct Result
            {
                Entry* entry;
                i32 score;
                u64 mask[2];
            };

            // Fixed pools: nothing is allocated while the palette is in use.
            constexpr u32 kMaxCommands  = 512;
            constexpr u32 kMaxDynamic   = 128;
            constexpr u32 kMaxResults   = 160;
            constexpr u32 kMaxProviders = 16;

            Entry s_Commands[kMaxCommands];
            u32 s_CommandCount = 0;
            Entry s_Dynamic[kMaxDynamic];
            u32 s_DynamicCount = 0;
            Result s_Results[kMaxResults];
            u32 s_ResultCount = 0;
            ProviderFn s_Providers[kMaxProviders];
            u32 s_ProviderCount = 0;
            Mode s_Modes[8];
            u32 s_ModeCount = 0;
            // set by RefreshResults: what is being searched for once any mode prefix is taken off
            const char* s_Needle = "";
            const Mode* s_ActiveMode = nullptr;
            bool s_CommandsOnly      = false;

            bool s_Open            = false;
            bool s_ClaimFocus      = false;
            bool s_BlockedKeyboard = false;
            char s_Query[96]       = { 0 };
            char s_LastQuery[96]   = { 1, 0 };
            i32 s_Selected         = 0;
            i32 s_ScrolledTo       = -1;
            f32 s_Scroll           = 0.0f;
            Vec2 s_LastMouse;
            u64 s_UseCounter = 1;
            u32 s_Refresh    = 0;

            inline char Lower(char c) { return (c >= 'A' && c <= 'Z') ? char(c + 32) : c; }
            inline bool IsBreak(char c) { return c == ' ' || c == ':' || c == '/' || c == '-' || c == '_' || c == '.'; }

            void Copy(char* dst, size_t size, const char* src)
            {
                size_t n = 0;
                if(src)
                    for(; src[n] && n < size - 1; n++)
                        dst[n] = src[n];
                dst[n] = 0;
            }

            void Fill(Entry& e, const CommandDesc& desc)
            {
                Copy(e.Name, sizeof(e.Name), desc.Name);
                Copy(e.Category, sizeof(e.Category), desc.Category);
                Copy(e.Hint, sizeof(e.Hint), desc.Hint);
                Copy(e.Icon, sizeof(e.Icon), desc.Icon);
                e.Run      = desc.Run;
                e.Arg      = desc.Arg;
                e.IsOn     = desc.IsOn;
                e.KeepOpen = desc.KeepOpen;
                e.Always   = desc.Always;
            }

            void Consider(Entry* entry)
            {
                if(s_ResultCount >= kMaxResults)
                    return;

                Result r;
                r.entry   = entry;
                r.score   = 0;
                r.mask[0] = r.mask[1] = 0;

                if(entry->Always)
                {
                    r.score = 1000;
                }
                else if(s_Needle[0])
                {
                    if(!Match(s_Needle, entry->Name, r.score, r.mask))
                    {
                        // the category can carry the match ("theme mid" -> Theme / Midnight)
                        char combined[112];
                        snprintf(combined, sizeof(combined), "%s %s", entry->Category, entry->Name);
                        u64 unused[2];
                        if(!Match(s_Needle, combined, r.score, unused))
                            return;
                        r.score -= 12;
                        r.mask[0] = r.mask[1] = 0;
                    }
                    // what was used lately floats up among equals
                    if(entry->LastUsed)
                        r.score += 4;
                }
                else
                {
                    // nothing typed: most recent first, then the order things were registered in
                    r.score = entry->LastUsed ? (i32)Maths::Min<u64>(entry->LastUsed, 100000) + 100000 : 0;
                }
                r.score += entry->Bias;

                // insertion sort, best first; ties keep their order
                u32 at = s_ResultCount;
                while(at > 0 && s_Results[at - 1].score < r.score)
                {
                    s_Results[at] = s_Results[at - 1];
                    at--;
                }
                s_Results[at] = r;
                s_ResultCount++;
            }

            void RefreshResults()
            {
                s_ResultCount  = 0;
                s_DynamicCount = 0;

                // a leading mode character narrows the search to that mode
                s_Needle       = s_Query;
                s_ActiveMode   = nullptr;
                s_CommandsOnly = false;
                if(s_Query[0] == '>')
                {
                    s_CommandsOnly = true;
                    s_Needle       = s_Query + 1;
                }
                else
                {
                    for(u32 i = 0; i < s_ModeCount; i++)
                    {
                        if(s_Query[0] == s_Modes[i].Prefix)
                        {
                            s_ActiveMode = &s_Modes[i];
                            s_Needle     = s_Query + 1;
                        }
                    }
                }
                while(*s_Needle == ' ')
                    s_Needle++;

                if(s_ActiveMode)
                {
                    if(s_ActiveMode->Provider)
                        s_ActiveMode->Provider(s_Needle);
                }
                else
                {
                    if(!s_CommandsOnly)
                        for(u32 i = 0; i < s_ProviderCount; i++)
                            s_Providers[i](s_Needle);
                    for(u32 i = 0; i < s_CommandCount; i++)
                        Consider(&s_Commands[i]);
                }
                for(u32 i = 0; i < s_DynamicCount; i++)
                    Consider(&s_Dynamic[i]);
            }

            void RunEntry(Entry* entry)
            {
                entry->LastUsed = s_UseCounter++;
                const bool stay = entry->KeepOpen || entry->IsOn != nullptr;
                // copied first: running it can register or drop commands
                const RunFn run = entry->Run;
                const u64 arg   = entry->Arg;
                if(!stay)
                    Close();
                if(run)
                    run(arg);
                s_Refresh = 0;
            }
        }

        bool Match(const char* pattern, const char* text, i32& outScore, u64 mask[2])
        {
            outScore = 0;
            mask[0]  = 0;
            mask[1]  = 0;
            if(!pattern || !pattern[0])
                return true;
            if(!text)
                return false;

            i32 score  = 0;
            u32 ti     = 0;
            u32 last   = 0;
            bool first = true;

            for(u32 pi = 0; pattern[pi]; pi++)
            {
                const char pc = Lower(pattern[pi]);
                if(pc == ' ')
                    continue;

                bool found = false;
                while(text[ti])
                {
                    if(Lower(text[ti]) == pc)
                    {
                        found = true;
                        break;
                    }
                    ti++;
                }
                if(!found)
                    return false;

                if(first)
                {
                    score -= (i32)ti * 2; // matches near the front rank higher
                    if(ti == 0)
                        score += 20;
                    first = false;
                }
                else if(ti == last + 1)
                    score += 12; // run of consecutive letters
                else
                    score -= (i32)(ti - last - 1);

                if(ti > 0 && IsBreak(text[ti - 1]))
                    score += 15; // start of a word
                if(text[ti] == pattern[pi])
                    score += 2;

                if(ti < 128)
                    mask[ti >> 6] |= (1ull << (ti & 63));

                last = ti;
                ti++;
            }

            outScore = score + 10;
            return true;
        }

        void Register(const CommandDesc& desc)
        {
            if(!desc.Name)
                return;
            const char* category = desc.Category ? desc.Category : "";
            for(u32 i = 0; i < s_CommandCount; i++)
            {
                if(strcmp(s_Commands[i].Name, desc.Name) == 0 && strcmp(s_Commands[i].Category, category) == 0)
                {
                    Fill(s_Commands[i], desc);
                    return;
                }
            }
            if(s_CommandCount >= kMaxCommands)
            {
                LWARN("CommandPalette: full, '%s' not registered", desc.Name);
                return;
            }
            Entry& e   = s_Commands[s_CommandCount++];
            e          = {};
            Fill(e, desc);
            s_Refresh = 0;
        }

        void Unregister(const char* name, const char* category)
        {
            for(u32 i = 0; i < s_CommandCount; i++)
            {
                if(strcmp(s_Commands[i].Name, name) == 0 && (!category || strcmp(s_Commands[i].Category, category) == 0))
                {
                    for(u32 k = i + 1; k < s_CommandCount; k++)
                        s_Commands[k - 1] = s_Commands[k];
                    s_CommandCount--;
                    s_Refresh = 0;
                    return;
                }
            }
        }

        bool Run(const char* name, const char* category)
        {
            if(!name)
                return false;
            for(u32 i = 0; i < s_CommandCount; i++)
            {
                if(strcasecmp(s_Commands[i].Name, name) == 0 && (!category || strcasecmp(s_Commands[i].Category, category) == 0) && s_Commands[i].Run)
                {
                    s_Commands[i].Run(s_Commands[i].Arg);
                    return true;
                }
            }
            return false;
        }

        void Clear()
        {
            s_CommandCount  = 0;
            s_ProviderCount = 0;
            s_ModeCount     = 0;
            s_ResultCount   = 0;
            s_ActiveMode    = nullptr;
        }

        void AddProvider(ProviderFn provider)
        {
            if(!provider)
                return;
            for(u32 i = 0; i < s_ProviderCount; i++)
                if(s_Providers[i] == provider)
                    return;
            if(s_ProviderCount < kMaxProviders)
                s_Providers[s_ProviderCount++] = provider;
        }

        void AddMode(char prefix, const char* label, const char* placeholder, ProviderFn provider)
        {
            for(u32 i = 0; i < s_ModeCount; i++)
            {
                if(s_Modes[i].Prefix == prefix)
                {
                    s_Modes[i].Provider = provider;
                    return;
                }
            }
            if(s_ModeCount >= 8)
                return;
            Mode& mode  = s_Modes[s_ModeCount++];
            mode.Prefix = prefix;
            Copy(mode.Label, sizeof(mode.Label), label);
            Copy(mode.Placeholder, sizeof(mode.Placeholder), placeholder);
            mode.Provider = provider;
        }

        void AddResult(const CommandDesc& desc, i32 scoreBias)
        {
            if(!desc.Name || s_DynamicCount >= kMaxDynamic)
                return;
            Entry& e = s_Dynamic[s_DynamicCount++];
            e        = {};
            Fill(e, desc);
            e.Bias = scoreBias;
        }

        void Open(const char* query)
        {
            s_Open       = true;
            s_Inline     = false;
            s_ClaimFocus = true;
            Copy(s_Query, sizeof(s_Query), query);
            s_LastQuery[0] = 1; // never equal to a real query: forces a refresh
            s_LastQuery[1] = 0;
            s_Selected     = 0;
            s_ScrolledTo   = -1;
            s_Scroll       = 0.0f;
        }

        void Close()
        {
            s_Open = false;
            if(s_Inline)
            {
                // the bar goes back to its hint
                s_Inline        = false;
                s_InlineRefocus = false;
                s_Query[0]      = 0;
            }
            if(s_BlockedKeyboard)
            {
                Input::Get().SetKeyboardBlocked(false);
                s_BlockedKeyboard = false;
            }
            UI_State* ui = GetUIState();
            if(ui)
            {
                ui->FocusedTextInput    = 0;
                ui->TextInputBuffer     = nullptr;
                ui->TextInputBufferSize = 0;
            }
        }

        void Toggle()
        {
            if(s_Open)
                Close();
            else
                Open();
        }

        bool IsOpen() { return s_Open; }

        void FocusBar()
        {
            if(s_Open && !s_Inline)
                Close();
            s_InlineRefocus = true;
        }

        void DrawTrigger(const char* id, float width, const char* placeholder)
        {
            UI_State* ui            = GetUIState();
            const f32 dpi           = ui->DPIScale;
            const UIToolTheme& theme = UIGetToolTheme();

            UI_Interaction pill = UIBeginChild(id, { SizeKind_Pixels, width * dpi }, { SizeKind_Pixels, floorf(28.0f * dpi) },
                                               WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY | WidgetFlags_DrawBackground | WidgetFlags_Clickable);
            UI_Widget* widget = ui->parents.Back();
            UIClearBorders(widget);
            widget->style_vars[StyleVar_Padding]      = Vec4(10.0f * dpi, 0.0f, 0.0f, 0.0f);
            widget->style_vars[StyleVar_ItemSpacing]  = Vec4(6.0f * dpi, 0.0f, 0.0f, 0.0f);
            widget->style_vars[StyleVar_CornerRadius] = Vec4(14.0f * dpi, 0.0f, 0.0f, 0.0f);
            UISetBackground(widget, theme.Field, theme.Hover, theme.Hover);

            const bool popup = s_Open && !s_Inline;

            UIPushStyle(StyleVar_TextColor, (s_Open && s_Inline) ? theme.Accent : theme.TextDim);
            UILabelCStr("###icon", ICON_MDI_MAGNIFY);
            UIPopStyle(StyleVar_TextColor);

            bool fieldFocused = false;
            if(popup)
            {
                // the popup has the field for now
                UIPushStyle(StyleVar_TextColor, theme.TextDim);
                UILabelCStr("###text", placeholder);
                UIPopStyle(StyleVar_TextColor);
                UIFlexSpacer();
            }
            else
            {
                if(s_InlineRefocus && s_InlineField)
                {
                    s_InlineRefocus        = false;
                    ui->PendingFocusWidget = s_InlineField;
                }
                // typed into where it is: no box of its own, the bar is the box
                UI_Interaction field = UITextField("###field", s_Query, sizeof(s_Query), placeholder, 0.0f);
                if(field.widget)
                {
                    UISetBackground(field.widget, Vec4(0.0f), Vec4(0.0f), Vec4(0.0f));
                    field.widget->style_vars[StyleVar_Padding] = Vec4(2.0f * dpi, 4.0f * dpi, 0.0f, 0.0f);
                    s_InlineField                              = field.widget->hash;
                }
                fieldFocused = ui->FocusedTextInput != 0 && ui->TextInputBuffer == s_Query;
            }

            Vec4 faint = theme.TextDim;
            faint.w    = 0.7f;
            // not when it would sit on the text
            if(width >= 220.0f)
                UIBadge("###key", Str8C((char*)UIShortcutText("Ctrl P", s_KeyHint, sizeof(s_KeyHint))), theme.Field, faint);
            UIEndChild();

            if(!popup)
            {
                s_InlineRect = Vec4(widget->position.x, widget->position.y, widget->size.x, widget->size.y);
                if(pill.clicked && !fieldFocused && s_InlineField)
                    ui->PendingFocusWidget = s_InlineField; // a press on the bar outside the text
                if(fieldFocused && !s_Open)
                {
                    s_Open         = true;
                    s_Inline       = true;
                    s_LastQuery[0] = 1;
                    s_LastQuery[1] = 0;
                    s_Selected     = 0;
                    s_ScrolledTo   = -1;
                    s_Scroll       = 0.0f;
                }
            }
        }

        void Draw(bool handleShortcut)
        {
            UI_State* ui = GetUIState();
            if(!ui)
                return;
            Input& input = Input::Get();

            if(handleShortcut)
            {
                const bool ctrl = input.GetKeyHeldRaw(InputCode::Key::LeftControl) || input.GetKeyHeldRaw(InputCode::Key::RightControl)
                    || input.GetKeyHeldRaw(InputCode::Key::LeftSuper) || input.GetKeyHeldRaw(InputCode::Key::RightSuper);
                if(ctrl && (input.GetKeyPressedRaw(InputCode::Key::K) || input.GetKeyPressedRaw(InputCode::Key::P)))
                    Toggle();
            }

            if(!s_Open)
                return;

            // typing must not drive whatever is underneath
            if(!s_BlockedKeyboard)
            {
                input.SetKeyboardBlocked(true);
                s_BlockedKeyboard = true;
            }

            const UIToolTheme& theme = UIGetToolTheme();
            const f32 dpi            = ui->DPIScale;
            const Vec2 view          = ui->root_parent.size;

            // the scene changes under an open palette: refresh now and then as well as on edits
            const bool queryChanged = strcmp(s_Query, s_LastQuery) != 0;
            if(queryChanged || s_Refresh == 0)
            {
                RefreshResults();
                if(queryChanged)
                {
                    s_Selected   = 0;
                    s_ScrolledTo = -1;
                    s_Scroll     = 0.0f;
                    Copy(s_LastQuery, sizeof(s_LastQuery), s_Query);
                }
                s_Refresh = 20;
            }
            else
                s_Refresh--;

            const i32 count = (i32)s_ResultCount;
            s_Selected      = Maths::Clamp(s_Selected, 0, Maths::Max(0, count - 1));

            // ---- keys ----
            if(input.GetKeyPressedRaw(InputCode::Key::Escape))
            {
                Close();
                return;
            }
            if(count > 0)
            {
                if(input.GetKeyPressedRaw(InputCode::Key::Down))
                    s_Selected = (s_Selected + 1) % count;
                if(input.GetKeyPressedRaw(InputCode::Key::Up))
                    s_Selected = (s_Selected + count - 1) % count;
                if(input.GetKeyPressedRaw(InputCode::Key::PageDown))
                    s_Selected = Maths::Min(s_Selected + 8, count - 1);
                if(input.GetKeyPressedRaw(InputCode::Key::PageUp))
                    s_Selected = Maths::Max(s_Selected - 8, 0);
                if(input.GetKeyPressedRaw(InputCode::Key::Enter) || input.GetKeyPressedRaw(InputCode::Key::KPEnter))
                {
                    RunEntry(s_Results[s_Selected].entry);
                    if(!s_Open)
                        return;
                    // Enter drops text focus: take it back so typing carries on
                    if(s_Inline)
                        s_InlineRefocus = true;
                    else
                        s_ClaimFocus = true;
                }
            }

            // centred under the bar, kept on screen
            const f32 margin      = 8.0f * dpi;
            const f32 inlineWidth = Maths::Min(Maths::Max(s_InlineRect.z, Maths::Min(560.0f * dpi, view.x * 0.92f)), view.x - margin * 2.0f);
            const f32 inlineX     = Maths::Clamp(s_InlineRect.x + (s_InlineRect.z - inlineWidth) * 0.5f, margin, view.x - inlineWidth - margin);
            const Vec4 inlineList = Vec4(floorf(inlineX), floorf(s_InlineRect.y + s_InlineRect.w + 6.0f * dpi), inlineWidth, view.y);

            // the popup dims what is behind it; under the bar nothing is dimmed and a press elsewhere closes
            bool clickedOutside = false;
            if(s_Inline)
            {
                const Vec2 pointer = UIMousePosition();
                const bool onBar   = pointer.x >= s_InlineRect.x && pointer.x <= s_InlineRect.x + s_InlineRect.z && pointer.y >= s_InlineRect.y && pointer.y <= s_InlineRect.y + s_InlineRect.w;
                const bool onList  = pointer.x >= inlineList.x && pointer.x <= inlineList.x + inlineList.z && pointer.y >= inlineList.y && pointer.y <= inlineList.y + s_InlineListHeight;
                clickedOutside     = input.GetMouseClicked(InputCode::MouseKey::ButtonLeft) && !onBar && !onList;
            }
            else
                clickedOutside = UIDimBackdrop("##palettedim", theme.Dark ? 0.38f : 0.22f).clicked;

            const Vec2 mouse      = input.GetMousePosition();
            const bool mouseMoved = Maths::Abs(mouse.x - s_LastMouse.x) > 0.5f || Maths::Abs(mouse.y - s_LastMouse.y) > 0.5f;
            s_LastMouse           = mouse;

            const f32 rowLogical = 34.0f;
            const f32 rowHeight  = rowLogical * dpi;
            const f32 stride     = rowHeight + 2.0f * dpi;
            const f32 width      = s_Inline ? inlineWidth : Maths::Min(680.0f * dpi, view.x * 0.92f);
            const i32 visible    = (i32)Maths::Clamp(floorf((view.y * 0.5f) / stride), 4.0f, 12.0f);
            const f32 listHeight = (f32)Maths::Min(visible, Maths::Max(count, 1)) * stride + 6.0f * dpi;

            if(s_Selected != s_ScrolledTo)
            {
                s_ScrolledTo     = s_Selected;
                const f32 top    = (f32)s_Selected * stride;
                const f32 bottom = top + rowHeight;
                const f32 window = listHeight - 6.0f * dpi;
                if(top < s_Scroll)
                    s_Scroll = top;
                else if(bottom > s_Scroll + window)
                    s_Scroll = bottom - window;
                s_Scroll = Maths::Clamp(s_Scroll, 0.0f, Maths::Max(0.0f, (f32)count * stride - window));
            }

            UIPushStyle(StyleVar_FontSize, theme.FontSize * 1.08f * dpi);
            UIPushStyle(StyleVar_BackgroundColor, theme.Popup);
            UIPushStyle(StyleVar_Padding, Vec2(10.0f * dpi, 10.0f * dpi));
            UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 6.0f * dpi));
            UIPushStyle(StyleVar_CornerRadius, 14.0f * dpi);
            UIPushStyle(StyleVar_TextColor, theme.Text);
            UIBeginOverlay(s_Inline ? "##commandbarlist" : "##commandpalette", SizeKind_Pixels, width, SizeKind_ChildSum, 1.0f,
                           WidgetFlags_StackVertically | WidgetFlags_AnimateAppear | WidgetFlags_DrawBorder);
            if(s_Inline)
                UIWindowAnchor(UIAnchor_TopLeft, inlineList.x, inlineList.y);
            else
                UIWindowAnchor(UIAnchor_TopCenter, 0.0f, floorf(view.y * 0.13f));
            {
                UI_Widget* panel                           = ui->parents.Back();
                s_InlineListHeight                         = panel->size.y;
                panel->style_vars[StyleVar_Border]         = Vec4(Maths::Max(1.0f, dpi), Maths::Max(1.0f, dpi), 0.0f, 0.0f);
                panel->style_vars[StyleVar_BorderColor]    = theme.Border;
                panel->style_vars[StyleVar_HotBorderColor] = theme.Border;
            }
            UIPopStyle(StyleVar_TextColor);
            UIPopStyle(StyleVar_CornerRadius);
            UIPopStyle(StyleVar_ItemSpacing);
            UIPopStyle(StyleVar_Padding);
            UIPopStyle(StyleVar_BackgroundColor);

            if(!s_Inline)
            {
                // ---- search row ----
                UIBeginRowFullWidth();
                {
                    UI_Widget* row                        = ui->parents.Back();
                    row->style_vars[StyleVar_Padding]     = Vec4(6.0f * dpi, 2.0f * dpi, 0.0f, 0.0f);
                    row->style_vars[StyleVar_ItemSpacing] = Vec4(8.0f * dpi, 0.0f, 0.0f, 0.0f);
                    row->flags |= WidgetFlags_CentreChildrenY;

                    if(s_ClaimFocus)
                    {
                        s_ClaimFocus = false;
                        UIFocusNextTextInput();
                    }

                    UIPushStyle(StyleVar_FontSize, theme.FontSize * 1.35f * dpi);
                    UIPushStyle(StyleVar_TextColor, theme.Accent);
                    UILabelCStr("##paletteicon", ICON_MDI_MAGNIFY);
                    UIPopStyle(StyleVar_TextColor);
                    if(s_ActiveMode || s_CommandsOnly)
                    {
                        UIPushStyle(StyleVar_FontSize, theme.FontSize * dpi);
                        UIBadge("##palettemode", s_ActiveMode ? Str8C((char*)s_ActiveMode->Label) : Str8Lit("Commands"), theme.Selection, theme.Text);
                        UIPopStyle(StyleVar_FontSize);
                    }
                    UI_Interaction field = UISearchField("##palettesearch", s_Query, sizeof(s_Query), "Type a command, a panel or an entity...", 0.0f);
                    if(field.widget)
                    {
                        // no box: the whole panel is the field
                        UISetBackground(field.widget, Vec4(0.0f), Vec4(0.0f), Vec4(0.0f));
                        field.widget->style_vars[StyleVar_Padding] = Vec4(2.0f * dpi, 6.0f * dpi, 0.0f, 0.0f);
                    }
                    UIPopStyle(StyleVar_FontSize);
                }
                UIEndRow();

                // hairline between the field and the list
                {
                    UI_Widget* line = PushWidget(WidgetFlags_DrawBackground, Str8Lit(""), 0x504C494E45ull,
                                                 { SizeKind_PercentOfParent, 1.0f }, { SizeKind_Pixels, Maths::Max(1.0f, dpi) });
                    UIClearBorders(line);
                    line->style_vars[StyleVar_Padding]         = Vec4(0.0f);
                    line->style_vars[StyleVar_CornerRadius]    = Vec4(0.0f);
                    line->style_vars[StyleVar_BackgroundColor] = theme.Border;
                }
            }

            Entry* clicked = nullptr;

            if(count == 0)
            {
                UIPushStyle(StyleVar_TextColor, theme.TextDim);
                UILabelCStr("##palettenone", (s_ActiveMode && !s_Needle[0]) ? s_ActiveMode->Placeholder : "  Nothing matches");
                UIPopStyle(StyleVar_TextColor);
            }
            else
            {
                UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 2.0f * dpi));
                UIBeginScrollArea("##palettelist", listHeight, &s_Scroll);
                {
                    UI_Widget* area                            = ui->parents.Back();
                    area->style_vars[StyleVar_BackgroundColor] = Vec4(0.0f);
                    area->style_vars[StyleVar_BorderColor]     = Vec4(0.0f);
                    area->style_vars[StyleVar_Border]          = Vec4(0.0f);
                    area->style_vars[StyleVar_Padding]         = Vec4(2.0f * dpi, 3.0f * dpi, 0.0f, 0.0f);
                }

                // only the rows in view are built
                const i32 first = Maths::Max(0, (i32)floorf(s_Scroll / stride) - 1);
                const i32 last  = Maths::Min(count, first + visible + 3);

                if(first > 0)
                {
                    UI_Widget* top = PushWidget(0, Str8Lit(""), 0x50544F50ull, { SizeKind_PercentOfParent, 1.0f },
                                                { SizeKind_Pixels, (f32)first * stride - 2.0f * dpi });
                    top->style_vars[StyleVar_Padding] = Vec4(0.0f);
                }

                for(i32 i = first; i < last; i++)
                {
                    Result& result = s_Results[i];
                    Entry* entry   = result.entry;
                    const bool sel = i == s_Selected;

                    char id[32];
                    snprintf(id, sizeof(id), "###palrow%d", i);
                    UI_Interaction row = UIBeginSelectableRow(id, sel, rowLogical);
                    {
                        UI_Widget* rowWidget                       = ui->parents.Back();
                        rowWidget->style_vars[StyleVar_CornerRadius] = Vec4(8.0f * dpi, 0.0f, 0.0f, 0.0f);
                        rowWidget->style_vars[StyleVar_Padding]      = Vec4(10.0f * dpi, 0.0f, 0.0f, 0.0f);
                        rowWidget->style_vars[StyleVar_ItemSpacing]  = Vec4(10.0f * dpi, 0.0f, 0.0f, 0.0f);
                        if(sel)
                            UISetBackground(rowWidget, theme.Selection, theme.Selection, theme.Selection);

                        if(row.hovering && mouseMoved)
                            s_Selected = i;

                        UIPushStyle(StyleVar_TextColor, sel ? theme.Accent : theme.TextDim);
                        UILabelCStr("###icon", entry->Icon[0] ? entry->Icon : ICON_MDI_CHEVRON_RIGHT);
                        UIPopStyle(StyleVar_TextColor);

                        UIHighlightMask mask;
                        mask.bits[0] = result.mask[0];
                        mask.bits[1] = result.mask[1];
                        UIPushStyle(StyleVar_TextColor, theme.Text);
                        UILabelHighlighted("###name", Str8C(entry->Name), mask, theme.Accent);
                        UIPopStyle(StyleVar_TextColor);

                        UIFlexSpacer();

                        if(entry->IsOn)
                        {
                            const bool on = entry->IsOn(entry->Arg);
                            UIBadge("###state", on ? Str8Lit("ON") : Str8Lit("OFF"),
                                    on ? theme.Accent : theme.Field, on ? theme.AccentText : theme.TextDim);
                        }
                        if(entry->Hint[0])
                            {
                            char hint[32];
                            UIBadge("###hint", Str8C((char*)UIShortcutText(entry->Hint, hint, sizeof(hint))), theme.Field, theme.TextDim);
                        }

                        if(entry->Category[0])
                        {
                            Vec4 faint = theme.TextDim;
                            faint.w    = 0.75f;
                            UIPushStyle(StyleVar_TextColor, faint);
                            UIPushStyle(StyleVar_FontSize, theme.FontSize * 0.92f * dpi);
                            UILabelCStr("###category", entry->Category);
                            UIPopStyle(StyleVar_FontSize);
                            UIPopStyle(StyleVar_TextColor);
                        }
                    }
                    UIEndSelectableRow();

                    if(row.clicked)
                    {
                        s_Selected = i;
                        clicked    = entry;
                    }
                }

                if(last < count)
                {
                    UI_Widget* bottom = PushWidget(0, Str8Lit(""), 0x50424F54ull, { SizeKind_PercentOfParent, 1.0f },
                                                   { SizeKind_Pixels, (f32)(count - last) * stride - 2.0f * dpi });
                    bottom->style_vars[StyleVar_Padding] = Vec4(0.0f);
                }

                UIEndScrollArea();
                UIPopStyle(StyleVar_ItemSpacing);
            }

            // ---- footer ----
            {
                Vec4 faint = theme.TextDim;
                faint.w    = 0.75f;
                UIPushStyle(StyleVar_TextColor, faint);
                UIPushStyle(StyleVar_FontSize, theme.FontSize * 0.92f * dpi);
                char footer[224];
                int at = snprintf(footer, sizeof(footer), "  %s %s  move     Enter  run     Esc  close     >  commands",
                                  ICON_MDI_ARROW_UP, ICON_MDI_ARROW_DOWN);
                for(u32 i = 0; i < s_ModeCount && at > 0 && at < (int)sizeof(footer) - 40; i++)
                {
                    char lower[24];
                    snprintf(lower, sizeof(lower), "%s", s_Modes[i].Label);
                    for(char* c = lower; *c; c++)
                        *c = Lower(*c);
                    at += snprintf(footer + at, sizeof(footer) - (size_t)at, "     %c  %s", s_Modes[i].Prefix, lower);
                }
                UILabelCStr("##palettefooter", footer);
                UIPopStyle(StyleVar_FontSize);
                UIPopStyle(StyleVar_TextColor);
            }

            UIEndPanel();
            UIPopStyle(StyleVar_FontSize);

            if(clicked)
                RunEntry(clicked);
            else if(clickedOutside)
                Close();
        }
    }
}
