#include "Precompiled.h"
#include "UI.h"
#include "UIInternal.h"
#include "UITheme.h"
#include "Core/OS/Input.h"
#include "Core/Undo.h"
#include "Maths/MathsUtilities.h"
#include "Utilities/StringUtilities.h"
#include "Graphics/RHI/Texture.h"
#include "ImGui/IconsMaterialDesignIcons.h"
#include <filesystem>

// Widgets for tools: popups, menus, property rows, tables, drag and drop.
// Everything here keeps its state in fixed-size statics - nothing is allocated per frame
// beyond the widget labels the UI already copies into its frame arena.

namespace Lumos
{
    namespace
    {
        inline UI_State* UI() { return GetUIState(); }
        inline float Dpi() { return GetUIState()->DPIScale; }
        inline float FontSize() { return GetUIState()->style_variable_lists[StyleVar_FontSize].last->value.x; }

        inline u64 HashCStr(const char* str)
        {
            return StringUtilities::BasicHashFromString(Str8C((char*)str));
        }

        // rect = x, y, width, height
        inline bool PointInRect(const Vec2& p, const Vec4& r)
        {
            return p.x >= r.x && p.x <= r.x + r.z && p.y >= r.y && p.y <= r.y + r.w;
        }

        inline Vec4 WidgetRect(const UI_Widget* w)
        {
            return Vec4(w->position.x, w->position.y, w->size.x, w->size.y);
        }

        inline bool MouseHeld() { return Input::Get().GetMouseHeld(InputCode::MouseKey::ButtonLeft); }

        struct Palette
        {
            Vec4 Text;
            Vec4 Dim;
            Vec4 Panel;
            Vec4 Field;
            Vec4 Hover;
            Vec4 Border;
            Vec4 Accent;
        };

        // Everything here takes its colours from the shared tool theme.
        Palette Colours()
        {
            const UIToolTheme& theme = UIGetToolTheme();
            Palette p;
            p.Text   = theme.Text;
            p.Dim    = theme.TextDim;
            p.Panel  = theme.Popup;
            p.Field  = theme.Field;
            p.Hover  = theme.Hover;
            p.Border = theme.Border;
            p.Accent = theme.Accent;
            return p;
        }

        inline float Rounding() { return UIGetToolTheme().WidgetRounding; }

        // ---- popups ----

        struct PopupEntry
        {
            u64 id;
            UI_State* state;
            Vec2 pos;
            Vec4 rect;      // last frame
            Vec4 ownerRect; // the widget that opens it: clicks there are the owner's to handle
            u64 openedFrame;
            u64 builtFrame;
            f32 contentWidth; // widest row last frame
            f32 buildWidth;   // widest row so far this frame
            i32 depth;
            bool open;
            bool isMenu;
        };

        constexpr u32 kMaxPopups = 8;
        PopupEntry s_Popups[kMaxPopups];
        u32 s_PopupCount = 0;

        i32 s_PopupStack[kMaxPopups];
        i32 s_PopupStackDepth = 0;

        i32 FindPopup(u64 id)
        {
            for(u32 i = 0; i < s_PopupCount; i++)
                if(s_Popups[i].id == id && s_Popups[i].state == UI())
                    return (i32)i;
            return -1;
        }

        PopupEntry* CurrentPopup()
        {
            if(s_PopupStackDepth <= 0)
                return nullptr;
            return &s_Popups[s_PopupStack[s_PopupStackDepth - 1]];
        }

        void OpenPopupEntry(u64 id, const Vec2& pos, const Vec4& ownerRect, bool isMenu)
        {
            i32 index = FindPopup(id);
            if(index < 0)
            {
                if(s_PopupCount >= kMaxPopups)
                    return;
                index = (i32)s_PopupCount++;
            }
            PopupEntry& e  = s_Popups[index];
            e.id           = id;
            e.state        = UI();
            e.pos          = pos;
            e.rect         = Vec4(pos.x, pos.y, 0.0f, 0.0f);
            e.ownerRect    = ownerRect;
            e.openedFrame  = UI()->FrameIndex;
            e.builtFrame   = UI()->FrameIndex;
            e.contentWidth = 0.0f;
            e.buildWidth   = 0.0f;
            e.depth        = CurrentPopup() ? CurrentPopup()->depth + 1 : 0;
            e.open         = true;
            e.isMenu       = isMenu;
        }

        // Closes every popup of this UI deeper than `depth` (-1 = all of them).
        void ClosePopupsDeeperThan(i32 depth)
        {
            for(u32 i = 0; i < s_PopupCount; i++)
                if(s_Popups[i].state == UI() && s_Popups[i].depth > depth)
                    s_Popups[i].open = false;
        }

        bool AnyMenuOpen()
        {
            for(u32 i = 0; i < s_PopupCount; i++)
                if(s_Popups[i].state == UI() && s_Popups[i].open && s_Popups[i].isMenu && s_Popups[i].depth == 0)
                    return true;
            return false;
        }

        bool BeginPopupHash(u64 id)
        {
            const i32 index = FindPopup(id);
            if(index < 0 || !s_Popups[index].open || s_PopupStackDepth >= (i32)kMaxPopups)
                return false;

            PopupEntry& e   = s_Popups[index];
            UI_State* ui    = UI();
            const float dpi = ui->DPIScale;

            ui->parents.PushBack(&ui->root_parent);

            UI_Widget* popup = PushWidget(WidgetFlags_StackVertically | WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | WidgetFlags_TopMost,
                                          Str8Lit(""),
                                          id,
                                          { SizeKind_MaxChild, 1.0f },
                                          { SizeKind_ChildSum, 1.0f });

            // keep it on screen (size is last frame's; the first frame can poke out)
            const Vec2 rootPos  = ui->root_parent.position;
            const Vec2 rootSize = ui->root_parent.size;
            Vec2 pos            = e.pos;
            pos.x               = Maths::Clamp(pos.x, rootPos.x, Maths::Max(rootPos.x, rootPos.x + rootSize.x - popup->size.x));
            pos.y               = Maths::Clamp(pos.y, rootPos.y, Maths::Max(rootPos.y, rootPos.y + rootSize.y - popup->size.y));
            popup->relative_position = pos - rootPos;

            const Palette c                              = Colours();
            popup->style_vars[StyleVar_Padding]          = Vec4(4.0f * dpi, 4.0f * dpi, 0.0f, 0.0f);
            popup->style_vars[StyleVar_ItemSpacing]      = Vec4(0.0f, 1.0f * dpi, 0.0f, 0.0f);
            popup->style_vars[StyleVar_CornerRadius]     = Vec4((Rounding() + 3.0f) * dpi, 0.0f, 0.0f, 0.0f);
            popup->style_vars[StyleVar_Border]           = Vec4(1.0f * dpi, 1.0f * dpi, 0.0f, 0.0f);
            popup->style_vars[StyleVar_BackgroundColor]  = c.Panel;
            popup->style_vars[StyleVar_BorderColor]      = c.Border;
            popup->style_vars[StyleVar_ShadowColor]      = Vec4(0.0f, 0.0f, 0.0f, 0.35f);
            popup->style_vars[StyleVar_ShadowOffset]     = Vec4(0.0f, 3.0f * dpi, 0.0f, 0.0f);
            popup->style_vars[StyleVar_ShadowBlur]       = Vec4(10.0f * dpi, 0.0f, 0.0f, 0.0f);

            e.builtFrame = ui->FrameIndex;
            e.buildWidth = 0.0f;
            e.rect       = WidgetRect(popup);

            s_PopupStack[s_PopupStackDepth++] = index;
            PushParent(popup);
            return true;
        }

        // ---- menus ----
        bool s_InMenuBar = false;

        // ---- number field being typed into ----
        u64 s_NumEditHash        = 0;
        UI_State* s_NumEditState = nullptr;
        char s_NumEditBuffer[32] = { 0 };
        bool s_NumEditHadFocus   = false;

        // ---- property grid ----
        f32 s_PropFraction[8];
        i32 s_PropDepth = 0;

        // ---- tables ----
        struct TableFrame
        {
            UI_Widget* table;
            UI_Widget* row;
            UI_Widget* cell;
            f32* fractions;
            i32 columns;
            i32 column;
            u32 rowIndex;
        };
        TableFrame s_Tables[4];
        i32 s_TableDepth = 0;
        f32 s_TableDragStart[2];

        // ---- drag and drop ----
        struct DragDropState
        {
            bool active;
            bool dropFrame;
            bool consumed;
            UI_State* state;
            u64 type;
            u64 source;
            u32 size;
            u8 data[256];
            char preview[64];
        };
        DragDropState s_Drag = {};

        void ClearTextFocus()
        {
            UI_State* ui            = UI();
            ui->FocusedTextInput    = 0;
            ui->TextInputBuffer     = nullptr;
            ui->TextInputBufferSize = 0;
            ui->TextInputSelStart   = 0;
            ui->TextInputSelEnd     = 0;
        }
    }

    // ---- frame hooks -------------------------------------------------------

    void UIWidgetsBeginBuild()
    {
        UI_State* ui = UI();

        s_PopupStackDepth = 0;
        s_InMenuBar       = false;
        s_PropDepth       = 0;
        s_TableDepth      = 0;

        // drop popups that were closed, or whose owner stopped building them
        for(u32 i = 0; i < s_PopupCount;)
        {
            PopupEntry& e    = s_Popups[i];
            const bool stale = e.state == ui && e.builtFrame + 1 < ui->FrameIndex && e.openedFrame + 1 < ui->FrameIndex;
            if(e.state == ui && (!e.open || stale))
            {
                s_Popups[i] = s_Popups[s_PopupCount - 1];
                s_PopupCount--;
            }
            else
                i++;
        }

        // a press outside closes them; a press inside one closes the ones nested deeper
        const bool pressed = Input::Get().GetMouseClicked(InputCode::MouseKey::ButtonLeft)
            || Input::Get().GetMouseClicked(InputCode::MouseKey::ButtonRight);
        if(pressed && s_PopupCount > 0)
        {
            const Vec2 mouse = UIMousePosition();
            i32 hitDepth     = -1;
            for(u32 i = 0; i < s_PopupCount; i++)
            {
                const PopupEntry& e = s_Popups[i];
                if(e.state == ui && e.open && PointInRect(mouse, e.rect))
                    hitDepth = Maths::Max(hitDepth, e.depth);
            }
            for(u32 i = 0; i < s_PopupCount; i++)
            {
                PopupEntry& e = s_Popups[i];
                if(e.state != ui || !e.open || e.openedFrame >= ui->FrameIndex)
                    continue;
                if(e.depth > hitDepth && !PointInRect(mouse, e.ownerRect))
                    e.open = false;
            }
        }

        if(s_Drag.active && s_Drag.state == ui)
        {
            if(s_Drag.dropFrame)
                s_Drag = {};
            else if(!MouseHeld())
                s_Drag.dropFrame = true;
        }
    }

    void UIWidgetsEndBuild()
    {
        UI_State* ui = UI();
        if(s_Drag.active && s_Drag.state == ui && !s_Drag.dropFrame && s_Drag.preview[0])
        {
            const float dpi = ui->DPIScale;
            ui->parents.PushBack(&ui->root_parent);

            UI_Widget* ghost = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawText | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | WidgetFlags_TopMost,
                                          Str8C(s_Drag.preview),
                                          HashCStr("##uidragghost"),
                                          { SizeKind_TextContent, 1.0f },
                                          { SizeKind_TextContent, 1.0f });

            const Palette c          = Colours();
            ghost->relative_position = UIMousePosition() - ui->root_parent.position + Vec2(12.0f * dpi, 10.0f * dpi);
            UIClearBorders(ghost);
            ghost->style_vars[StyleVar_Padding]         = Vec4(8.0f * dpi, 4.0f * dpi, 0.0f, 0.0f);
            ghost->style_vars[StyleVar_CornerRadius]    = Vec4(5.0f * dpi, 0.0f, 0.0f, 0.0f);
            ghost->style_vars[StyleVar_BackgroundColor] = Vec4(c.Accent.x, c.Accent.y, c.Accent.z, 0.85f);
            ghost->style_vars[StyleVar_TextColor]       = Vec4(1.0f, 1.0f, 1.0f, 1.0f);

            ui->parents.PopBack();
        }
    }

    // ---- containers --------------------------------------------------------

    UI_Interaction UIBeginChild(const char* str, UI_Size width, UI_Size height, u32 flags, bool clip)
    {
        u64 hash;
        HandleUIString(str, &hash);

        UI_Widget* child = PushWidget(flags, Str8Lit(""), hash, width, height);
        if(UI()->style_variable_lists[StyleVar_Padding].count == 0)
            child->style_vars[StyleVar_Padding] = Vec4(0.0f);
        if(UI()->style_variable_lists[StyleVar_Border].count == 0)
            child->style_vars[StyleVar_Border] = Vec4(0.0f);
        child->Clip = clip;

        UI_Interaction interaction = HandleWidgetInteraction(child);
        PushParent(child);
        return interaction;
    }

    void UIEndChild()
    {
        PopParent(GetCurrentParent());
    }

    Vec4 UIGetLastWidgetRect()
    {
        UI_Widget* parent = GetCurrentParent();
        if(!parent || !parent->last)
            return Vec4(0.0f);
        return WidgetRect(parent->last);
    }

    bool UIIsLastWidgetHovered()
    {
        UI_Widget* parent = GetCurrentParent();
        if(!parent || !parent->last || UI()->OverlayBlocksInput)
            return false;
        const UI_Widget* w = parent->last;
        const Vec2 mouse   = UIMousePosition();
        const Vec4& clip   = w->ClipRect;
        return PointInRect(mouse, WidgetRect(w)) && mouse.x >= clip.x && mouse.x <= clip.z && mouse.y >= clip.y && mouse.y <= clip.w;
    }

    // ---- popups ------------------------------------------------------------

    void UIOpenPopup(const char* id)
    {
        const u64 hash  = HashCStr(id);
        const i32 index = FindPopup(hash);
        // the owner was pressed again: that closes it
        if(index >= 0 && s_Popups[index].open)
        {
            s_Popups[index].open = false;
            return;
        }
        const Vec4 owner = UIGetLastWidgetRect();
        OpenPopupEntry(hash, Vec2(owner.x, owner.y + owner.w), owner, false);
    }

    void UIOpenPopupAt(const char* id, const Vec2& position)
    {
        OpenPopupEntry(HashCStr(id), position, Vec4(0.0f), false);
    }

    bool UIIsPopupOpen(const char* id)
    {
        const i32 index = FindPopup(HashCStr(id));
        return index >= 0 && s_Popups[index].open;
    }

    bool UIBeginPopup(const char* id)
    {
        return BeginPopupHash(HashCStr(id));
    }

    void UIEndPopup()
    {
        if(s_PopupStackDepth <= 0)
            return;

        UI_State* ui  = UI();
        PopupEntry& e = s_Popups[s_PopupStack[--s_PopupStackDepth]];

        UI_Widget* popup = GetCurrentParent();
        PopParent(popup);
        ui->parents.PopBack(); // root

        e.rect         = WidgetRect(popup);
        e.contentWidth = e.buildWidth;

        if(PointInRect(UIMousePosition(), e.rect))
            ui->OverlayBlocksInput = true;
    }

    void UICloseCurrentPopup()
    {
        if(PopupEntry* e = CurrentPopup())
            e->open = false;
    }

    void UICloseAllPopups()
    {
        ClosePopupsDeeperThan(-1);
    }

    // ---- menus -------------------------------------------------------------

    bool UIBeginMenuBar(const char* str)
    {
        const float dpi = Dpi();
        u64 hash;
        HandleUIString(str, &hash);

        UI_Widget* bar = PushWidget(WidgetFlags_StackHorizontally | WidgetFlags_DrawBackground | WidgetFlags_CentreChildrenY,
                                    Str8Lit(""),
                                    hash,
                                    { SizeKind_PercentOfParent, 1.0f },
                                    { SizeKind_MaxChild, 1.0f });

        const bool dark                           = UIThemeIsDark();
        bar->style_vars[StyleVar_Padding]         = Vec4(6.0f * dpi, 2.0f * dpi, 0.0f, 0.0f);
        bar->style_vars[StyleVar_ItemSpacing]     = Vec4(2.0f * dpi, 0.0f, 0.0f, 0.0f);
        bar->style_vars[StyleVar_Border]          = Vec4(0.0f);
        bar->style_vars[StyleVar_CornerRadius]    = Vec4(0.0f);
        bar->style_vars[StyleVar_BackgroundColor] = dark ? Vec4(0.10f, 0.105f, 0.12f, 1.0f) : Vec4(0.90f, 0.90f, 0.92f, 1.0f);

        PushParent(bar);
        s_InMenuBar = true;
        return true;
    }

    void UIEndMenuBar()
    {
        PopParent(GetCurrentParent());
        s_InMenuBar = false;
    }

    const char* UIShortcutText(const char* hint, char* out, u32 outSize)
    {
#ifdef LUMOS_PLATFORM_MACOS
        // Cmd+Space belongs to the system
        if(hint && strncmp(hint, "Ctrl ", 5) == 0 && strcmp(hint, "Ctrl Space") != 0)
        {
            snprintf(out, outSize, "Cmd %s", hint + 5);
            return out;
        }
#endif
        snprintf(out, outSize, "%s", hint ? hint : "");
        return out;
    }

    // One row of a menu popup. `submenu` draws a chevron instead of a shortcut.
    static Vec4 DisabledText(const Palette& c)
    {
        // text alpha is not a reliable fade: mixed towards the menu's own colour instead
        const f32 t = 0.68f;
        return Vec4(c.Text.x + (c.Panel.x - c.Text.x) * t, c.Text.y + (c.Panel.y - c.Text.y) * t, c.Text.z + (c.Panel.z - c.Text.z) * t, 1.0f);
    }

    static UI_Interaction MenuRow(const char* str, const char* shortcutIn, bool checked, bool hasCheck, bool enabled, bool submenu, UI_Widget** outRow)
    {
        char shortcutText[32];
        const char* shortcut = shortcutIn ? UIShortcutText(shortcutIn, shortcutText, sizeof(shortcutText)) : nullptr;

        UI_State* ui    = UI();
        const float dpi = ui->DPIScale;
        const float fs  = FontSize();
        const Palette c = Colours();

        u64 hash;
        String8 text = HandleUIString(str, &hash);

        const float checkWidth = 20.0f * dpi;
        const float padX       = 8.0f * dpi;
        const float tailWidth  = submenu ? 18.0f * dpi : (shortcut && shortcut[0] ? GetStringSize(Str8C((char*)shortcut), fs).x + 28.0f * dpi : 0.0f);
        const float ownWidth   = checkWidth + GetStringSize(text, fs).x + tailWidth + padX * 2.0f + 8.0f * dpi;

        float rowWidth = ownWidth;
        if(PopupEntry* e = CurrentPopup())
        {
            e->buildWidth = Maths::Max(e->buildWidth, ownWidth);
            rowWidth      = Maths::Max(rowWidth, e->contentWidth);
        }

        UI_Widget* row = PushWidget((enabled ? WidgetFlags_Clickable : 0) | WidgetFlags_DrawBackground | WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY,
                                    Str8Lit(""),
                                    hash,
                                    { SizeKind_Pixels, rowWidth },
                                    { SizeKind_MaxChild, 1.0f });
        UIClearBorders(row);
        row->style_vars[StyleVar_Padding]      = Vec4(padX, 3.0f * dpi, 0.0f, 0.0f);
        row->style_vars[StyleVar_ItemSpacing]  = Vec4(0.0f);
        row->style_vars[StyleVar_CornerRadius] = Vec4(Rounding() * dpi, 0.0f, 0.0f, 0.0f);
        UISetBackground(row, Vec4(0.0f), Vec4(c.Accent.x, c.Accent.y, c.Accent.z, 0.55f), Vec4(c.Accent.x, c.Accent.y, c.Accent.z, 0.75f));

        UI_Interaction interaction = HandleWidgetInteraction(row);

        const Vec4 textColour = enabled ? c.Text : DisabledText(c);

        PushParent(row);
        {
            UI_Widget* check = PushWidget(WidgetFlags_DrawText,
                                          (hasCheck && checked) ? Str8Lit(ICON_MDI_CHECK) : Str8Lit(""),
                                          UIHashCombine(hash, 0x43484Bull),
                                          { SizeKind_Pixels, checkWidth },
                                          { SizeKind_TextContent, 1.0f });
            check->style_vars[StyleVar_Padding] = Vec4(0.0f, 2.0f * dpi, 0.0f, 0.0f);
            UISetTextColour(check, textColour);

            UI_Widget* label = PushWidget(WidgetFlags_DrawText, text, UIHashCombine(hash, 0x4C424Cull),
                                          { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
            label->style_vars[StyleVar_Padding] = Vec4(0.0f, 2.0f * dpi, 0.0f, 0.0f);
            UISetTextColour(label, textColour);

            UIFlexSpacer();

            if(submenu || (shortcut && shortcut[0]))
            {
                UI_Widget* tail = PushWidget(WidgetFlags_DrawText,
                                             submenu ? Str8Lit(ICON_MDI_CHEVRON_RIGHT) : Str8C((char*)shortcut),
                                             UIHashCombine(hash, 0x54414Cull),
                                             { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
                tail->style_vars[StyleVar_Padding] = Vec4(0.0f, 2.0f * dpi, 0.0f, 0.0f);
                UISetTextColour(tail, enabled ? c.Dim : DisabledText(c));
            }
        }
        PopParent(row);

        if(outRow)
            *outRow = row;
        return interaction;
    }

    bool UIBeginMenu(const char* str, bool enabled)
    {
        UI_State* ui    = UI();
        const float dpi = ui->DPIScale;
        const Palette c = Colours();

        // a title on the bar
        if(s_InMenuBar && s_PopupStackDepth == 0)
        {
            u64 hash;
            String8 text = HandleUIString(str, &hash);

            UI_Widget* title = PushWidget((enabled ? WidgetFlags_Clickable : 0) | WidgetFlags_DrawText | WidgetFlags_DrawBackground,
                                          text, hash,
                                          { SizeKind_TextContent, 1.0f },
                                          { SizeKind_TextContent, 1.0f });
            UIClearBorders(title);
            title->style_vars[StyleVar_Padding]      = Vec4(8.0f * dpi, 3.0f * dpi, 0.0f, 0.0f);
            title->style_vars[StyleVar_CornerRadius] = Vec4(Rounding() * dpi, 0.0f, 0.0f, 0.0f);

            const u64 id     = title->hash;
            const i32 index  = FindPopup(id);
            const bool open  = index >= 0 && s_Popups[index].open;
            const Vec4 rect  = WidgetRect(title);
            const Vec2 below = Vec2(rect.x, rect.y + rect.w + 2.0f * dpi);

            UISetBackground(title, open ? c.Hover : Vec4(0.0f), c.Hover, c.Hover);
            UISetTextColour(title, enabled ? c.Text : DisabledText(c));

            UI_Interaction interaction = HandleWidgetInteraction(title);
            if(interaction.clicked)
            {
                UICloseAllPopups();
                if(!open)
                    OpenPopupEntry(id, below, rect, true);
            }
            else if(interaction.hovering && !open && AnyMenuOpen())
            {
                // sliding along the bar with a menu open swaps to the one under the pointer
                UICloseAllPopups();
                OpenPopupEntry(id, below, rect, true);
            }
            else if(open)
            {
                s_Popups[index].ownerRect = rect;
                s_Popups[index].pos       = below;
            }

            return BeginPopupHash(id);
        }

        // a submenu row inside another menu
        if(s_PopupStackDepth <= 0)
            return false;

        const i32 parentDepth = CurrentPopup()->depth;

        UI_Widget* row             = nullptr;
        UI_Interaction interaction = MenuRow(str, nullptr, false, false, enabled, true, &row);
        const u64 id               = row->hash;
        const i32 index            = FindPopup(id);
        const bool open            = index >= 0 && s_Popups[index].open;
        const Vec4 rect            = WidgetRect(row);
        const Vec2 beside          = Vec2(rect.x + rect.z + 2.0f * dpi, rect.y - 4.0f * dpi);

        if((interaction.hovering || interaction.clicked) && !open && enabled)
        {
            ClosePopupsDeeperThan(parentDepth);
            OpenPopupEntry(id, beside, rect, true);
        }
        else if(open)
        {
            s_Popups[index].ownerRect = rect;
            s_Popups[index].pos       = beside;
            // keep the row lit while its menu is up
            row->style_vars[StyleVar_BackgroundColor] = Vec4(c.Accent.x, c.Accent.y, c.Accent.z, 0.35f);
        }

        return BeginPopupHash(id);
    }

    void UIEndMenu()
    {
        UIEndPopup();
    }

    bool UIMenuItem(const char* str, const char* shortcut, bool* checked, bool enabled)
    {
        const i32 depth = CurrentPopup() ? CurrentPopup()->depth : -1;

        UI_Interaction interaction = MenuRow(str, shortcut, checked ? *checked : false, checked != nullptr, enabled, false, nullptr);

        // resting on a plain item folds any submenu that was open beside it
        if(interaction.hovering && depth >= 0)
            ClosePopupsDeeperThan(depth);

        if(interaction.clicked && enabled)
        {
            if(checked)
                *checked = !*checked;
            UICloseAllPopups();
            return true;
        }
        return false;
    }

    void UIMenuSeparator()
    {
        const float dpi = Dpi();
        PopupEntry* e   = CurrentPopup();
        const float w   = e ? Maths::Max(e->contentWidth, 10.0f * dpi) : 10.0f * dpi;

        UISpacer(2.0f);
        UI_Widget* parent = GetCurrentParent();
        UI_Widget* line   = PushWidget(WidgetFlags_DrawBackground, Str8Lit(""),
                                       UIHashCombine(0x4D53455021ull, UI()->WidgetIdCounter++),
                                       { SizeKind_Pixels, w }, { SizeKind_Pixels, Maths::Max(1.0f, dpi) });
        (void)parent;
        UIClearBorders(line);
        line->style_vars[StyleVar_Padding]         = Vec4(0.0f);
        line->style_vars[StyleVar_CornerRadius]    = Vec4(0.0f);
        line->style_vars[StyleVar_BackgroundColor] = Colours().Border;
        UISpacer(2.0f);
    }

    // ---- buttons / images --------------------------------------------------

    UI_Interaction UIIconButton(const char* str, float size, bool selected)
    {
        const float dpi = Dpi();
        const Palette c = Colours();

        u64 hash;
        String8 text = HandleUIString(str, &hash);

        UI_Widget* button = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawText | WidgetFlags_DrawBackground,
                                       text, hash,
                                       { SizeKind_Pixels, size * dpi },
                                       { SizeKind_Pixels, size * dpi });
        UIClearBorders(button);
        button->TextAlignment                      = UI_Text_Alignment_Center_X | UI_Text_Alignment_Center_Y;
        button->style_vars[StyleVar_Padding]       = Vec4(0.0f);
        button->style_vars[StyleVar_CornerRadius]  = Vec4(Rounding() * dpi, 0.0f, 0.0f, 0.0f);
        const Vec4 on                              = Vec4(c.Accent.x, c.Accent.y, c.Accent.z, 0.45f);
        UISetBackground(button, selected ? on : Vec4(0.0f), selected ? on : c.Hover, on);
        if(UI()->style_variable_lists[StyleVar_TextColor].count == 0)
            UISetTextColour(button, selected ? Vec4(1.0f, 1.0f, 1.0f, 1.0f) : c.Text);

        return HandleWidgetInteraction(button);
    }

    static UI_Widget* ImageWidget(const char* str, Graphics::Texture2D* texture, const Vec2& size, bool flipV, bool clickable)
    {
        u64 hash;
        HandleUIString(str, &hash);

        UI_Widget* image = PushWidget(WidgetFlags_DrawBackground | (clickable ? WidgetFlags_Clickable : 0),
                                      Str8Lit(""), hash,
                                      { SizeKind_Pixels, size.x },
                                      { SizeKind_Pixels, size.y });
        UIClearBorders(image);
        image->texture                          = texture;
        image->FlipV                            = flipV;
        image->style_vars[StyleVar_Padding]      = Vec4(0.0f);
        image->style_vars[StyleVar_CornerRadius] = Vec4(0.0f);
        // the fill colour tints the texture: keep it white in every state
        const Vec4 white = texture ? Vec4(1.0f, 1.0f, 1.0f, 1.0f) : Vec4(0.0f, 0.0f, 0.0f, 1.0f);
        UISetBackground(image, white, white, white);
        return image;
    }

    UI_Interaction UIImageButton(const char* str, Graphics::Texture2D* texture, const Vec2& size, bool flipV)
    {
        UI_Widget* image = ImageWidget(str, texture, size, flipV, true);
        const Vec4 hot   = Vec4(0.88f, 0.92f, 1.0f, 1.0f);
        image->style_vars[StyleVar_HotBackgroundColor]    = hot;
        image->style_vars[StyleVar_ActiveBackgroundColor] = hot;
        image->style_vars[StyleVar_CornerRadius]          = Vec4(3.0f * Dpi(), 0.0f, 0.0f, 0.0f);
        return HandleWidgetInteraction(image);
    }

    UI_Interaction UIImageSized(const char* str, Graphics::Texture2D* texture, const Vec2& size, bool flipV)
    {
        return HandleWidgetInteraction(ImageWidget(str, texture, size, flipV, true));
    }

    // ---- picture tooltip ---------------------------------------------------

    void UIImageTooltip(const UI_Interaction& hovered, Graphics::Texture2D* texture, const char* caption, bool flipV)
    {
        static u64 s_Over   = 0;
        static f32 s_Since  = 0.0f;
        UI_State* ui        = UI();
        if(!hovered.widget || !texture)
            return;
        const u64 id = hovered.widget->hash;
        if(!hovered.hovering)
        {
            if(s_Over == id)
                s_Over = 0;
            return;
        }
        if(s_Over != id)
        {
            s_Over  = id;
            s_Since = ui->CurrentTime;
        }
        if(ui->CurrentTime - s_Since < 0.3f || texture->GetWidth() == 0 || texture->GetHeight() == 0)
            return;

        const float dpi    = ui->DPIScale;
        const Palette c    = Colours();
        const float box    = floorf(260.0f * dpi);
        const float aspect = (float)texture->GetWidth() / (float)texture->GetHeight();
        const Vec2 picture = aspect >= 1.0f ? Vec2(box, floorf(box / aspect)) : Vec2(floorf(box * aspect), box);
        const float pad    = floorf(8.0f * dpi);
        const float textH  = caption && caption[0] ? floorf(34.0f * dpi) : 0.0f;
        const Vec2 size    = Vec2(picture.x + pad * 2.0f, picture.y + pad * 2.0f + textH);

        // beside the pointer, on whichever side has the room
        const Vec2 screen = ui->root_parent.size;
        const Vec2 mouse  = UIMousePosition();
        Vec2 position     = mouse + Vec2(18.0f * dpi, 14.0f * dpi);
        if(position.x + size.x > screen.x - 4.0f * dpi)
            position.x = mouse.x - size.x - 14.0f * dpi;
        if(position.y + size.y > screen.y - 4.0f * dpi)
            position.y = screen.y - size.y - 4.0f * dpi;
        position.x = Maths::Max(position.x, 4.0f * dpi);
        position.y = Maths::Max(position.y, 4.0f * dpi);

        ui->parents.PushBack(&ui->root_parent);
        UI_Widget* panel = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | WidgetFlags_TopMost | WidgetFlags_StackVertically,
                                      Str8Lit(""), HashCStr("##uiimagetooltip"), { SizeKind_Pixels, size.x }, { SizeKind_Pixels, size.y });
        panel->relative_position                 = Vec2(floorf(position.x), floorf(position.y)) - ui->root_parent.position;
        panel->style_vars[StyleVar_Padding]      = Vec4(pad, pad, 0.0f, 0.0f);
        panel->style_vars[StyleVar_ItemSpacing]  = Vec4(0.0f, 4.0f * dpi, 0.0f, 0.0f);
        panel->style_vars[StyleVar_CornerRadius] = Vec4(8.0f * dpi, 0.0f, 0.0f, 0.0f);
        panel->style_vars[StyleVar_Border]       = Vec4(1.0f, 1.0f, 0.0f, 0.0f);
        const Vec4 fill   = Vec4(0.085f, 0.085f, 0.10f, 1.0f);
        const Vec4 border = Vec4(0.26f, 0.26f, 0.30f, 1.0f);
        UISetBackground(panel, fill, fill, fill);
        panel->style_vars[StyleVar_BorderColor]       = border;
        panel->style_vars[StyleVar_HotBorderColor]    = border;
        panel->style_vars[StyleVar_ActiveBorderColor] = border;
        PushParent(panel);
        {
            UI_Widget* image = PushWidget(WidgetFlags_DrawBackground, Str8Lit(""), 0x494D47ull, { SizeKind_Pixels, picture.x }, { SizeKind_Pixels, picture.y });
            UIClearBorders(image);
            image->texture                          = texture;
            image->FlipV                            = flipV;
            image->style_vars[StyleVar_Padding]      = Vec4(0.0f);
            image->style_vars[StyleVar_CornerRadius] = Vec4(3.0f * dpi, 0.0f, 0.0f, 0.0f);
            UISetBackground(image, Vec4(1.0f), Vec4(1.0f), Vec4(1.0f));
            if(textH > 0.0f)
            {
                UI_Widget* text = PushWidget(WidgetFlags_DrawText, Str8C((char*)caption), 0x434150ull, { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
                text->style_vars[StyleVar_Padding]  = Vec4(0.0f);
                text->style_vars[StyleVar_FontSize] = Vec4(floorf(12.0f * dpi), 0.0f, 0.0f, 0.0f);
                UISetTextColour(text, Vec4(0.80f, 0.80f, 0.83f, 1.0f));
            }
        }
        PopParent(panel);
        ui->parents.PopBack();
        (void)c;
    }

    // ---- slider ------------------------------------------------------------

    bool UIToolSlider(const char* str, float* value, float minValue, float maxValue, float width)
    {
        const float dpi = Dpi();
        const Palette c = Colours();
        u64 hash;
        HandleUIString(str, &hash);

        const float height = floorf(18.0f * dpi);
        const float knob   = floorf(12.0f * dpi);
        const float trackH = Maths::Max(2.0f, floorf(4.0f * dpi));

        // the whole strip takes the press; the track, fill and knob are drawn inside it
        UI_Widget* strip = PushWidget(WidgetFlags_Clickable | WidgetFlags_Draggable, Str8Lit(""), hash,
                                      width > 0.0f ? UI_Size { SizeKind_Pixels, floorf(width * dpi) } : UI_Size { SizeKind_Grow, 1.0f }, { SizeKind_Pixels, height });
        // filling the row: as wide as layout made it last frame
        const float w = width > 0.0f ? floorf(width * dpi) : Maths::Max(strip->size.x, knob * 2.0f);
        UIClearBorders(strip);
        strip->style_vars[StyleVar_Padding] = Vec4(0.0f);
        strip->drag_constraint_x            = true;
        strip->drag_constraint_y            = true;
        UI_Interaction interaction          = HandleWidgetInteraction(strip);

        const float before = *value;
        const float span   = Maths::Max(1.0f, w - knob);
        if(interaction.pressed || strip->dragging)
        {
            const float t = Maths::Clamp((UIMousePosition().x - strip->position.x - knob * 0.5f) / span, 0.0f, 1.0f);
            *value        = minValue + (maxValue - minValue) * t;
        }
        const float t = maxValue > minValue ? Maths::Clamp((*value - minValue) / (maxValue - minValue), 0.0f, 1.0f) : 0.0f;

        auto part = [&](u64 tag, float x, float y, float pw, float ph, const Vec4& colour, float rounding)
        {
            UI_Widget* quad = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y, Str8Lit(""), tag,
                                         { SizeKind_Pixels, pw }, { SizeKind_Pixels, ph });
            quad->relative_position                 = Vec2(floorf(x), floorf(y));
            quad->style_vars[StyleVar_Padding]      = Vec4(0.0f);
            quad->style_vars[StyleVar_CornerRadius] = Vec4(rounding, 0.0f, 0.0f, 0.0f);
            UIClearBorders(quad);
            UISetBackground(quad, colour, colour, colour);
        };

        const bool lit = interaction.hovering || strip->dragging;
        PushParent(strip);
        part(0x54524Bull, knob * 0.5f, (height - trackH) * 0.5f, span, trackH, c.Field, trackH * 0.5f);
        part(0x46494Cull, knob * 0.5f, (height - trackH) * 0.5f, span * t, trackH, c.Accent, trackH * 0.5f);
        part(0x4B4E42ull, span * t, (height - knob) * 0.5f, knob, knob, lit ? c.Accent : c.Text, knob * 0.5f);
        PopParent(strip);

        return *value != before;
    }

    bool UIPropertySlider(const char* label, float* value, float minValue, float maxValue, const char* fmt)
    {
        const float before = *value;
        UIBeginPropertyRow(label);
        UIToolSlider("s", value, minValue, maxValue, 0.0f);
        const float range = maxValue - minValue;
        UIDragFloatField("v", value, range > 0.0f ? range * 0.004f : 0.01f, minValue, maxValue, fmt, 52.0f);
        UIEndPropertyRow();
        return *value != before;
    }

    void UIPropertyHeading(const char* text)
    {
        const float dpi = Dpi();
        const Palette c = Colours();
        u64 hash;
        String8 label = HandleUIString(text, &hash);

        UI_Widget* row = PushWidget(WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY, Str8Lit(""), UIHashCombine(hash, 0x48454144ull),
                                    { SizeKind_PercentOfParent, 1.0f }, { SizeKind_Pixels, floorf(24.0f * dpi) });
        row->style_vars[StyleVar_Padding]     = Vec4(2.0f * dpi, 0.0f, 0.0f, 0.0f);
        row->style_vars[StyleVar_ItemSpacing] = Vec4(8.0f * dpi, 0.0f, 0.0f, 0.0f);
        PushParent(row);
        UI_Widget* title = PushWidget(WidgetFlags_DrawText, label, hash, { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
        title->style_vars[StyleVar_Padding] = Vec4(0.0f, 2.0f * dpi, 0.0f, 0.0f);
        UISetTextColour(title, c.Accent);
        UI_Widget* line = PushWidget(WidgetFlags_DrawBackground, Str8Lit(""), 0x484C4E45ull, { SizeKind_Grow, 1.0f }, { SizeKind_Pixels, Maths::Max(1.0f, floorf(dpi)) });
        UIClearBorders(line);
        line->style_vars[StyleVar_Padding]      = Vec4(0.0f);
        line->style_vars[StyleVar_CornerRadius] = Vec4(0.0f);
        UISetBackground(line, c.Border, c.Border, c.Border);
        PopParent(row);
    }

    bool UIPropertyFold(const char* str, bool defaultOpen, Graphics::Texture2D* thumbnail)
    {
        const float dpi = Dpi();
        const Palette c = Colours();
        u64 hash;
        String8 label = HandleUIString(str, &hash);

        UI_Widget* row = PushWidget(WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY | WidgetFlags_Clickable | WidgetFlags_DrawBackground, Str8Lit(""),
                                    UIHashCombine(hash, 0x464F4C44ull), { SizeKind_PercentOfParent, 1.0f }, { SizeKind_Pixels, floorf(24.0f * dpi) });
        UIClearBorders(row);
        row->style_vars[StyleVar_Padding]      = Vec4(2.0f * dpi, 0.0f, 0.0f, 0.0f);
        row->style_vars[StyleVar_ItemSpacing]  = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);
        row->style_vars[StyleVar_CornerRadius] = Vec4(Rounding() * dpi, 0.0f, 0.0f, 0.0f);
        UISetBackground(row, Vec4(0.0f), c.Hover, c.Hover);
        if(row->FirstFrame)
            row->StateValue = defaultOpen ? 1u : 0u;
        if(HandleWidgetInteraction(row).clicked)
            row->StateValue = row->StateValue ? 0u : 1u;
        const bool open = row->StateValue != 0;

        PushParent(row);
        UI_Widget* arrow = PushWidget(WidgetFlags_DrawText, open ? Str8Lit(ICON_MDI_CHEVRON_DOWN) : Str8Lit(ICON_MDI_CHEVRON_RIGHT), 0x464152ull,
                                      { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
        arrow->style_vars[StyleVar_Padding] = Vec4(0.0f, 2.0f * dpi, 0.0f, 0.0f);
        UISetTextColour(arrow, c.Dim);
        if(thumbnail)
        {
            UI_Widget* image = PushWidget(WidgetFlags_DrawBackground, Str8Lit(""), 0x46544842ull, { SizeKind_Pixels, floorf(18.0f * dpi) }, { SizeKind_Pixels, floorf(18.0f * dpi) });
            UIClearBorders(image);
            image->texture                          = thumbnail;
            image->style_vars[StyleVar_Padding]      = Vec4(0.0f);
            image->style_vars[StyleVar_CornerRadius] = Vec4(3.0f * dpi, 0.0f, 0.0f, 0.0f);
            UISetBackground(image, Vec4(1.0f), Vec4(1.0f), Vec4(1.0f));
        }
        UI_Widget* title = PushWidget(WidgetFlags_DrawText, label, hash, { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
        title->style_vars[StyleVar_Padding] = Vec4(0.0f, 2.0f * dpi, 0.0f, 0.0f);
        UISetTextColour(title, c.Text);
        PopParent(row);
        return open;
    }

    // ---- text fields -------------------------------------------------------

    UI_Interaction UITextField(const char* str, char* buffer, u32 bufferSize, const char* placeholder, float width)
    {
        const float dpi = Dpi();
        const Palette c = Colours();

        u64 hash;
        HandleUIString(str, &hash);
        const bool focused = UI()->FocusedTextInput == UIHashCombine(GetCurrentParent()->hash, hash);

        // caret, selection and focus line take the tool accent whatever the context's own is
        const Vec4 contextAccent   = UI()->AccentColour;
        UI()->AccentColour         = c.Accent;
        UI_Interaction interaction = UISearchField(str, buffer, bufferSize, placeholder ? placeholder : "", width);
        UI()->AccentColour         = contextAccent;
        UI_Widget* field           = interaction.widget;
        if(!field)
            return interaction;

        field->style_vars[StyleVar_Padding]      = Vec4(6.0f * dpi, 3.0f * dpi, 0.0f, 0.0f);
        field->style_vars[StyleVar_CornerRadius] = Vec4(Rounding() * dpi, 0.0f, 0.0f, 0.0f);
        UISetBackground(field, c.Field, focused ? c.Field : c.Hover, focused ? c.Field : c.Hover);
        UISetTextColour(field, buffer[0] ? c.Text : c.Dim);
        return interaction;
    }

    // ---- number fields -----------------------------------------------------

    static bool DragField(const char* str, float* value, float speed, float minValue, float maxValue,
                          const char* fmt, UI_Size sizeX, const Vec4* tint, bool integer)
    {
        UI_State* ui    = UI();
        const float dpi = ui->DPIScale;
        const Palette c = Colours();

        u64 hash;
        HandleUIString(str, &hash);
        const u64 fullHash = UIHashCombine(GetCurrentParent()->hash, hash);
        const float before = *value;

        auto clamped = [&](float v)
        {
            if(integer)
                v = floorf(v + 0.5f);
            if(maxValue > minValue)
                v = Maths::Clamp(v, minValue, maxValue);
            return v;
        };

        // typing a value in
        if(s_NumEditHash == fullHash && s_NumEditState == ui)
        {
            char id[48];
            snprintf(id, sizeof(id), "###numedit%llx", (unsigned long long)hash);
            u64 editHash;
            HandleUIString(id, &editHash);
            const u64 editFieldHash = UIHashCombine(GetCurrentParent()->hash, editHash);

            if(!s_NumEditHadFocus)
                UIFocusNextTextInput();

            const float width          = sizeX.kind == SizeKind_Pixels ? sizeX.value / dpi : 0.0f;
            UI_Interaction interaction = UITextField(id, s_NumEditBuffer, sizeof(s_NumEditBuffer), "", width);
            if(interaction.widget)
            {
                // same box and centred text as the drag field it stands in for
                interaction.widget->style_vars[StyleVar_Padding] = Vec4(4.0f * dpi, 3.0f * dpi, 0.0f, 0.0f);
                interaction.widget->TextAlignment               |= UI_Text_Alignment_Center_X;
                if(tint)
                    UISetTextColour(interaction.widget, *tint);
                if(sizeX.kind != SizeKind_Pixels)
                    interaction.widget->semantic_size[UIAxis_X] = sizeX;
            }

            const bool focused = ui->FocusedTextInput == editFieldHash;
            const bool cancel  = Input::Get().GetKeyPressedRaw(InputCode::Key::Escape);
            const bool lost    = s_NumEditHadFocus && !focused; // Enter drops focus too
            if(focused)
                s_NumEditHadFocus = true;

            if(lost || cancel)
            {
                if(!cancel && s_NumEditBuffer[0])
                    *value = clamped((float)strtod(s_NumEditBuffer, nullptr));
                if(focused)
                    ClearTextFocus();
                s_NumEditHash = 0;
            }
            return *value != before;
        }

        char buffer[48];
        if(integer)
            snprintf(buffer, sizeof(buffer), "%d", (int)floorf(*value + 0.5f));
        else
            snprintf(buffer, sizeof(buffer), fmt ? fmt : "%.3f", *value);

        UI_Widget* field = PushWidget(WidgetFlags_Clickable | WidgetFlags_Draggable | WidgetFlags_DrawBackground | WidgetFlags_DrawText,
                                      Str8C(buffer), hash, sizeX, { SizeKind_TextContent, 1.0f });
        UIClearBorders(field);
        field->TextAlignment                     = UI_Text_Alignment_Center_X;
        field->drag_constraint_y                 = true;
        field->style_vars[StyleVar_Padding]      = Vec4(4.0f * dpi, 3.0f * dpi, 0.0f, 0.0f);
        field->style_vars[StyleVar_CornerRadius] = Vec4(Rounding() * dpi, 0.0f, 0.0f, 0.0f);
        UISetBackground(field, c.Field, c.Hover, c.Hover);
        UISetTextColour(field, tint ? *tint : c.Text);

        UI_Interaction interaction = HandleWidgetInteraction(field);

        // written on with a pencil: the same as a double click, the text field takes it from there
        UIScribbleRegister(field, fullHash, true);
        if(interaction.double_clicked || UIScribbleTake(fullHash))
        {
            s_NumEditHash     = fullHash;
            s_NumEditState    = ui;
            s_NumEditHadFocus = false;
            if(integer)
                snprintf(s_NumEditBuffer, sizeof(s_NumEditBuffer), "%d", (int)floorf(*value + 0.5f));
            else
                snprintf(s_NumEditBuffer, sizeof(s_NumEditBuffer), "%g", *value);
            return false;
        }

        if(interaction.pressed)
            memcpy(&field->StateValue, value, sizeof(float));

        if(field->dragging)
        {
            float start = 0.0f;
            memcpy(&start, &field->StateValue, sizeof(float));
            *value = clamped(start + (interaction.drag_delta.x / dpi) * speed);
        }

        return *value != before;
    }

    bool UIDragFloatField(const char* str, float* value, float speed, float minValue, float maxValue,
                          const char* fmt, float width, const Vec4* tint)
    {
        const UI_Size sizeX = width > 0.0f ? UI_Size { SizeKind_Pixels, width * Dpi() } : UI_Size { SizeKind_Grow, 1.0f };
        return DragField(str, value, speed, minValue, maxValue, fmt, sizeX, tint, false);
    }

    bool UIDragIntField(const char* str, int* value, float speed, int minValue, int maxValue, float width)
    {
        const UI_Size sizeX = width > 0.0f ? UI_Size { SizeKind_Pixels, width * Dpi() } : UI_Size { SizeKind_Grow, 1.0f };
        float v             = (float)*value;
        const bool changed  = DragField(str, &v, speed, (float)minValue, (float)maxValue, "%.0f", sizeX, nullptr, true);
        if(changed)
            *value = (int)floorf(v + 0.5f);
        return changed;
    }

    // ---- property grid -----------------------------------------------------

    void UIBeginPropertyGrid(const char* str, float labelFraction)
    {
        const float dpi = Dpi();
        u64 hash;
        HandleUIString(str, &hash);

        UI_Widget* grid = PushWidget(WidgetFlags_StackVertically, Str8Lit(""), hash,
                                     { SizeKind_PercentOfParent, 1.0f },
                                     { SizeKind_ChildSum, 1.0f });
        grid->style_vars[StyleVar_Padding]     = Vec4(0.0f);
        grid->style_vars[StyleVar_ItemSpacing] = Vec4(0.0f, 2.0f * dpi, 0.0f, 0.0f);
        PushParent(grid);

        if(s_PropDepth < 8)
            s_PropFraction[s_PropDepth] = labelFraction;
        s_PropDepth++;
    }

    void UIEndPropertyGrid()
    {
        PopParent(GetCurrentParent());
        if(s_PropDepth > 0)
            s_PropDepth--;
    }

    void UIBeginPropertyRow(const char* label)
    {
        const float dpi      = Dpi();
        const Palette c      = Colours();
        const float fraction = (s_PropDepth > 0 && s_PropDepth <= 8) ? s_PropFraction[s_PropDepth - 1] : 0.38f;

        u64 hash;
        String8 text = HandleUIString(label, &hash);

        UI_Widget* row = PushWidget(WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY, Str8Lit(""),
                                    UIHashCombine(hash, 0x50524F57ull),
                                    { SizeKind_PercentOfParent, 1.0f },
                                    { SizeKind_MaxChild, 1.0f });
        row->style_vars[StyleVar_Padding]     = Vec4(0.0f, 1.0f * dpi, 0.0f, 0.0f);
        row->style_vars[StyleVar_ItemSpacing] = Vec4(6.0f * dpi, 0.0f, 0.0f, 0.0f);
        PushParent(row);

        // the label sits in a clipped cell so a long one can't push the editors about
        UI_Widget* cell = PushWidget(0, Str8Lit(""), UIHashCombine(hash, 0x5043454Cull),
                                     { SizeKind_PercentOfParent, fraction },
                                     { SizeKind_MaxChild, 1.0f });
        cell->style_vars[StyleVar_Padding] = Vec4(0.0f);
        cell->Clip                         = true;
        PushParent(cell);
        {
            UI_Widget* labelWidget = PushWidget(WidgetFlags_DrawText, text, hash,
                                                { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
            labelWidget->style_vars[StyleVar_Padding] = Vec4(2.0f * dpi, 2.0f * dpi, 0.0f, 0.0f);
            UISetTextColour(labelWidget, c.Dim);
        }
        PopParent(cell);

        UI_Widget* value = PushWidget(WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY, Str8Lit(""),
                                      UIHashCombine(hash, 0x5056414Cull),
                                      { SizeKind_Grow, 1.0f },
                                      { SizeKind_MaxChild, 1.0f });
        value->style_vars[StyleVar_Padding]     = Vec4(0.0f);
        value->style_vars[StyleVar_ItemSpacing] = Vec4(3.0f * dpi, 0.0f, 0.0f, 0.0f);
        PushParent(value);
    }

    void UIEndPropertyRow()
    {
        PopParent(GetCurrentParent()); // value
        PopParent(GetCurrentParent()); // row
    }

    void UIPropertyLabel(const char* label, const String8& value)
    {
        UIBeginPropertyRow(label);
        UI_Widget* text = PushWidget(WidgetFlags_DrawText, value, HashCStr("v"),
                                     { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
        text->style_vars[StyleVar_Padding] = Vec4(2.0f * Dpi(), 2.0f * Dpi(), 0.0f, 0.0f);
        UISetTextColour(text, Colours().Text);
        UIEndPropertyRow();
    }

    bool UIPropertyFloat(const char* label, float* value, float speed, float minValue, float maxValue, const char* fmt)
    {
        UIBeginPropertyRow(label);
        const bool changed = DragField("v", value, speed, minValue, maxValue, fmt, { SizeKind_Grow, 1.0f }, nullptr, false);
        UIEndPropertyRow();
        return changed;
    }

    bool UIPropertyInt(const char* label, int* value, float speed, int minValue, int maxValue)
    {
        UIBeginPropertyRow(label);
        const bool changed = UIDragIntField("v", value, speed, minValue, maxValue, 0.0f);
        UIEndPropertyRow();
        return changed;
    }

    static bool CheckBox(const char* str, bool* value)
    {
        const float dpi = Dpi();
        const Palette c = Colours();
        u64 hash;
        HandleUIString(str, &hash);

        UI_Widget* box = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground | WidgetFlags_DrawText,
                                    *value ? Str8Lit(ICON_MDI_CHECK) : Str8Lit(""), hash,
                                    { SizeKind_Pixels, 18.0f * dpi }, { SizeKind_Pixels, 18.0f * dpi });
        UIClearBorders(box);
        box->TextAlignment                     = UI_Text_Alignment_Center_X | UI_Text_Alignment_Center_Y;
        box->style_vars[StyleVar_Padding]      = Vec4(0.0f);
        box->style_vars[StyleVar_CornerRadius] = Vec4(Rounding() * dpi, 0.0f, 0.0f, 0.0f);
        if(*value)
            UISetBackground(box, c.Accent, c.Accent, c.Accent);
        else
            UISetBackground(box, c.Field, c.Hover, c.Hover);
        UISetTextColour(box, UIGetToolTheme().AccentText);

        if(HandleWidgetInteraction(box).clicked)
        {
            *value = !*value;
            return true;
        }
        return false;
    }

    bool UIPropertyBool(const char* label, bool* value)
    {
        UIBeginPropertyRow(label);
        const bool changed = CheckBox("v", value);
        UIEndPropertyRow();
        return changed;
    }

    static bool VecRow(const char* label, float* v, int count, float speed)
    {
        const UIToolTheme& theme = UIGetToolTheme();
        const Vec4 kAxisTint[4]  = { theme.AxisX, theme.AxisY, theme.AxisZ, theme.Warning };
        static const char* kAxisId[4] = { "x", "y", "z", "w" };

        UIBeginPropertyRow(label);
        bool changed = false;
        for(int i = 0; i < count; i++)
            changed |= DragField(kAxisId[i], &v[i], speed, 0.0f, 0.0f, "%.2f", { SizeKind_Grow, 1.0f }, &kAxisTint[i], false);
        UIEndPropertyRow();
        return changed;
    }

    bool UIPropertyVec2(const char* label, float* xy, float speed) { return VecRow(label, xy, 2, speed); }
    bool UIPropertyVec3(const char* label, float* xyz, float speed) { return VecRow(label, xyz, 3, speed); }
    bool UIPropertyVec4(const char* label, float* xyzw, float speed) { return VecRow(label, xyzw, 4, speed); }

    // ---- colour picker -----------------------------------------------------

    namespace
    {
        Vec3 HsvToRgb(float h, float s, float v)
        {
            h             = h - floorf(h);
            const float i = floorf(h * 6.0f);
            const float f = h * 6.0f - i;
            const float p = v * (1.0f - s);
            const float q = v * (1.0f - f * s);
            const float t = v * (1.0f - (1.0f - f) * s);
            switch((int)i % 6)
            {
            case 0: return Vec3(v, t, p);
            case 1: return Vec3(q, v, p);
            case 2: return Vec3(p, v, t);
            case 3: return Vec3(p, q, v);
            case 4: return Vec3(t, p, v);
            default: return Vec3(v, p, q);
            }
        }

        void RgbToHsv(const float* rgb, float* h, float* s, float* v)
        {
            const float maxC  = Maths::Max(rgb[0], Maths::Max(rgb[1], rgb[2]));
            const float minC  = Maths::Min(rgb[0], Maths::Min(rgb[1], rgb[2]));
            const float delta = maxC - minC;
            *v                = maxC;
            *s                = maxC > 0.0001f ? delta / maxC : 0.0f;
            // grey has no hue of its own: the caller keeps the one it had
            if(delta < 0.0001f)
                return;
            float hue;
            if(maxC == rgb[0])
                hue = (rgb[1] - rgb[2]) / delta;
            else if(maxC == rgb[1])
                hue = 2.0f + (rgb[2] - rgb[0]) / delta;
            else
                hue = 4.0f + (rgb[0] - rgb[1]) / delta;
            hue /= 6.0f;
            *h = hue < 0.0f ? hue + 1.0f : hue;
        }

        // the colour the open picker is on, kept as hue / saturation / value so dragging through
        // black or grey does not lose the hue
        u64 s_PickFor        = 0;
        float s_PickHue      = 0.0f;
        float s_PickSat      = 0.0f;
        float s_PickVal      = 0.0f;
        float s_PickStart[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        float s_PickLast[4]  = { 0.0f, 0.0f, 0.0f, 1.0f };
        char s_PickHex[16]   = { 0 };

        void PickerQuad(u64 tag, float x, float y, float w, float h, const Vec4& colour, float rounding = 0.0f, bool border = false)
        {
            UI_Widget* quad = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | (border ? WidgetFlags_DrawBorder : 0), Str8Lit(""), tag,
                                         { SizeKind_Pixels, w }, { SizeKind_Pixels, h });
            quad->relative_position                 = Vec2(x, y);
            quad->style_vars[StyleVar_Padding]      = Vec4(0.0f);
            quad->style_vars[StyleVar_CornerRadius] = Vec4(rounding, 0.0f, 0.0f, 0.0f);
            UIClearBorders(quad);
            if(border)
            {
                const Vec4 edge                             = Vec4(0.0f, 0.0f, 0.0f, 0.75f);
                quad->style_vars[StyleVar_Border]            = Vec4(Maths::Max(1.0f, floorf(Dpi())), Maths::Max(1.0f, floorf(Dpi())), 0.0f, 0.0f);
                quad->style_vars[StyleVar_BorderColor]       = edge;
                quad->style_vars[StyleVar_HotBorderColor]    = edge;
                quad->style_vars[StyleVar_ActiveBorderColor] = edge;
            }
            UISetBackground(quad, colour, colour, colour);
        }

        // An area that takes a press and a drag; tx / ty come back 0..1 across it. Left open as the parent.
        UI_Widget* PickerArea(u64 tag, float w, float h, float* tx, float* ty, bool* active)
        {
            UI_Widget* area = PushWidget(WidgetFlags_Clickable | WidgetFlags_Draggable, Str8Lit(""), tag, { SizeKind_Pixels, w }, { SizeKind_Pixels, h });
            UIClearBorders(area);
            area->style_vars[StyleVar_Padding] = Vec4(0.0f);
            area->drag_constraint_x            = true;
            area->drag_constraint_y            = true;
            UI_Interaction interaction         = HandleWidgetInteraction(area);
            *active                            = interaction.pressed || area->dragging;
            if(*active)
            {
                const Vec2 mouse = UIMousePosition() - area->position;
                *tx              = Maths::Clamp(mouse.x / Maths::Max(1.0f, w), 0.0f, 1.0f);
                *ty              = Maths::Clamp(mouse.y / Maths::Max(1.0f, h), 0.0f, 1.0f);
            }
            PushParent(area);
            return area;
        }

        void WriteHex(const float* rgba, bool alpha)
        {
            auto byte = [](float v) { return (int)floorf(Maths::Clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f); };
            if(alpha)
                snprintf(s_PickHex, sizeof(s_PickHex), "%02X%02X%02X%02X", byte(rgba[0]), byte(rgba[1]), byte(rgba[2]), byte(rgba[3]));
            else
                snprintf(s_PickHex, sizeof(s_PickHex), "%02X%02X%02X", byte(rgba[0]), byte(rgba[1]), byte(rgba[2]));
        }

        // Square of saturation x value, hue strip, alpha strip, before / after, hex and channel fields.
        bool ColourPickerBody(u64 owner, float* rgba, bool alpha)
        {
            UI_State* ui    = UI();
            const float dpi = ui->DPIScale;
            const Palette c = Colours();
            const int count = alpha ? 4 : 3;

            // just opened, or the value was changed from outside: take it in
            bool outside = s_PickFor != owner;
            for(int i = 0; i < count && !outside; i++)
                outside = Maths::Abs(rgba[i] - s_PickLast[i]) > 0.0005f;
            if(outside)
            {
                if(s_PickFor != owner)
                {
                    for(int i = 0; i < 4; i++)
                        s_PickStart[i] = i < count ? rgba[i] : 1.0f;
                }
                s_PickFor = owner;
                RgbToHsv(rgba, &s_PickHue, &s_PickSat, &s_PickVal);
                WriteHex(rgba, alpha);
            }

            const float width   = floorf(220.0f * dpi);
            const float squareH = floorf(150.0f * dpi);
            const float stripH  = floorf(14.0f * dpi);
            const float gap     = floorf(8.0f * dpi);
            bool changed        = false;
            bool active         = false;
            float tx = 0.0f, ty = 0.0f;

            UI_Widget* body = PushWidget(WidgetFlags_StackVertically, Str8Lit(""), UIHashCombine(owner, 0x5049434B4552ull), { SizeKind_Pixels, width + gap * 2.0f }, { SizeKind_ChildSum, 1.0f });
            body->style_vars[StyleVar_Padding]     = Vec4(gap, gap, 0.0f, 0.0f);
            body->style_vars[StyleVar_ItemSpacing] = Vec4(0.0f, gap, 0.0f, 0.0f);
            PushParent(body);

            // ---- saturation across, value down ----
            {
                PickerArea(0x53565351ull, width, squareH, &tx, &ty, &active);
                if(active)
                {
                    s_PickSat = tx;
                    s_PickVal = 1.0f - ty;
                    changed   = true;
                }
                const int cols = 24, rows = 16;
                const float cw = width / (float)cols, ch = squareH / (float)rows;
                for(int y = 0; y < rows; y++)
                    for(int x = 0; x < cols; x++)
                    {
                        const Vec3 rgb = HsvToRgb(s_PickHue, ((float)x + 0.5f) / (float)cols, 1.0f - ((float)y + 0.5f) / (float)rows);
                        PickerQuad(0x100000ull + (u64)(y * cols + x), floorf((float)x * cw), floorf((float)y * ch), ceilf(cw) + 1.0f, ceilf(ch) + 1.0f, Vec4(rgb.x, rgb.y, rgb.z, 1.0f));
                    }
                const float knob = floorf(12.0f * dpi);
                const Vec3 now   = HsvToRgb(s_PickHue, s_PickSat, s_PickVal);
                PickerQuad(0x4B31ull, s_PickSat * width - knob * 0.5f, (1.0f - s_PickVal) * squareH - knob * 0.5f, knob, knob, Vec4(1.0f, 1.0f, 1.0f, 1.0f), knob * 0.5f, true);
                PickerQuad(0x4B32ull, s_PickSat * width - knob * 0.5f + 2.0f * dpi, (1.0f - s_PickVal) * squareH - knob * 0.5f + 2.0f * dpi, knob - 4.0f * dpi, knob - 4.0f * dpi,
                           Vec4(now.x, now.y, now.z, 1.0f), knob * 0.5f);
                PopParent(GetCurrentParent());
            }

            // ---- hue ----
            {
                PickerArea(0x485545ull, width, stripH, &tx, &ty, &active);
                if(active)
                {
                    s_PickHue = Maths::Min(tx, 0.999f);
                    changed   = true;
                }
                const int steps = 48;
                const float sw  = width / (float)steps;
                for(int i = 0; i < steps; i++)
                {
                    const Vec3 rgb = HsvToRgb(((float)i + 0.5f) / (float)steps, 1.0f, 1.0f);
                    PickerQuad(0x200000ull + (u64)i, floorf((float)i * sw), 0.0f, ceilf(sw) + 1.0f, stripH, Vec4(rgb.x, rgb.y, rgb.z, 1.0f));
                }
                const float knob = floorf(6.0f * dpi);
                PickerQuad(0x4B33ull, s_PickHue * width - knob * 0.5f, -2.0f * dpi, knob, stripH + 4.0f * dpi, Vec4(1.0f, 1.0f, 1.0f, 1.0f), knob * 0.5f, true);
                PopParent(GetCurrentParent());
            }

            // ---- alpha, over a checker so see-through reads as see-through ----
            if(alpha)
            {
                PickerArea(0x414C50ull, width, stripH, &tx, &ty, &active);
                if(active)
                {
                    rgba[3] = tx;
                    changed = true;
                }
                const int steps = 32;
                const float sw  = width / (float)steps;
                const Vec3 rgb  = HsvToRgb(s_PickHue, s_PickSat, s_PickVal);
                for(int i = 0; i < steps; i++)
                {
                    const float grey = (i % 2) ? 0.42f : 0.62f;
                    const float a    = ((float)i + 0.5f) / (float)steps;
                    // blended by hand: one opaque quad per step
                    PickerQuad(0x300000ull + (u64)i, floorf((float)i * sw), 0.0f, ceilf(sw) + 1.0f, stripH,
                               Vec4(grey + (rgb.x - grey) * a, grey + (rgb.y - grey) * a, grey + (rgb.z - grey) * a, 1.0f));
                }
                const float knob = floorf(6.0f * dpi);
                PickerQuad(0x4B34ull, Maths::Clamp(rgba[3], 0.0f, 1.0f) * width - knob * 0.5f, -2.0f * dpi, knob, stripH + 4.0f * dpi, Vec4(1.0f, 1.0f, 1.0f, 1.0f), knob * 0.5f, true);
                PopParent(GetCurrentParent());
            }

            if(changed)
            {
                const Vec3 rgb = HsvToRgb(s_PickHue, s_PickSat, s_PickVal);
                rgba[0] = rgb.x, rgba[1] = rgb.y, rgba[2] = rgb.z;
                WriteHex(rgba, alpha);
            }

            // ---- before | now, and the hex code ----
            {
                UI_Widget* row = PushWidget(WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY, Str8Lit(""), 0x524F5731ull, { SizeKind_Pixels, width }, { SizeKind_Pixels, floorf(24.0f * dpi) });
                row->style_vars[StyleVar_Padding]     = Vec4(0.0f);
                row->style_vars[StyleVar_ItemSpacing] = Vec4(gap, 0.0f, 0.0f, 0.0f);
                PushParent(row);

                // pressing the old colour goes back to it
                UI_Widget* before = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Clickable, Str8Lit(""), 0x4F4C44ull, { SizeKind_Pixels, floorf(38.0f * dpi) }, { SizeKind_Pixels, floorf(22.0f * dpi) });
                UIClearBorders(before);
                before->style_vars[StyleVar_CornerRadius] = Vec4(Rounding() * dpi, 0.0f, 0.0f, 0.0f);
                const Vec4 old                            = Vec4(s_PickStart[0], s_PickStart[1], s_PickStart[2], 1.0f);
                UISetBackground(before, old, old, old);
                if(HandleWidgetInteraction(before).clicked)
                {
                    for(int i = 0; i < count; i++)
                        rgba[i] = s_PickStart[i];
                    RgbToHsv(rgba, &s_PickHue, &s_PickSat, &s_PickVal);
                    WriteHex(rgba, alpha);
                    changed = true;
                }
                UITooltip("The colour it was. Press to go back to it");

                UI_Widget* after = PushWidget(WidgetFlags_DrawBackground, Str8Lit(""), 0x4E4557ull, { SizeKind_Pixels, floorf(38.0f * dpi) }, { SizeKind_Pixels, floorf(22.0f * dpi) });
                UIClearBorders(after);
                after->style_vars[StyleVar_CornerRadius] = Vec4(Rounding() * dpi, 0.0f, 0.0f, 0.0f);
                const Vec4 now                           = Vec4(rgba[0], rgba[1], rgba[2], 1.0f);
                UISetBackground(after, now, now, now);

                UIPushStyle(StyleVar_TextColor, c.Dim);
                UILabelCStr("##pickhash", "#");
                UIPopStyle(StyleVar_TextColor);
                char hexBefore[16];
                snprintf(hexBefore, sizeof(hexBefore), "%s", s_PickHex);
                UITextField("###pickhex", s_PickHex, sizeof(s_PickHex), alpha ? "RRGGBBAA" : "RRGGBB", 0.0f);
                if(strcmp(hexBefore, s_PickHex) != 0)
                {
                    // taken as soon as it is a whole code
                    const size_t length = strlen(s_PickHex);
                    bool valid          = length == 6 || (alpha && length == 8);
                    for(size_t i = 0; i < length && valid; i++)
                        valid = isxdigit((unsigned char)s_PickHex[i]) != 0;
                    if(valid)
                    {
                        const unsigned long value = strtoul(s_PickHex, nullptr, 16);
                        const int shift           = length == 8 ? 8 : 0;
                        rgba[0]                   = (float)((value >> (16 + shift)) & 0xFF) / 255.0f;
                        rgba[1]                   = (float)((value >> (8 + shift)) & 0xFF) / 255.0f;
                        rgba[2]                   = (float)((value >> shift) & 0xFF) / 255.0f;
                        if(length == 8)
                            rgba[3] = (float)(value & 0xFF) / 255.0f;
                        RgbToHsv(rgba, &s_PickHue, &s_PickSat, &s_PickVal);
                        changed = true;
                    }
                }
                PopParent(row);
            }

            // ---- channels, 0..255 ----
            {
                UI_Widget* row = PushWidget(WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY, Str8Lit(""), 0x524F5732ull, { SizeKind_Pixels, width }, { SizeKind_ChildSum, 1.0f });
                row->style_vars[StyleVar_Padding]     = Vec4(0.0f);
                row->style_vars[StyleVar_ItemSpacing] = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);
                PushParent(row);
                static const char* kIds[4]  = { "###pickr", "###pickg", "###pickb", "###picka" };
                static const Vec4 kTints[4] = { Vec4(0.95f, 0.45f, 0.42f, 1.0f), Vec4(0.50f, 0.85f, 0.48f, 1.0f), Vec4(0.45f, 0.65f, 0.98f, 1.0f), Vec4(0.80f, 0.80f, 0.82f, 1.0f) };
                for(int i = 0; i < count; i++)
                {
                    float value = floorf(Maths::Clamp(rgba[i], 0.0f, 1.0f) * 255.0f + 0.5f);
                    if(UIDragFloatField(kIds[i], &value, 1.0f, 0.0f, 255.0f, "%.0f", 0.0f, &kTints[i]))
                    {
                        rgba[i] = value / 255.0f;
                        if(i < 3)
                            RgbToHsv(rgba, &s_PickHue, &s_PickSat, &s_PickVal);
                        WriteHex(rgba, alpha);
                        changed = true;
                    }
                }
                PopParent(row);
            }

            PopParent(body);

            for(int i = 0; i < 4; i++)
                s_PickLast[i] = i < count ? rgba[i] : 1.0f;
            return changed;
        }
    }

    bool UIPropertyColour(const char* label, float* rgba, bool alpha)
    {
        const float dpi = Dpi();
        const Palette c = Colours();
        bool changed    = false;

        UIBeginPropertyRow(label);

        UI_Widget* swatch = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground | WidgetFlags_DrawBorder, Str8Lit(""),
                                       HashCStr("swatch"),
                                       { SizeKind_Grow, 1.0f }, { SizeKind_Pixels, 20.0f * dpi });
        const Vec4 colour = Vec4(rgba[0], rgba[1], rgba[2], 1.0f);
        swatch->style_vars[StyleVar_Padding]      = Vec4(0.0f);
        swatch->style_vars[StyleVar_Border]       = Vec4(1.0f * dpi, 1.0f * dpi, 0.0f, 0.0f);
        swatch->style_vars[StyleVar_CornerRadius] = Vec4(Rounding() * dpi, 0.0f, 0.0f, 0.0f);
        swatch->style_vars[StyleVar_ShadowColor]  = Vec4(0.0f);
        swatch->style_vars[StyleVar_BorderColor]       = c.Border;
        swatch->style_vars[StyleVar_HotBorderColor]    = c.Accent;
        swatch->style_vars[StyleVar_ActiveBorderColor] = c.Accent;
        UISetBackground(swatch, colour, colour, colour);

        // one popup id per swatch: the widget's own (parent-chained) hash
        char popupId[40];
        snprintf(popupId, sizeof(popupId), "##colour%llx", (unsigned long long)swatch->hash);

        if(HandleWidgetInteraction(swatch).clicked)
            UIOpenPopup(popupId);

        // LUMOS_UI_PICKER=1: the first swatch drawn after a couple of seconds opens its picker (screenshots)
        static bool s_PickerHook = getenv("LUMOS_UI_PICKER") != nullptr;
        if(s_PickerHook && UI()->FrameIndex > 150)
        {
            s_PickerHook = false;
            UIOpenPopup(popupId);
        }

        if(UIBeginPopup(popupId))
        {
            changed |= ColourPickerBody(swatch->hash, rgba, alpha);
            UIEndPopup();
        }

        UIEndPropertyRow();
        return changed;
    }

    bool UIPropertyText(const char* label, char* buffer, u32 bufferSize, const char* placeholder)
    {
        char before[256];
        snprintf(before, sizeof(before), "%s", buffer);

        UIBeginPropertyRow(label);
        UITextField("v", buffer, bufferSize, placeholder ? placeholder : "", 0.0f);
        UIEndPropertyRow();

        return strncmp(before, buffer, sizeof(before) - 1) != 0;
    }

    bool UIPropertyDropdown(const char* label, int* selected, const char** options, int optionCount)
    {
        const float dpi = Dpi();
        const Palette c = Colours();
        bool changed    = false;

        UIBeginPropertyRow(label);

        const char* current = (*selected >= 0 && *selected < optionCount) ? options[*selected] : "";
        UI_Widget* button   = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground | WidgetFlags_DrawText,
                                         Str8C((char*)current), HashCStr("dd"),
                                         { SizeKind_Grow, 1.0f }, { SizeKind_TextContent, 1.0f });
        UIClearBorders(button);
        button->style_vars[StyleVar_Padding]      = Vec4(6.0f * dpi, 3.0f * dpi, 0.0f, 0.0f);
        button->style_vars[StyleVar_CornerRadius] = Vec4(Rounding() * dpi, 0.0f, 0.0f, 0.0f);
        UISetBackground(button, c.Field, c.Hover, c.Hover);
        UISetTextColour(button, c.Text);

        char popupId[40];
        snprintf(popupId, sizeof(popupId), "##dropdown%llx", (unsigned long long)button->hash);

        if(HandleWidgetInteraction(button).clicked)
            UIOpenPopup(popupId);

        if(UIBeginPopup(popupId))
        {
            for(int i = 0; i < optionCount; i++)
            {
                char itemId[96];
                snprintf(itemId, sizeof(itemId), "%s###opt%d", options[i], i);
                bool isCurrent = (i == *selected);
                if(UIMenuItem(itemId, nullptr, &isCurrent))
                {
                    changed   = *selected != i;
                    *selected = i;
                }
            }
            UIEndPopup();
        }

        UIEndPropertyRow();
        return changed;
    }

    bool UISectionHeader(const char* str, bool defaultOpen, bool* removeClicked)
    {
        const float dpi = Dpi();
        const Palette c = Colours();

        u64 hash;
        String8 text = HandleUIString(str, &hash);

        UI_Widget* row = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground | WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY,
                                    Str8Lit(""), hash,
                                    { SizeKind_PercentOfParent, 1.0f },
                                    { SizeKind_MaxChild, 1.0f });
        if(row->FirstFrame)
            row->StateValue = defaultOpen ? 1u : 0u;

        UIClearBorders(row);
        row->style_vars[StyleVar_Padding]      = Vec4(4.0f * dpi, 3.0f * dpi, 0.0f, 0.0f);
        row->style_vars[StyleVar_ItemSpacing]  = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);
        row->style_vars[StyleVar_CornerRadius] = Vec4(Rounding() * dpi, 0.0f, 0.0f, 0.0f);
        UISetBackground(row, c.Field, c.Hover, c.Hover);

        if(HandleWidgetInteraction(row).clicked)
            row->StateValue = row->StateValue ? 0u : 1u;
        const bool open = row->StateValue != 0;

        PushParent(row);
        {
            UI_Widget* arrow = PushWidget(WidgetFlags_DrawText, open ? Str8Lit(ICON_MDI_CHEVRON_DOWN) : Str8Lit(ICON_MDI_CHEVRON_RIGHT),
                                          UIHashCombine(hash, 0x4152ull),
                                          { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
            arrow->style_vars[StyleVar_Padding] = Vec4(0.0f, 1.0f * dpi, 0.0f, 0.0f);
            UISetTextColour(arrow, c.Dim);

            UI_Widget* title = PushWidget(WidgetFlags_DrawText, text, UIHashCombine(hash, 0x5449ull),
                                          { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
            title->style_vars[StyleVar_Padding] = Vec4(0.0f, 1.0f * dpi, 0.0f, 0.0f);
            UISetTextColour(title, c.Text);

            if(removeClicked)
            {
                UIFlexSpacer();
                *removeClicked = UIIconButton(ICON_MDI_CLOSE "###remove", 18.0f).clicked;
            }
        }
        PopParent(row);

        return open;
    }

    // ---- tables ------------------------------------------------------------

    static void TableCloseCell(TableFrame& t)
    {
        if(t.cell)
        {
            PopParent(t.cell);
            t.cell = nullptr;
        }
    }

    static void TableCloseRow(TableFrame& t)
    {
        TableCloseCell(t);
        if(t.row)
        {
            PopParent(t.row);
            t.row = nullptr;
        }
    }

    void UIBeginTable(const char* str, int columns, float* fractions)
    {
        u64 hash;
        HandleUIString(str, &hash);

        UI_Widget* table = PushWidget(WidgetFlags_StackVertically, Str8Lit(""), hash,
                                      { SizeKind_PercentOfParent, 1.0f },
                                      { SizeKind_ChildSum, 1.0f });
        table->style_vars[StyleVar_Padding]     = Vec4(0.0f);
        table->style_vars[StyleVar_ItemSpacing] = Vec4(0.0f);
        PushParent(table);

        if(s_TableDepth < 4)
        {
            TableFrame& t = s_Tables[s_TableDepth];
            t             = {};
            t.table       = table;
            t.fractions   = fractions;
            t.columns     = Maths::Clamp(columns, 1, 16);
        }
        s_TableDepth++;
    }

    void UITableNextRow(float height, u64 stableId)
    {
        if(s_TableDepth <= 0 || s_TableDepth > 4)
            return;
        TableFrame& t = s_Tables[s_TableDepth - 1];
        TableCloseRow(t);

        const UI_Size sizeY = height > 0.0f ? UI_Size { SizeKind_Pixels, height * Dpi() } : UI_Size { SizeKind_MaxChild, 1.0f };
        t.row               = PushWidget(WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY, Str8Lit(""),
                                         UIHashCombine(0x54524F57ull, stableId ? stableId : (u64)t.rowIndex),
                                         { SizeKind_PercentOfParent, 1.0f }, sizeY);
        t.row->style_vars[StyleVar_Padding]     = Vec4(0.0f);
        t.row->style_vars[StyleVar_ItemSpacing] = Vec4(0.0f);
        PushParent(t.row);
        t.rowIndex++;
        t.column = 0;
    }

    void UITableNextColumn()
    {
        if(s_TableDepth <= 0 || s_TableDepth > 4)
            return;
        TableFrame& t = s_Tables[s_TableDepth - 1];
        if(!t.row)
            UITableNextRow(0.0f, 0);
        TableCloseCell(t);

        const int column     = Maths::Min(t.column, t.columns - 1);
        const float fraction = t.fractions ? t.fractions[column] : 1.0f / (float)t.columns;

        t.cell = PushWidget(WidgetFlags_StackVertically, Str8Lit(""),
                            UIHashCombine(0x5443454Cull, (u64)t.column),
                            { SizeKind_PercentOfParent, fraction },
                            { SizeKind_MaxChild, 1.0f });
        t.cell->style_vars[StyleVar_Padding] = Vec4(4.0f * Dpi(), 1.0f * Dpi(), 0.0f, 0.0f);
        t.cell->Clip                         = true;
        PushParent(t.cell);
        t.column++;
    }

    void UITableHeader(const char** names)
    {
        if(s_TableDepth <= 0 || s_TableDepth > 4)
            return;
        TableFrame& t   = s_Tables[s_TableDepth - 1];
        UI_State* ui    = UI();
        const float dpi = ui->DPIScale;
        const Palette c = Colours();

        UITableNextRow(0.0f, 0x48445221ull);
        UI_Widget* row = t.row;
        row->flags |= WidgetFlags_DrawBackground;
        row->style_vars[StyleVar_BackgroundColor] = c.Field;
        row->style_vars[StyleVar_CornerRadius]    = Vec4(0.0f);

        for(int i = 0; i < t.columns; i++)
        {
            UITableNextColumn();
            UI_Widget* label = PushWidget(WidgetFlags_DrawText, Str8C((char*)names[i]), HashCStr("h"),
                                          { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
            label->style_vars[StyleVar_Padding] = Vec4(0.0f, 2.0f * dpi, 0.0f, 0.0f);
            UISetTextColour(label, c.Dim);
        }
        TableCloseCell(t);

        // dividers: drag to move width between the two columns either side
        if(t.fractions && row->size.x > 1.0f)
        {
            const Vec2 mouse = UIMousePosition();
            float edge       = 0.0f;
            for(int i = 0; i < t.columns - 1; i++)
            {
                edge += t.fractions[i];
                UI_Widget* grip = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                             Str8Lit(""), UIHashCombine(0x54475250ull, (u64)i),
                                             { SizeKind_Pixels, 5.0f * dpi }, { SizeKind_Pixels, Maths::Max(row->size.y, 4.0f) });
                UIClearBorders(grip);
                grip->relative_position                  = Vec2(edge * row->size.x - 2.5f * dpi, 0.0f);
                grip->style_vars[StyleVar_Padding]       = Vec4(0.0f);
                grip->style_vars[StyleVar_CornerRadius]  = Vec4(0.0f);
                UISetBackground(grip, Vec4(0.0f), Vec4(c.Accent.x, c.Accent.y, c.Accent.z, 0.6f), c.Accent);

                UI_Interaction interaction = HandleWidgetInteraction(grip);
                if(interaction.pressed)
                {
                    s_TableDragStart[0] = t.fractions[i];
                    s_TableDragStart[1] = t.fractions[i + 1];
                }
                if(ui->active_widget == grip->hash && MouseHeld())
                {
                    const float total = s_TableDragStart[0] + s_TableDragStart[1];
                    const float moved = (mouse.x - grip->press_mouse_p.x) / row->size.x;
                    const float left  = Maths::Clamp(s_TableDragStart[0] + moved, 0.05f, total - 0.05f);
                    t.fractions[i]     = left;
                    t.fractions[i + 1] = total - left;
                }
            }
        }

        TableCloseRow(t);
    }

    void UIEndTable()
    {
        if(s_TableDepth <= 0)
            return;
        s_TableDepth--;
        if(s_TableDepth < 4)
        {
            TableFrame& t = s_Tables[s_TableDepth];
            TableCloseRow(t);
            PopParent(t.table);
        }
        else
            PopParent(GetCurrentParent());
    }

    // ---- splitter ----------------------------------------------------------

    bool UISplitter(const char* str, bool vertical, float* value, float minValue, float maxValue, float thickness, bool invert)
    {
        UI_State* ui    = UI();
        const float dpi = ui->DPIScale;
        const Palette c = Colours();

        u64 hash;
        HandleUIString(str, &hash);

        const UI_Size thin = { SizeKind_Pixels, thickness * dpi };
        const UI_Size full = { SizeKind_PercentOfParent, 1.0f };
        UI_Widget* bar     = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground, Str8Lit(""), hash,
                                        vertical ? thin : full, vertical ? full : thin);
        UIClearBorders(bar);
        bar->style_vars[StyleVar_Padding]      = Vec4(0.0f);
        bar->style_vars[StyleVar_CornerRadius] = Vec4(0.0f);
        UISetBackground(bar, Vec4(0.0f), Vec4(c.Accent.x, c.Accent.y, c.Accent.z, 0.6f), c.Accent);

        UI_Interaction interaction = HandleWidgetInteraction(bar);
        if(interaction.pressed)
            memcpy(&bar->StateValue, value, sizeof(float));

        if(ui->active_widget == bar->hash && MouseHeld())
        {
            float start = 0.0f;
            memcpy(&start, &bar->StateValue, sizeof(float));
            const Vec2 moved = UIMousePosition() - bar->press_mouse_p;
            const float d    = (vertical ? moved.x : moved.y) * (invert ? -1.0f : 1.0f);
            *value           = Maths::Clamp(start + d, minValue, maxValue);
            return true;
        }
        return false;
    }

    // ---- drag and drop -----------------------------------------------------

    bool UIDragSource(const char* type, const void* data, u32 size, const String8& preview)
    {
        UI_State* ui      = UI();
        UI_Widget* parent = GetCurrentParent();
        UI_Widget* source = parent ? parent->last : nullptr;
        if(!source)
            return false;

        if(!s_Drag.active && ui->active_widget == source->hash && MouseHeld())
        {
            const Vec2 moved = UIMousePosition() - source->press_mouse_p;
            const float slop = 6.0f * ui->DPIScale;
            if(moved.x * moved.x + moved.y * moved.y > slop * slop)
            {
                s_Drag        = {};
                s_Drag.active = true;
                s_Drag.state  = ui;
                s_Drag.type   = HashCStr(type);
                s_Drag.source = source->hash;
                s_Drag.size   = Maths::Min(size, (u32)sizeof(s_Drag.data));
                if(data && s_Drag.size)
                    memcpy(s_Drag.data, data, s_Drag.size);
                snprintf(s_Drag.preview, sizeof(s_Drag.preview), "%.*s", (int)preview.size, preview.size ? (const char*)preview.str : "");
            }
        }

        return s_Drag.active && s_Drag.state == ui && s_Drag.source == source->hash;
    }

    bool UIDropTarget(const char* type, void* outData, u32 size)
    {
        UI_State* ui = UI();
        if(!s_Drag.active || s_Drag.state != ui || s_Drag.consumed || s_Drag.type != HashCStr(type))
            return false;

        UI_Widget* parent = GetCurrentParent();
        UI_Widget* target = parent ? parent->last : nullptr;
        if(!target || target->hash == s_Drag.source)
            return false;

        const Vec2 mouse = UIMousePosition();
        const Vec4& clip = target->ClipRect;
        if(!PointInRect(mouse, WidgetRect(target)) || mouse.x < clip.x || mouse.x > clip.z || mouse.y < clip.y || mouse.y > clip.w)
            return false;

        // show where it would land
        const Palette c = Colours();
        const Vec4 lit  = Vec4(c.Accent.x, c.Accent.y, c.Accent.z, 0.30f);
        target->flags |= WidgetFlags_DrawBackground;
        UISetBackground(target, lit, lit, lit);

        if(!s_Drag.dropFrame)
            return false;

        if(outData && size)
            memcpy(outData, s_Drag.data, Maths::Min(size, s_Drag.size));
        s_Drag.consumed = true;
        return true;
    }

    bool UIIsDragDropActive(const char* type)
    {
        if(!s_Drag.active || s_Drag.state != UI())
            return false;
        return !type || s_Drag.type == HashCStr(type);
    }

    // ---- file dialog -------------------------------------------------------

    namespace
    {
        struct FileRow
        {
            char Name[128];
            bool Folder;
        };

        constexpr u32 kMaxFileRows = 600;

        struct FileDialog
        {
            bool Open         = false;
            bool Listed       = false;
            bool PickFolder   = false;
            char Title[64]    = { 0 };
            char Accept[24]   = { 0 };
            char Extension[16] = { 0 };
            char Folder[1024] = { 0 };
            char PathEdit[1024] = { 0 };
            char Filter[64]   = { 0 };
            i32 Selected      = -1;
            f32 Scroll        = 0.0f;
            u32 Count         = 0;
            FileRow Rows[kMaxFileRows];
            void (*OnPicked)(const char*, void*) = nullptr;
            void* User                          = nullptr;
            UI_State* State                     = nullptr;
        };
        FileDialog s_File;

        bool EndsWithNoCase(const char* text, const char* suffix)
        {
            const size_t a = strlen(text), b = strlen(suffix);
            if(b > a)
                return false;
            for(size_t i = 0; i < b; i++)
            {
                char x = text[a - b + i], y = suffix[i];
                if(x >= 'A' && x <= 'Z')
                    x = char(x + 32);
                if(y >= 'A' && y <= 'Z')
                    y = char(y + 32);
                if(x != y)
                    return false;
            }
            return true;
        }

        void FileDialogList()
        {
            s_File.Count    = 0;
            s_File.Selected = -1;
            s_File.Scroll   = 0.0f;
            s_File.Listed   = true;
            snprintf(s_File.PathEdit, sizeof(s_File.PathEdit), "%s", s_File.Folder);

            std::error_code error;
            std::filesystem::directory_iterator it(s_File.Folder, std::filesystem::directory_options::skip_permission_denied, error);
            if(error)
                return;
            for(const auto& entry : it)
            {
                if(s_File.Count >= kMaxFileRows)
                    break;
                const std::string name = entry.path().filename().string();
                if(name.empty() || name[0] == '.')
                    continue;
                const bool folder = entry.is_directory(error);
                if(!folder && (s_File.PickFolder || (s_File.Extension[0] && !EndsWithNoCase(name.c_str(), s_File.Extension))))
                    continue;
                FileRow& row = s_File.Rows[s_File.Count++];
                snprintf(row.Name, sizeof(row.Name), "%s", name.c_str());
                row.Folder = folder;
            }

            // folders first, then by name
            for(u32 i = 1; i < s_File.Count; i++)
            {
                const FileRow row = s_File.Rows[i];
                i32 j             = (i32)i - 1;
                while(j >= 0 && ((!s_File.Rows[j].Folder && row.Folder) || (s_File.Rows[j].Folder == row.Folder && strcasecmp(s_File.Rows[j].Name, row.Name) > 0)))
                {
                    s_File.Rows[j + 1] = s_File.Rows[j];
                    j--;
                }
                s_File.Rows[j + 1] = row;
            }
        }

        void FileDialogGo(const char* folder)
        {
            std::error_code error;
            if(!folder || !folder[0] || !std::filesystem::is_directory(folder, error))
                return;
            snprintf(s_File.Folder, sizeof(s_File.Folder), "%s", folder);
            // no trailing slash, except the root
            size_t length = strlen(s_File.Folder);
            while(length > 1 && s_File.Folder[length - 1] == '/')
                s_File.Folder[--length] = 0;
            s_File.Filter[0] = 0;
            FileDialogList();
        }

        void FileDialogClose()
        {
            s_File.Open = false;
            if(s_File.State)
            {
                s_File.State->FocusedTextInput    = 0;
                s_File.State->TextInputBuffer     = nullptr;
                s_File.State->TextInputBufferSize = 0;
            }
        }

        void FileDialogPick(const char* name)
        {
            char path[1280];
            if(name && name[0])
                snprintf(path, sizeof(path), "%s%s%s", s_File.Folder, strcmp(s_File.Folder, "/") == 0 ? "" : "/", name);
            else
                snprintf(path, sizeof(path), "%s", s_File.Folder);
            // closed first: the callback may open another one
            void (*picked)(const char*, void*) = s_File.OnPicked;
            void* user                        = s_File.User;
            FileDialogClose();
            if(picked)
                picked(path, user);
        }
    }

    void UIOpenFileDialog(const UIFileDialogDesc& desc)
    {
        s_File.Open       = true;
        s_File.PickFolder = desc.PickFolder;
        s_File.OnPicked   = desc.OnPicked;
        s_File.User       = desc.User;
        s_File.State      = UI();
        snprintf(s_File.Title, sizeof(s_File.Title), "%s", desc.Title ? desc.Title : "Open");
        snprintf(s_File.Accept, sizeof(s_File.Accept), "%s", desc.Accept ? desc.Accept : "Open");
        snprintf(s_File.Extension, sizeof(s_File.Extension), "%s", desc.Extension ? desc.Extension : "");

        const char* home = getenv("HOME");
        s_File.Folder[0] = 0;
        FileDialogGo(desc.StartFolder);
        if(!s_File.Folder[0])
            FileDialogGo(home ? home : "/");
        if(!s_File.Folder[0])
            FileDialogGo("/");
    }

    bool UIFileDialogIsOpen() { return s_File.Open; }

    void UIDrawFileDialog()
    {
        UI_State* ui = UI();
        if(!s_File.Open || s_File.State != ui)
            return;

        const UIToolTheme& theme = UIGetToolTheme();
        const float dpi          = ui->DPIScale;
        const Vec2 view          = ui->root_parent.size;
        const float width        = Maths::Min(680.0f * dpi, view.x * 0.94f);
        const float listHeight   = Maths::Clamp(view.y * 0.5f, 160.0f * dpi, 460.0f * dpi);
        const float rowLogical   = 26.0f;

        if(Input::Get().GetKeyPressedRaw(InputCode::Key::Escape))
        {
            FileDialogClose();
            return;
        }

        const bool outside = UIDimBackdrop("##filedialogdim", theme.Dark ? 0.45f : 0.25f).clicked;

        UIPushStyle(StyleVar_FontSize, theme.FontSize * dpi);
        UIPushStyle(StyleVar_BackgroundColor, theme.Popup);
        UIPushStyle(StyleVar_Padding, Vec2(14.0f * dpi, 12.0f * dpi));
        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 8.0f * dpi));
        UIPushStyle(StyleVar_CornerRadius, 12.0f * dpi);
        UIPushStyle(StyleVar_TextColor, theme.Text);
        UIBeginOverlay("##filedialog", SizeKind_Pixels, width, SizeKind_ChildSum, 1.0f, WidgetFlags_StackVertically | WidgetFlags_AnimateAppear | WidgetFlags_DrawBorder);
        UIWindowAnchor(UIAnchor_MiddleCenter, 0.0f, 0.0f);
        {
            UI_Widget* panel                           = ui->parents.Back();
            panel->style_vars[StyleVar_Border]         = Vec4(Maths::Max(1.0f, dpi), Maths::Max(1.0f, dpi), 0.0f, 0.0f);
            panel->style_vars[StyleVar_BorderColor]    = theme.Border;
            panel->style_vars[StyleVar_HotBorderColor] = theme.Border;
        }
        UIPopStyle(StyleVar_TextColor);
        UIPopStyle(StyleVar_CornerRadius);
        UIPopStyle(StyleVar_ItemSpacing);
        UIPopStyle(StyleVar_Padding);
        UIPopStyle(StyleVar_BackgroundColor);

        char goTo[1024] = { 0 };
        char pick[160]  = { 0 };
        bool pickNow    = false;
        bool cancel     = false;

        // ---- title ----
        UIBeginRowFullWidth();
        ui->parents.Back()->flags |= WidgetFlags_CentreChildrenY;
        UIPushStyle(StyleVar_FontSize, theme.FontSize * 1.15f * dpi);
        UILabelCStr("##filedialogtitle", s_File.Title);
        UIPopStyle(StyleVar_FontSize);
        UIFlexSpacer();
        if(s_File.Extension[0])
        {
            UIPushStyle(StyleVar_TextColor, theme.TextDim);
            UILabelCStr("##filedialogkind", s_File.Extension);
            UIPopStyle(StyleVar_TextColor);
        }
        UIEndRow();

        // ---- where: up, home, the path (typed or pasted, Enter goes there) ----
        UIBeginRowFullWidth();
        {
            UI_Widget* row                        = ui->parents.Back();
            row->style_vars[StyleVar_ItemSpacing] = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);
            row->flags |= WidgetFlags_CentreChildrenY;

            if(UIIconButton(ICON_MDI_ARROW_UP "###filedialogup", 24.0f).clicked)
            {
                snprintf(goTo, sizeof(goTo), "%s", s_File.Folder);
                char* slash = strrchr(goTo, '/');
                if(slash && slash != goTo)
                    *slash = 0;
                else
                    snprintf(goTo, sizeof(goTo), "/");
            }
            UITooltip("Up a folder");
            if(UIIconButton(ICON_MDI_HOME "###filedialoghome", 24.0f).clicked)
                if(const char* home = getenv("HOME"))
                    snprintf(goTo, sizeof(goTo), "%s", home);
            UITooltip("Home");

            const bool wasFocused      = ui->FocusedTextInput != 0 && ui->TextInputBuffer == s_File.PathEdit;
            UI_Interaction field       = UITextField("###filedialogpath", s_File.PathEdit, sizeof(s_File.PathEdit), "Folder", 0.0f);
            const bool focused         = ui->FocusedTextInput != 0 && ui->TextInputBuffer == s_File.PathEdit;
            if(field.widget)
                field.widget->style_vars[StyleVar_Padding] = Vec4(8.0f * dpi, 5.0f * dpi, 0.0f, 0.0f);
            // Enter drops the focus: that is the moment to go
            if(wasFocused && !focused && strcmp(s_File.PathEdit, s_File.Folder) != 0)
                snprintf(goTo, sizeof(goTo), "%s", s_File.PathEdit);
        }
        UIEndRow();

        UIBeginRowFullWidth();
        {
            UI_Interaction field = UITextField("###filedialogfilter", s_File.Filter, sizeof(s_File.Filter), "Filter this folder", 0.0f);
            if(field.widget)
                field.widget->style_vars[StyleVar_Padding] = Vec4(8.0f * dpi, 5.0f * dpi, 0.0f, 0.0f);
        }
        UIEndRow();

        // ---- what is in it ----
        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 1.0f * dpi));
        UIBeginScrollArea("##filedialoglist", listHeight, &s_File.Scroll);
        {
            UI_Widget* area                            = ui->parents.Back();
            area->style_vars[StyleVar_BackgroundColor] = theme.Field;
            area->style_vars[StyleVar_BorderColor]     = Vec4(0.0f);
            area->style_vars[StyleVar_Border]          = Vec4(0.0f);
            area->style_vars[StyleVar_CornerRadius]    = Vec4(theme.WidgetRounding * dpi + 2.0f * dpi, 0.0f, 0.0f, 0.0f);
            area->style_vars[StyleVar_Padding]         = Vec4(4.0f * dpi, 4.0f * dpi, 0.0f, 0.0f);
        }
        u32 shown = 0;
        for(u32 i = 0; i < s_File.Count; i++)
        {
            const FileRow& entry = s_File.Rows[i];
            if(s_File.Filter[0])
            {
                // plain "contains", any case
                bool found = false;
                const size_t n = strlen(s_File.Filter);
                for(const char* c = entry.Name; *c && !found; c++)
                    found = strncasecmp(c, s_File.Filter, n) == 0;
                if(!found)
                    continue;
            }
            shown++;

            char id[32];
            snprintf(id, sizeof(id), "###filerow%u", i);
            const bool selected = (i32)i == s_File.Selected;
            UI_Interaction row  = UIBeginSelectableRow(id, selected, rowLogical);
            {
                UI_Widget* rowWidget                         = ui->parents.Back();
                rowWidget->style_vars[StyleVar_CornerRadius] = Vec4(theme.WidgetRounding * dpi, 0.0f, 0.0f, 0.0f);
                rowWidget->style_vars[StyleVar_Padding]      = Vec4(8.0f * dpi, 0.0f, 0.0f, 0.0f);
                rowWidget->style_vars[StyleVar_ItemSpacing]  = Vec4(8.0f * dpi, 0.0f, 0.0f, 0.0f);
                if(selected)
                    UISetBackground(rowWidget, theme.Selection, theme.Selection, theme.Selection);
                UIPushStyle(StyleVar_TextColor, entry.Folder ? theme.Accent : theme.TextDim);
                UILabelCStr("###icon", entry.Folder ? ICON_MDI_FOLDER : ICON_MDI_FILE);
                UIPopStyle(StyleVar_TextColor);
                UIPushStyle(StyleVar_TextColor, theme.Text);
                UILabelCStr("###name", entry.Name);
                UIPopStyle(StyleVar_TextColor);
            }
            UIEndSelectableRow();

            if(row.clicked)
                s_File.Selected = (i32)i;
            if(row.double_clicked)
            {
                if(entry.Folder)
                    snprintf(goTo, sizeof(goTo), "%s%s%s", s_File.Folder, strcmp(s_File.Folder, "/") == 0 ? "" : "/", entry.Name);
                else
                {
                    snprintf(pick, sizeof(pick), "%s", entry.Name);
                    pickNow = true;
                }
            }
        }
        if(shown == 0)
        {
            UIPushStyle(StyleVar_TextColor, theme.TextDim);
            UILabelCStr("##filedialogempty", s_File.Count == 0 ? "  Nothing here to pick" : "  Nothing matches");
            UIPopStyle(StyleVar_TextColor);
        }
        UIEndScrollArea();
        UIPopStyle(StyleVar_ItemSpacing);

        // ---- what will be picked, and the buttons ----
        const bool hasSelection = s_File.Selected >= 0 && s_File.Selected < (i32)s_File.Count;
        const FileRow* selected = hasSelection ? &s_File.Rows[s_File.Selected] : nullptr;
        // a folder picker takes the folder that is open when nothing inside it is selected
        const bool canAccept = s_File.PickFolder ? true : (selected != nullptr);

        UIBeginRowFullWidth();
        {
            UI_Widget* row                        = ui->parents.Back();
            row->style_vars[StyleVar_ItemSpacing] = Vec4(8.0f * dpi, 0.0f, 0.0f, 0.0f);
            row->flags |= WidgetFlags_CentreChildrenY;

            UIPushStyle(StyleVar_TextColor, theme.TextDim);
            UILabelEllipsis("##filedialogpicked", Str8C((char*)(selected ? selected->Name : (s_File.PickFolder ? "This folder" : "Nothing selected"))), width / dpi - 260.0f);
            UIPopStyle(StyleVar_TextColor);
            UIFlexSpacer();
            if(UIButton("Cancel###filedialogcancel").clicked)
                cancel = true;

            char accept[48];
            snprintf(accept, sizeof(accept), "%s###filedialogaccept", selected && selected->Folder && !s_File.PickFolder ? "Open Folder" : s_File.Accept);
            UIPushStyle(StyleVar_TextColor, canAccept ? theme.Text : theme.TextDim);
            const bool accepted = UIButton(accept).clicked || (canAccept && ui->FocusedTextInput == 0 && Input::Get().GetKeyPressedRaw(InputCode::Key::Enter));
            UIPopStyle(StyleVar_TextColor);
            if(accepted && canAccept)
            {
                if(selected && selected->Folder && !s_File.PickFolder)
                    snprintf(goTo, sizeof(goTo), "%s%s%s", s_File.Folder, strcmp(s_File.Folder, "/") == 0 ? "" : "/", selected->Name);
                else
                {
                    snprintf(pick, sizeof(pick), "%s", selected ? selected->Name : "");
                    pickNow = true;
                }
            }
        }
        UIEndRow();

        UIEndPanel();
        UIPopStyle(StyleVar_FontSize);

        // changes last: the listing was in use above
        if(pickNow)
            FileDialogPick(pick);
        else if(cancel || outside)
            FileDialogClose();
        else if(goTo[0])
            FileDialogGo(goTo);
    }
}
