#include "Precompiled.h"
#include "UI.h"
#include "Core/OS/Input.h"
#include "Core/OS/OS.h"
#include "Font.h"
#include "Maths/MathsUtilities.h"
#include "Utilities/StringUtilities.h"
#include "Graphics/RHI/Texture.h"
#include "Core/Application.h"
#include "Core/OS/Window.h"
#include "ImGui/IconsMaterialDesignIcons.h" // ICON_MDI_* string macros for use in UI labels
#include "UIInternal.h"
#include "Core/Undo.h"

#include <imgui/imgui.h>

#define HashUIName(Name) Lumos::StringUtilities::BasicHashFromString(Str8C(Name))
#define HashUIStr8Name(Name) Lumos::StringUtilities::BasicHashFromString(Name)

#ifdef ENABLE_UI_ASSERTS
#define UI_ASSERT ASSERT
#else
#define UI_ASSERT(...) ((void)0)
#endif

namespace Lumos
{
    static UI_State* s_UIState;
    static f32 s_UIFrameDT = 0.016f;

    // Scroll bars that only show while scrolling (or when the pointer is at them)
    static bool s_ScrollbarFade = false;
    struct ScrollbarFadeState
    {
        u64 hash;
        f32 lastOffset;
        f32 idle;
        f32 alpha;
        u64 lastFrame;
        f32 velocity;   // px/s, scroll physics
        f32 physOffset; // offset we left it at, to spot outside changes
    };
    static ScrollbarFadeState s_ScrollbarFades[64];

    void UISetScrollbarFade(bool enable) { s_ScrollbarFade = enable; }

    // Momentum after a flick and a rubber band past the ends
    static bool s_ScrollPhysics = false;
    void UISetScrollPhysics(bool enable) { s_ScrollPhysics = enable; }

    static ScrollbarFadeState& ScrollbarFadeFor(u64 hash, f32 offset)
    {
        ScrollbarFadeState* oldest = &s_ScrollbarFades[0];
        for(ScrollbarFadeState& f : s_ScrollbarFades)
        {
            if(f.hash == hash)
                return f;
            if(f.lastFrame < oldest->lastFrame)
                oldest = &f;
        }
        *oldest = { hash, offset, 10.0f, 0.0f, 0, 0.0f, offset };
        return *oldest;
    }

    UI_State* UICreateContext(Arena* arena)
    {
        UI_State* previous = s_UIState;
        InitialiseUI(arena);
        UI_State* created = s_UIState;
        if(previous)
            s_UIState = previous;
        return created;
    }

    void UIDestroyContext(UI_State* state)
    {
        if(!state)
            return;
        UI_State* previous = s_UIState;
        s_UIState          = state;
        ShutDownUI();
        s_UIState = (previous == state) ? nullptr : previous;
    }

    void UISetContext(UI_State* state)
    {
        if(state)
            s_UIState = state;
    }

    bool InitialiseUI(Arena* arena)
    {
        // constructed, not just zeroed: the member defaults (double-click time, safe-area scales,
        // tooltip delay...) were silently 0
        s_UIState                  = new(PushArray(arena, UI_State, 1)) UI_State();
        s_UIState->active_widget   = 0;
        s_UIState->hot_widget      = 0;
        s_UIState->next_hot_widget = 0;
        s_UIState->FrameIndex      = 0;
        s_UIState->AnimationRate   = 10.0f;

        s_UIState->UIArena      = arena;
        // Every widget's label and id string is formatted in here each frame, so
        // a text-heavy panel over a big scene needs real headroom.
        s_UIState->UIFrameArena     = ArenaAlloc(Megabytes(4));
        s_UIState->UIPrevFrameArena = ArenaAlloc(Megabytes(4));
        s_UIState->UIExitArena  = ArenaAlloc(Megabytes(1));

        s_UIState->WidgetAllocator = new PoolAllocator<UI_Widget>();
        s_UIState->parents         = TDArray<UI_Widget*>(arena);
        s_UIState->UndoTargets     = TDArray<UIUndoTarget>(arena);

        Style_Variable_List* style_variable_lists = s_UIState->style_variable_lists;

        for(u32 i = 0; i < StyleVar_Count; i++)
        {
            Style_Variable_List* style_variable_list = &style_variable_lists[i];
            Style_Variable* styleVariable            = PushObject(s_UIState->UIArena, Style_Variable);
            styleVariable->next                      = nullptr;
            styleVariable->prev                      = nullptr;

            style_variable_list->count      = 0;
            style_variable_list->first_free = nullptr;
            style_variable_list->first = style_variable_list->last = styleVariable;
        }

        style_variable_lists[StyleVar_Padding].first->value               = { 32.0f, 32.0f, 0.0f, 0.0f };
        style_variable_lists[StyleVar_Border].first->value                = { 1.0f, 1.0f, 0.0f, 0.0f };
        style_variable_lists[StyleVar_BorderColor].first->value           = { 0.7f, 0.7f, 0.7f, 1.0f };
        style_variable_lists[StyleVar_BackgroundColor].first->value       = { 0.95f, 0.95f, 0.95f, 1.0f };
        style_variable_lists[StyleVar_TextColor].first->value             = { 0.1f, 0.1f, 0.1f, 1.0f };
        style_variable_lists[StyleVar_HotBorderColor].first->value        = { 0.4f, 0.6f, 1.0f, 1.0f };
        style_variable_lists[StyleVar_HotBackgroundColor].first->value    = { 0.92f, 0.94f, 0.98f, 1.0f };
        style_variable_lists[StyleVar_HotTextColor].first->value          = { 0.05f, 0.05f, 0.05f, 1.0f };
        style_variable_lists[StyleVar_ActiveBorderColor].first->value     = { 0.3f, 0.5f, 0.95f, 1.0f };
        style_variable_lists[StyleVar_ActiveBackgroundColor].first->value = { 0.85f, 0.9f, 0.98f, 1.0f };
        style_variable_lists[StyleVar_ActiveTextColor].first->value       = { 0.0f, 0.0f, 0.0f, 1.0f };
        style_variable_lists[StyleVar_FontSize].first->value              = { 20.0f, 0.0f, 0.0f, 1.0f };
        style_variable_lists[StyleVar_CornerRadius].first->value          = { 6.0f, 0.0f, 0.0f, 0.0f };
        style_variable_lists[StyleVar_ShadowColor].first->value           = { 0.0f, 0.0f, 0.0f, 0.15f };
        style_variable_lists[StyleVar_ShadowOffset].first->value          = { 0.0f, 1.0f, 0.0f, 0.0f };
        style_variable_lists[StyleVar_ShadowBlur].first->value            = { 4.0f, 0.0f, 0.0f, 0.0f };
        style_variable_lists[StyleVar_ItemSpacing].first->value          = { 2.0f, 2.0f, 0.0f, 0.0f };
        style_variable_lists[StyleVar_Alpha].first->value                 = { 1.0f, 0.0f, 0.0f, 0.0f };

#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS)
        style_variable_lists[StyleVar_FontSize].first->value = { 22.0f, 0.0f, 0.0f, 1.0f };
#endif

        return true;
    }

    void ShutDownUI()
    {
        if(s_UIState)
        {
            HashMapDeinit(&s_UIState->widgets);
            ArenaRelease(s_UIState->UIFrameArena);
            ArenaRelease(s_UIState->UIPrevFrameArena);
            ArenaRelease(s_UIState->UIExitArena);
            delete s_UIState->WidgetAllocator;
            s_UIState->WidgetAllocator = nullptr;
        }
    }

    bool UIThemeIsDark()
    {
        const Vec4& bg = s_UIState->style_variable_lists[StyleVar_BackgroundColor].first->value;
        return (bg.x * 0.299f + bg.y * 0.587f + bg.z * 0.114f) < 0.5f;
    }

    void UISetNextFlags(u32 flags)
    {
        s_UIState->NextWidgetFlags |= flags;
    }

    void UIPushUndoTarget(void* ptr, u64 size, u64 owner, u64 typeKey)
    {
        s_UIState->UndoTargets.PushBack({ ptr, size, owner, typeKey, nullptr, nullptr });
    }

    void UIPushUndoCallbacks(UIUndoEditFn begin, UIUndoEditFn end, u64 owner)
    {
        s_UIState->UndoTargets.PushBack({ nullptr, 0, owner, 0, begin, end });
    }

    void UIPopUndoTarget()
    {
        if(!s_UIState->UndoTargets.Empty())
            s_UIState->UndoTargets.PopBack();
    }

    Vec2 UIMousePosition()
    {
        return Input::Get().GetMousePosition() * s_UIState->InputScale - s_UIState->InputOffset;
    }

    void UIPushStyle(StyleVar style_variable, float value)
    {
        UIPushStyle(style_variable, Vec4(value, 0.0f, 0.0f, 0.0f));
    }

    void UIPushStyle(StyleVar style_variable, const Vec2& value)
    {
        UIPushStyle(style_variable, Vec4(value, 0.0f, 0.0f));
    }

    void UIPushStyle(StyleVar style_variable, const Vec3& value)
    {
        UIPushStyle(style_variable, Vec4(value, 0.0f));
    }

    void UIPushStyle(StyleVar style_variable, const Vec4& value)
    {
        UI_ASSERT(style_variable < StyleVar_Count);

        Style_Variable_List* list = &s_UIState->style_variable_lists[style_variable];
        Style_Variable* variable  = nullptr;

        if(list->first_free)
        {
            variable         = list->first_free;
            list->first_free = list->first_free->next;
        }
        else
        {
            variable = PushObject(s_UIState->UIArena,
                                  Style_Variable);
        }

        variable->value = value;
        variable->next  = list->first;
        variable->prev  = list->last;

        list->first->prev = variable;
        list->last->next  = variable;
        list->last        = variable;

        list->count++;
    }

    void UIPopStyle(StyleVar style_variable)
    {
        UI_ASSERT(style_variable < StyleVar_Count);
        Style_Variable_List* list = &s_UIState->style_variable_lists[style_variable];
        UI_ASSERT(list->count);
        // Mismatched pop would eat the theme default and corrupt the list.
        if(list->count == 0)
            return;
        Style_Variable* last = list->last;
        list->last           = list->last->prev;
        list->last->next     = list->first;

        last->next       = list->first_free;
        list->first_free = last;

        list->count--;
    }

    UI_Widget* GetCurrentParent()
    {
        return s_UIState->parents.Back();
    }

    void PushParent(UI_Widget* widget)
    {
        s_UIState->parents.PushBack(widget);
    }

    void PopParent(UI_Widget* widget)
    {
        UI_ASSERT(!s_UIState->parents.Empty());
        UI_ASSERT(s_UIState->parents.Back() == widget);
        s_UIState->parents.PopBack();
    }

    static void
    SetWidgetStyleVars(UI_Widget* widget)
    {
        for(u32 i = 0; i < StyleVar_Count; i++)
        {
            Style_Variable_List* list = &s_UIState->style_variable_lists[i];
            widget->style_vars[i]     = list->last->value;
        }
    }

    // splitmix64 finaliser: ids differing by a character must not land on the same chained hash
    // (the old xor/shift combine collided for "art14" vs another row's "art19" - duplicate ids, layout cycles)
    static inline u64 UIHashMix(u64 x)
    {
        x ^= x >> 30;
        x *= 0xBF58476D1CE4E5B9ULL;
        x ^= x >> 27;
        x *= 0x94D049BB133111EBULL;
        x ^= x >> 31;
        return x;
    }

    u64 UIHashCombine(u64 a, u64 b)
    {
        return UIHashMix(a ^ UIHashMix(b + 0x9E3779B97F4A7C15ULL));
    }

    // Copy text into the widget's own storage (see UI_Widget::OwnedText) and
    // point it there. For engine-formatted strings, which must outlive the
    // frame arena they were built in.
    void UIAdoptText(UI_Widget* widget, const String8& src)
    {
        const u64 n = Maths::Min<u64>(src.size, sizeof(widget->OwnedText) - 1);
        if(n && src.str)
            memcpy(widget->OwnedText, src.str, (size_t)n);
        widget->OwnedText[n] = 0;
        widget->text.str     = (u8*)widget->OwnedText;
        widget->text.size    = n;
    }

    // One per UI context (there are only ever a few).
    struct LongPress
    {
        UI_State* Owner = nullptr;
        Vec2 At;
        f32 Start  = 0.0f;
        bool Down  = false;
        bool Done  = false; // fired during this press
        bool Fired = false; // fired this frame
    };
    static LongPress s_LongPress[4];

    static LongPress& LongPressFor(UI_State* ui)
    {
        for(LongPress& entry : s_LongPress)
            if(entry.Owner == ui)
                return entry;
        for(LongPress& entry : s_LongPress)
            if(!entry.Owner)
            {
                entry.Owner = ui;
                return entry;
            }
        s_LongPress[0]       = LongPress();
        s_LongPress[0].Owner = ui;
        return s_LongPress[0];
    }

    bool UILongPressed()
    {
        return s_UIState && LongPressFor(s_UIState).Fired;
    }

    UI_Widget* PushWidget(u32 flags,
                          const String8& text,
                          u64 hash,
                          UI_Size semantic_size_x,
                          UI_Size semantic_size_y)
    {
        UI_Widget* parent = GetCurrentParent();

        hash = UIHashCombine(parent->hash, hash);

        UI_Widget* widget = nullptr;
        HashMapFind(&s_UIState->widgets, hash, &widget);
        bool created = false;
        if(!widget)
        {
            void* mem                = s_UIState->WidgetAllocator->Allocate();
            widget                   = new(mem) UI_Widget();
            widget->HotTransition    = 0.0f;
            widget->ActiveTransition = 0.0f;
            widget->ToggleTransition = 0.0f;
            widget->ScaleAnimation   = 1.0f;
            widget->AppearTransition = 0.0f;
            // Unclipped until the post-layout propagation pass runs.
            widget->ClipRect         = Vec4(-1.0e9f, -1.0e9f, 1.0e9f, 1.0e9f);
            created                  = true;
            HashMapInsert(&s_UIState->widgets, hash, widget);
        }
        widget->FirstFrame = created;
        static u64 s_WidgetBuildCounter = 0;
        widget->BuildIndex = ++s_WidgetBuildCounter;
        // Rebuilt while mid-exit-fade = revived.
        widget->ExitTransition = 0.0f;

        // Same id built twice in one frame. The linking below assumes each
        // widget goes into a child list once; re-linking one that is already in
        // a list can leave the sibling pointers crossed, and the layout walk
        // then goes round in circles (the app hung with the trade terminal
        // open). Reported, not prevented: some widgets deliberately share an id
        // across siblings, and the layout walk has its own cycle guard.
        if(widget->LastFrameIndexActive == s_UIState->FrameIndex)
        {
            LWARN("UI: duplicate widget id this frame: '%.*s' under '%.*s' - disambiguate with '#id'",
                  (int)text.size, text.size ? (const char*)text.str : "",
                  (int)parent->text.size, parent->text.size ? (const char*)parent->text.str : "");
        }

        widget->parent               = parent;
        widget->flags                = flags | s_UIState->NextWidgetFlags;
        s_UIState->NextWidgetFlags   = 0;
        // Own the text until the frame is drawn: callers (Lua especially) pass temporaries
        // that can be freed and reused before layout / render read them
        widget->text = text;
        if(text.size > 0)
        {
            u8* copy = PushArrayNoZero(s_UIState->UIFrameArena, u8, text.size + 1);
            if(copy)
            {
                MemoryCopy(copy, text.str, text.size);
                copy[text.size] = 0;
                widget->text    = Str8(copy, text.size);
            }
        }
        widget->hash                 = hash;
        widget->texture              = nullptr;
        widget->MonoText             = s_UIState->MonoTextDepth > 0;
        widget->LastFrameIndexActive = s_UIState->FrameIndex;
        widget->ActiveTransition     = 0.0f;
        widget->Anchor               = UIAnchor_None;
        widget->AnchorMargin         = Vec2(0.0f, 0.0f);
        widget->Rotation             = 0.0f;
        widget->WarpStrips           = 0;
        widget->WarpVertical         = false;
        widget->FlipV                = false;
        widget->RenderSize           = Vec2(0.0f, 0.0f);
        widget->Clip                 = false;
        widget->ChildOffset          = Vec2(0.0f, 0.0f);
        widget->HighlightBits[0]     = 0;
        widget->HighlightBits[1]     = 0;
        // StateValue deliberately NOT reset — persistent per-widget state.

        widget->first = NULL;
        widget->last  = NULL;
        widget->next  = NULL;
        widget->prev  = NULL;

        widget->semantic_size[UIAxis_X] = semantic_size_x;
        widget->semantic_size[UIAxis_Y] = semantic_size_y;

        if(!parent->first || parent->last == widget || parent->first == widget)
        {
            parent->first = widget;
            parent->last  = widget;
        }
        else
        {
            widget->prev      = parent->last;
            widget->prev->next = widget;
            parent->last      = widget;
        }

        SetWidgetStyleVars(widget);

        return widget;
    }

    UI_Interaction HandleWidgetInteraction(UI_Widget* widget)
    {
        UI_ASSERT(widget);

        UI_Interaction interaction = {};
        interaction.widget         = widget;

        if(!(widget->flags & WidgetFlags_Clickable) && !(widget->flags & WidgetFlags_Draggable) && !(widget->flags & WidgetFlags_DragParent))
        {
            return interaction;
        }

        const Vec2& position = widget->position;
        const Vec2& size     = widget->size;
        const Vec2& mouse    = Input::Get().GetMousePosition() * s_UIState->InputScale - s_UIState->InputOffset; // Needs to take Quality setting render scale into accoutn too

        bool hovering = mouse.x >= position.x && mouse.x <= position.x + size.x && mouse.y >= position.y && mouse.y <= position.y + size.y;
        const Vec4& clip = widget->ClipRect;
        hovering = hovering && mouse.x >= clip.x && mouse.x <= clip.z && mouse.y >= clip.y && mouse.y <= clip.w;
        if(hovering && !s_UIState->OverlayBlocksInput)
        {
            // If this widget is marked to drag/forward interaction to its parent, forward the hot widget to parent
            if((widget->flags & WidgetFlags_DragParent) && widget->parent)
            {
                s_UIState->next_hot_widget = widget->parent->hash;
            }
            else
            {
                s_UIState->next_hot_widget = widget->hash;
            }
            interaction.hovering = true;
        }

        // A press on anything inside an undo-target scope may be the start of an edit:
        // mark the target now, the commit happens once the pointer lets go.
        if(widget->clicked && s_UIState->UndoTargets.Size() > 0)
        {
            const UIUndoTarget& target = s_UIState->UndoTargets.Back();
            if(target.Ptr)
            {
                UndoPushOwned(target.Ptr, (i64)target.Size, target.Owner, target.TypeKey);
                s_UIState->UndoPending = true;
            }
            else if(target.Begin && !s_UIState->UndoEndFn)
            {
                target.Begin(target.Owner);
                s_UIState->UndoEndFn    = target.End;
                s_UIState->UndoEndOwner = target.Owner;
                s_UIState->UndoPending  = true;
            }
        }

        interaction.clicked        = s_UIState->ClickOnRelease ? widget->released : widget->clicked;
        interaction.right_clicked  = widget->right_clicked;
        interaction.double_clicked = widget->double_clicked;
        interaction.dragging       = widget->dragging;
        interaction.pressed        = widget->clicked;
        interaction.released       = widget->released;
        if(widget->dragging)
            interaction.drag_delta = mouse - widget->drag_mouse_p;

        return interaction;
    }

    // LUMOS_FAKE_SAFE_AREA="top,bottom,left,right" in points: try a notched phone layout on desktop
    static SafeAreaInsets FakeSafeArea(const SafeAreaInsets& raw)
    {
        static const char* s_Fake = getenv("LUMOS_FAKE_SAFE_AREA");
        if(!s_Fake)
            return raw;
        float t = 0, b = 0, l = 0, r = 0;
        sscanf(s_Fake, "%f,%f,%f,%f", &t, &b, &l, &r);
        const float d = s_UIState->DPIScale;
        return { t * d, b * d, l * d, r * d };
    }

    SafeAreaInsets UIRawSafeAreaInsets()
    {
        return FakeSafeArea(OS::Get().GetSafeAreaInsets());
    }

    SafeAreaInsets UIResolveSafeAreaInsets(const SafeAreaInsets& raw, bool isEditor)
    {
        SafeAreaInsets sa = FakeSafeArea(raw);

        // Game, pure Fullscreen mode: edge-to-edge, ignore the safe area.
        const int saMode = Application::Get().GetProjectSettings().SafeAreaMode;
        if(!isEditor && saMode == (int)Application::SafeAreaLayout::Fullscreen)
            return { 0.0f, 0.0f, 0.0f, 0.0f };

        float pointScale = s_UIState->DPIScale;
#ifdef LUMOS_PLATFORM_IOS
        pointScale = OS::Get().GetSafeAreaScale();
#endif
        float sTop = 1.0f, sBottom = 1.0f, sLeft = 1.0f, sRight = 1.0f;
        float minMarginPt = 0.0f;
        if(isEditor)
        {
#ifdef LUMOS_PLATFORM_IOS
            minMarginPt = kSafeAreaEditorMarginPt;
#endif
        }
        else
        {
            const UISafeAreaConfig& cfg = s_UIState->SafeArea;
            sTop = cfg.ScaleTop; sBottom = cfg.ScaleBottom;
            sLeft = cfg.ScaleLeft; sRight = cfg.ScaleRight;
            minMarginPt = cfg.MinMargin;
        }
        const float m = minMarginPt * pointScale;
        sa.top    = Maths::Max(sa.top * sTop, m);
        sa.bottom = Maths::Max(sa.bottom * sBottom, m);
        sa.left   = Maths::Max(sa.left * sLeft, m);
        sa.right  = Maths::Max(sa.right * sRight, m);
        return sa;
    }

    static void UIKeyboardFrameCheck();
    static void UIScribbleBeginFrame();

    void UIBeginFrame(const Vec2& frame_buffer_size, f32 dt, const Vec2& inputOffset)
    {
        LUMOS_PROFILE_FUNCTION();

        // A push without its pop leaks across frames: every widget built after
        // it silently inherits the stale value, and the effect shows up
        // somewhere else entirely. Say so once per offending var.
        for(u32 v = 0; v < StyleVar_Count; v++)
        {
            if(s_UIState->style_variable_lists[v].count != 0)
            {
                LWARN("UI: style var %u left pushed %d deep at end of frame - a UIPushStyle without its UIPopStyle",
                      v, (int)s_UIState->style_variable_lists[v].count);
            }
        }

        if(s_UIState->MonoTextDepth != 0)
        {
            LWARN("UI: UIPushMonoText left pushed %u deep at end of frame", s_UIState->MonoTextDepth);
            s_UIState->MonoTextDepth = 0;
        }

        Swap(s_UIState->UIFrameArena, s_UIState->UIPrevFrameArena);
        ArenaClear(s_UIState->UIFrameArena);

        // An edit started by a widget is one undo step: closed once nothing is held or being typed into.
        if(s_UIState->UndoPending && s_UIState->active_widget == 0 && s_UIState->FocusedTextInput == 0)
        {
            if(s_UIState->UndoEndFn)
                s_UIState->UndoEndFn(s_UIState->UndoEndOwner);
            s_UIState->UndoEndFn = nullptr;
            UndoCommit();
            s_UIState->UndoPending = false;
        }
        s_UIState->UndoTargets.Clear();

        s_UIState->InputOffset         = inputOffset;
        s_UIState->FrameBufferSize     = frame_buffer_size;
        s_UIState->next_hot_widget     = 0;
        s_UIState->OverlayBlocksInput  = false;
        s_UIState->CurrentTime += dt;
        s_UIState->parents.Clear();
        s_UIState->FrameIndex++;
        UIKeyboardFrameCheck();
        UIScribbleBeginFrame();
        s_UIState->AnimationRateDT = s_UIState->AnimationRate * dt;
        s_UIFrameDT                = dt;
        s_UIState->TreeIndentLevel = 0;

        s_UIState->PendingFocusWidget = 0;
        if(s_UIState->TabPressed || s_UIState->ShiftTabPressed)
        {
            i32 numFocusable = (i32)s_UIState->FocusableWidgets.Size();
            if(numFocusable > 0)
            {
                if(s_UIState->TabPressed)
                    s_UIState->FocusIndex = (s_UIState->FocusIndex + 1) % numFocusable;
                else
                    s_UIState->FocusIndex = (s_UIState->FocusIndex - 1 + numFocusable) % numFocusable;
                s_UIState->PendingFocusWidget = s_UIState->FocusableWidgets[s_UIState->FocusIndex];
            }
        }
        s_UIState->TabPressed      = false;
        s_UIState->ShiftTabPressed = false;

        s_UIState->FocusableWidgets.Clear();
        s_UIState->WidgetIdCounter = 0;

        // Handle dragging BEFORE building widgets so position updates are immediate
        if(s_UIState->active_widget && s_UIState->active_widget_state)
        {
            Vec2 mouse_p = Input::Get().GetMousePosition() * s_UIState->InputScale - inputOffset;

            if(Input::Get().GetMouseHeld(Lumos::InputCode::MouseKey::ButtonLeft))
            {
                if((s_UIState->active_widget_state->flags & WidgetFlags_Draggable))
                {
                    if(!s_UIState->active_widget_state->dragging)
                    {
                        s_UIState->active_widget_state->dragging     = true;
                        s_UIState->active_widget_state->drag_mouse_p = mouse_p;
                        s_UIState->active_widget_state->drag_offset  = mouse_p - s_UIState->active_widget_state->position;
                    }
                    else
                    {
                        UI_Widget* widget = s_UIState->active_widget_state;
                        UI_Widget* parent = widget->parent;

                        if(widget->drag_constraint_x)
                            mouse_p.x = widget->drag_mouse_p.x;

                        if(widget->drag_constraint_y)
                            mouse_p.y = widget->drag_mouse_p.y;

                        Vec2 parentPos  = parent ? parent->position : Vec2(0.0f, 0.0f);
                        Vec2 parentSize = parent ? parent->size : frame_buffer_size;
                        Vec2 minAbs     = parentPos;
                        Vec2 maxAbs     = parentPos + parentSize - widget->size;
                        Vec2 desiredAbs = Maths::Clamp(mouse_p - widget->drag_offset, minAbs, maxAbs);

                        widget->relative_position = desiredAbs - parentPos;
                    }
                }
            }
        }

        SafeAreaInsets sa = OS::Get().GetSafeAreaInsets();
        const bool isEditor = Application::Get().GetAppType() == AppType::Editor;
        sa = UIResolveSafeAreaInsets(sa, isEditor);
        // this UI places its own controls round the notch
        if(s_UIState->FullBleed)
            sa = { 0.0f, 0.0f, 0.0f, 0.0f };

        {
            const float viewportMin = Maths::Min(
                frame_buffer_size.x - sa.left - sa.right,
                frame_buffer_size.y - sa.top - sa.bottom);
            const float baseFont = Maths::Clamp(viewportMin * 0.034f, 16.0f, 44.0f);
            auto* fontList = &s_UIState->style_variable_lists[StyleVar_FontSize];
            if(fontList->first)
                fontList->first->value.x = baseFont;
        }

        UI_Widget* root_parent                    = &s_UIState->root_parent;
        const Vec2 insetSize                      = Vec2(frame_buffer_size.x - sa.left - sa.right,
                                                         frame_buffer_size.y - sa.top - sa.bottom);
        root_parent->semantic_size[UIAxis_X]      = { SizeKind_Pixels, insetSize.x };
        root_parent->semantic_size[UIAxis_Y]      = { SizeKind_Pixels, insetSize.y };
        root_parent->hash                         = HashUIStr8Name(Str8Lit("root"));
        root_parent->flags                        = WidgetFlags_StackVertically;
        root_parent->text                         = Str8Lit("root");
        root_parent->style_vars[StyleVar_Padding] = { 0.0f, 0.0f, 0.0f, 0.0f };
        root_parent->style_vars[StyleVar_Border]  = { 0.0f, 0.0f, 0.0f, 0.0f };
        root_parent->cursor                       = { 0.0f, 0.0f };
        root_parent->position                     = { sa.left, sa.top };
        root_parent->size                         = insetSize;
        root_parent->first                        = NULL;
        root_parent->last                         = NULL;
        root_parent->next                         = NULL;
        root_parent->prev                         = NULL;

        PushParent(root_parent);
    }

    void UIEndFrame(Graphics::Font* font)
    {
        LUMOS_PROFILE_FUNCTION();
        for(u32 i = 0; i < StyleVar_Count; i++)
        {
            Style_Variable_List* list = &s_UIState->style_variable_lists[i];
            UI_ASSERT(list->count == 0);
        }

        PopParent(&s_UIState->root_parent);
        UI_ASSERT(s_UIState->parents.Empty());

        Input* input = &Input::Get();
        Vec2 mouse_p = input->GetMousePosition() * s_UIState->InputScale - s_UIState->InputOffset;

        if(!s_UIState->root_parent.first)
        {
            return;
        }

        ForHashMapEach(u64, UI_Widget*, &s_UIState->widgets, it)
        {
            UI_Widget* w        = *it.value;
            w->right_clicked    = false;
            w->clicked          = false;
            w->double_clicked   = false;
            w->released         = false;
        }

        // touch has no right button: a finger held still stands in for one
        LongPress& longPress = LongPressFor(s_UIState);
        longPress.Fired      = false;
        if(s_UIState->ClickOnRelease)
        {
            const bool held = Input::Get().GetMouseHeld(Lumos::InputCode::MouseKey::ButtonLeft);
            if(Input::Get().GetMouseClicked(Lumos::InputCode::MouseKey::ButtonLeft))
            {
                longPress.Down  = true;
                longPress.Done  = false;
                longPress.Start = s_UIState->CurrentTime;
                longPress.At    = mouse_p;
            }
            else if(!held)
                longPress.Down = false;
            if(longPress.Down && !longPress.Done)
            {
                const f32 slop = 12.0f * s_UIState->DPIScale;
                const Vec2 d   = mouse_p - longPress.At;
                if(d.x * d.x + d.y * d.y > slop * slop)
                    longPress.Down = false;
                else if(s_UIState->CurrentTime - longPress.Start >= 0.5f)
                {
                    longPress.Done = longPress.Fired = true;
                    if(UI_Widget* w = s_UIState->active_widget ? s_UIState->active_widget_state : nullptr)
                    {
                        w->right_clicked                        = true;
                        s_UIState->ContextMenuPos               = mouse_p;
                        s_UIState->ContextMenuTrigger           = s_UIState->active_widget;
                        s_UIState->ContextMenuOpen              = true;
                        s_UIState->ContextMenuCloseRequestFrame = 0;
                    }
                }
            }
        }
        else
            longPress.Down = false;

        // Handle mouse release (drag position updates moved to UIBeginFrame for responsiveness)
        if(s_UIState->active_widget)
        {
            if(Input::Get().GetMouseHeld(Lumos::InputCode::MouseKey::ButtonLeft))
            {
                s_UIState->active_widget_state->clicked = false;
            }
            else
            {
                // Mouse released
                UI_Widget* w = s_UIState->active_widget_state;
                w->clicked   = false;
                w->dragging  = false;
                w->released = mouse_p.x >= w->position.x && mouse_p.x <= w->position.x + w->size.x
                    && mouse_p.y >= w->position.y && mouse_p.y <= w->position.y + w->size.y;
                if(s_UIState->ClickOnRelease && w->released)
                {
                    // Finger travelled too far since press = a drag, not a tap.
                    const f32 slop = 12.0f * s_UIState->DPIScale;
                    Vec2 d         = mouse_p - w->press_mouse_p;
                    // lifting after a long press is not a tap as well
                    if(d.x * d.x + d.y * d.y > slop * slop || longPress.Done)
                        w->released = false;
                }
                w->drag_mouse_p          = { 0.0f, 0.0f };
                w->drag_offset           = { 0.0f, 0.0f };
                s_UIState->active_widget = 0;
            }
        }

        if(!s_UIState->active_widget)
        {
            s_UIState->hot_widget = s_UIState->next_hot_widget;

            if(Input::Get().GetMouseClicked(Lumos::InputCode::MouseKey::ButtonLeft))
            {
                if(s_UIState->ContextMenuOpen && s_UIState->ContextMenuCloseRequestFrame == 0)
                    s_UIState->ContextMenuCloseRequestFrame = s_UIState->FrameIndex;
                if(s_UIState->OpenDropdown && !s_UIState->OverlayBlocksInput && s_UIState->hot_widget != s_UIState->OpenDropdown)
                    s_UIState->OpenDropdown = 0;
            }

            if(s_UIState->hot_widget)
            {
                if(Input::Get().GetMouseClicked(Lumos::InputCode::MouseKey::ButtonLeft))
                {
                    s_UIState->active_widget = s_UIState->hot_widget;
                    s_UIState->hot_widget    = 0;

                    HashMapFind(&s_UIState->widgets, s_UIState->active_widget, &s_UIState->active_widget_state);

                    if(s_UIState->active_widget_state)
                    {
                        s_UIState->active_widget_state->clicked       = true;
                        s_UIState->active_widget_state->press_mouse_p = mouse_p;

                        // Double-click detection
                        f32 timeSinceLastClick = s_UIState->CurrentTime - s_UIState->LastClickTime;
                        if(s_UIState->LastClickedWidget == s_UIState->active_widget &&
                           timeSinceLastClick < s_UIState->DoubleClickTime)
                        {
                            s_UIState->active_widget_state->double_clicked = true;
                        }
                        else
                        {
                            s_UIState->active_widget_state->double_clicked = false;
                        }

                        s_UIState->LastClickTime     = s_UIState->CurrentTime;
                        s_UIState->LastClickedWidget = s_UIState->active_widget;
                    }
                }

                // Right-click detection
                if(Input::Get().GetMouseClicked(Lumos::InputCode::MouseKey::ButtonRight))
                {
                    UI_Widget* hotWidget = nullptr;
                    HashMapFind(&s_UIState->widgets, s_UIState->hot_widget, &hotWidget);
                    if(hotWidget)
                    {
                        hotWidget->right_clicked = true;
                        s_UIState->ContextMenuPos = Input::Get().GetMousePosition() * s_UIState->InputScale - s_UIState->InputOffset;
                        s_UIState->ContextMenuTrigger = s_UIState->hot_widget;
                        s_UIState->ContextMenuOpen = true;
                        s_UIState->ContextMenuCloseRequestFrame = 0;
                    }
                }
            }
        }

        TDArray<UI_Widget*> lWidgetsToDelete(s_UIState->UIFrameArena);
        bool anyExitFading = false;
        // a finger that has lifted isn't hovering: no highlight left on the last thing tapped
        const bool showHover = !s_UIState->ClickOnRelease || Input::Get().GetMouseHeld(Lumos::InputCode::MouseKey::ButtonLeft);
        ForHashMapEach(u64, UI_Widget*, &s_UIState->widgets, it)
        {
            u64 key          = *it.key;
            UI_Widget* value = *it.value;

            if((key == s_UIState->hot_widget && showHover) || key == s_UIState->active_widget)
            {
                if(key == s_UIState->hot_widget)
                    value->HotTransition += s_UIState->AnimationRateDT;
                else
                {
                    value->ActiveTransition += s_UIState->AnimationRateDT;
                    value->HotTransition += s_UIState->AnimationRateDT;
                }
            }
            else
            {
                value->HotTransition -= s_UIState->AnimationRateDT;
                value->ActiveTransition -= s_UIState->AnimationRateDT;
            }

            value->HotTransition    = Maths::Clamp(value->HotTransition, 0.0f, 1.0f);
            value->ActiveTransition = Maths::Clamp(value->ActiveTransition, 0.0f, 1.0f);

            // Scale animation for buttons (press effect)
            if(value->flags & WidgetFlags_AnimateScale)
            {
                float targetScale = (key == s_UIState->active_widget) ? 0.95f : 1.0f;
                value->ScaleAnimation += (targetScale - value->ScaleAnimation) * s_UIState->AnimationRateDT * 2.0f;
                value->ScaleAnimation = Maths::Clamp(value->ScaleAnimation, 0.9f, 1.0f);
            }

            // Appear pop-in ticks once after creation; renderer eases it.
            if(value->AppearTransition < 1.0f)
                value->AppearTransition = Maths::Clamp(value->AppearTransition + s_UIState->AnimationRateDT * 1.5f, 0.0f, 1.0f);

            if(value->LastFrameIndexActive < s_UIState->FrameIndex)
            {
                bool fading = value->ExitTransition > 0.0f;
                if(!fading)
                {
                    for(UI_Widget* p = value; p; p = p->parent)
                    {
                        if(p->flags & WidgetFlags_AnimateExit)
                        {
                            fading = true;
                            break;
                        }
                    }
                    if(fading)
                    {
                        value->ExitTransition = 0.0001f;
                        if(value->text.size > 0)
                            value->text = PushStr8Copy(s_UIState->UIExitArena, value->text);
                        i32 depth = 0;
                        for(UI_Widget* p = value->parent; p; p = p->parent)
                            depth++;
                        value->ExitDepth = depth;
                    }
                }

                if(fading && value->ExitTransition < 1.0f)
                {
                    value->ExitTransition = Maths::Clamp(value->ExitTransition + s_UIState->AnimationRateDT * 1.5f, 0.0f, 1.0f);
                    if(value->ExitTransition >= 1.0f)
                        lWidgetsToDelete.PushBack(value); // fade finished
                    else
                        anyExitFading = true; // renderer draws it via ExitTransition
                }
                else
                {
                    lWidgetsToDelete.PushBack(value);
                }
            }
        }

        // Exit-text copies are only needed while something is fading.
        if(!anyExitFading)
            ArenaClear(s_UIState->UIExitArena);

        if(lWidgetsToDelete.Size() > 0)
        {
            for(auto widget : lWidgetsToDelete)
                widget->LastFrameIndexActive = UINT64_MAX; // doomed marker; nothing reads it after this

            ForHashMapEach(u64, UI_Widget*, &s_UIState->widgets, it)
            {
                UI_Widget* w = *it.value;
                if(w->parent && w->parent->LastFrameIndexActive == UINT64_MAX)
                    w->parent = nullptr;
            }
        }

        for(auto widget : lWidgetsToDelete)
        {
            if(s_UIState->active_widget_state == widget)
            {
                s_UIState->active_widget       = 0;
                s_UIState->active_widget_state = nullptr;
            }
            if(s_UIState->hot_widget == widget->hash)
                s_UIState->hot_widget = 0;
            if(s_UIState->FocusedTextInput == widget->hash)
            {
                s_UIState->FocusedTextInput    = 0;
                s_UIState->TextInputBuffer     = nullptr;
                s_UIState->TextInputBufferSize = 0;
            }
            HashMapRemove(&s_UIState->widgets, widget->hash);
            s_UIState->WidgetAllocator->Deallocate(widget);
        }

        if(s_UIState->ContextMenuCloseRequestFrame > 0 &&
           s_UIState->FrameIndex > s_UIState->ContextMenuCloseRequestFrame)
        {
            s_UIState->ContextMenuOpen              = false;
            s_UIState->ContextMenuCloseRequestFrame = 0;
        }
    }

    String8 HandleUIString(const char* str, u64* out_hash)
    {
        String8 text = Str8C((char*)str);

        u64 marker = FindSubstr8(text, Str8Lit("###"), 0, MatchFlags(0));
        if(marker != text.size)
        {
            String8 idPart = Substr8(text, { marker + 3, text.size });
            *out_hash      = StringUtilities::BasicHashFromString(idPart);
            return Substr8(text, { 0, marker });
        }

        *out_hash = StringUtilities::BasicHashFromString(text);

        u64 last_hash = FindSubstr8(text, Str8Lit("#"), 0, MatchFlags::FindLast);
        if(last_hash != text.size)
        {
            text = Substr8(text, { 0, last_hash });
        }
        return text;
    }

    // Identity for the wrapper window a panel/overlay puts around its content.
    // Derived arithmetically from the content hash: this used to be a string
    // formatted into UIFrameArena every frame, and when that arena ran dry the
    // format came back EMPTY - so every panel in the frame hashed identically,
    // collapsed onto one widget, corrupted the sibling lists and sent the
    // layout walk round in circles (the app hung with the trade terminal open).
    static inline u64 UIWrapperHash(u64 contentHash, u64 salt)
    {
        u64 h = UIHashMix(contentHash ^ UIHashMix(salt));
        return h ? h : 1;
    }

    UI_Interaction UIBeginPanel(const char* str, u32 extraFlags)
    {
        return UIBeginPanel(str, SizeKind_MaxChild, 1.0f, SizeKind_ChildSum, 1.0f, extraFlags);
    }

    UI_Interaction UIBeginPanel(const char* str, SizeKind sizeKindX, float xValue, SizeKind sizeKindY, float yValue, u32 extraFlags)
    {
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        const u64 hashWindow = UIWrapperHash(hash, 0x57494E444F57ull); // "WINDOW"
        UI_Widget* window    = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | extraFlags,
                                         Str8Lit("Window"),
                                         hashWindow,
                                         { sizeKindX, xValue },
                                         { sizeKindY, yValue });

        // Enhanced panel appearance - theme aware, DPI scaled
        float dpi = s_UIState->DPIScale;
        window->style_vars[StyleVar_Padding]      = Vec4(0.0f);
        window->style_vars[StyleVar_CornerRadius] = Vec4(6.0f * dpi, 0.0f, 0.0f, 0.0f);
        window->style_vars[StyleVar_Border]       = Vec4(1.0f * dpi, 1.0f * dpi, 0.0f, 0.0f);
        window->style_vars[StyleVar_ShadowColor]  = Vec4(0.0f, 0.0f, 0.0f, 0.3f);
        window->style_vars[StyleVar_ShadowOffset] = Vec4(2.0f * dpi, 2.0f * dpi, 0.0f, 0.0f);
        window->style_vars[StyleVar_ShadowBlur]   = Vec4(5.0f * dpi, 0.0f, 0.0f, 0.0f);

        if(UIThemeIsDark())
        {
            window->style_vars[StyleVar_BackgroundColor] = Vec4(0.15f, 0.15f, 0.15f, 0.98f);
            window->style_vars[StyleVar_BorderColor]     = Vec4(0.35f, 0.35f, 0.35f, 1.0f);
        }
        else
        {
            window->style_vars[StyleVar_BackgroundColor] = Vec4(0.95f, 0.95f, 0.95f, 0.98f);
            window->style_vars[StyleVar_BorderColor]     = Vec4(0.7f, 0.7f, 0.7f, 1.0f);
        }

        PushParent(window);

        const u64 hashHeader = UIWrapperHash(hash, 0x484541444552ull); // "HEADER"
        String8 HeaderText2  = Str8Lit("Header");
        UIPushStyle(StyleVar_Padding, { 0.0f, 0.0f, 0.0f, 0.0f });
        UI_Widget* header = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_StackVertically | WidgetFlags_DragParent,
                                       HeaderText2,
                                       hashHeader,
                                       { SizeKind_PercentOfParent, 1.0f },
                                       { SizeKind_ChildSum, 1.0f });
        UIPopStyle(StyleVar_Padding);

        // Enhanced header appearance - theme aware
        if(UIThemeIsDark())
        {
            header->style_vars[StyleVar_BackgroundColor]    = Vec4(0.20f, 0.20f, 0.20f, 1.0f);
            header->style_vars[StyleVar_HotBackgroundColor] = Vec4(0.24f, 0.24f, 0.24f, 1.0f);
        }
        else
        {
            header->style_vars[StyleVar_BackgroundColor]    = Vec4(0.82f, 0.82f, 0.82f, 1.0f);
            header->style_vars[StyleVar_HotBackgroundColor] = Vec4(0.78f, 0.78f, 0.78f, 1.0f);
        }

        PushParent(header);

        UI_Widget* title = PushWidget(WidgetFlags_DrawText,
                                      text,
                                      hash,
                                      { SizeKind_TextContent, 1.0f },
                                      { SizeKind_TextContent, 1.0f });

        // Enhanced title text appearance
        title->style_vars[StyleVar_Padding]   = Vec4(8.0f * dpi, 5.0f * dpi, 0.0f, 0.0f);

        PopParent(header);

        return HandleWidgetInteraction(header);
    }

    void UIEndPanel()
    {
        PopParent(GetCurrentParent());
    }

    UI_Interaction UIArrow(const char* str, float angleRad, float length, float thickness, const Vec4& color)
    {
        u64 hash;
        String8 text = HandleUIString(str, &hash);
        UI_Widget* widget = PushWidget(WidgetFlags_DrawBackground,
                                       text,
                                       hash,
                                       { SizeKind_Pixels, length },
                                       { SizeKind_Pixels, length });
        widget->style_vars[StyleVar_BackgroundColor] = color;
        widget->style_vars[StyleVar_Border]          = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        widget->style_vars[StyleVar_CornerRadius]    = Vec4(thickness * 0.5f, 0.0f, 0.0f, 0.0f);
        widget->Rotation                             = angleRad;
        widget->RenderSize                           = Vec2(length, thickness);
        return HandleWidgetInteraction(widget);
    }

    UI_Interaction UIBeginOverlay(const char* str, SizeKind sizeKindX, float xValue, SizeKind sizeKindY, float yValue, u32 extraFlags)
    {
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        const u64 hashWindow = UIWrapperHash(hash, 0x4F5645524C4159ull); // "OVERLAY"
        UI_Widget* window    = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | extraFlags,
                                         Str8Lit("Overlay"),
                                         hashWindow,
                                         { sizeKindX, xValue },
                                         { sizeKindY, yValue });

        const float dpi = s_UIState->DPIScale;
        auto setIfDefault = [&](StyleVar v, const Vec4& val) {
            if(s_UIState->style_variable_lists[v].count == 0)
                window->style_vars[v] = val;
        };
        setIfDefault(StyleVar_CornerRadius, Vec4(18.0f * dpi, 0.0f, 0.0f, 0.0f));
        setIfDefault(StyleVar_Border,       Vec4(0.0f, 0.0f, 0.0f, 0.0f));
        setIfDefault(StyleVar_ShadowColor,  Vec4(0.0f, 0.0f, 0.0f, 0.35f));
        setIfDefault(StyleVar_ShadowOffset, Vec4(0.0f, 2.0f * dpi, 0.0f, 0.0f));
        setIfDefault(StyleVar_ShadowBlur,   Vec4(14.0f * dpi, 0.0f, 0.0f, 0.0f));
        setIfDefault(StyleVar_Padding,      Vec4(24.0f * dpi, 18.0f * dpi, 0.0f, 0.0f));
        setIfDefault(StyleVar_ItemSpacing,  Vec4(0.0f, 6.0f * dpi, 0.0f, 0.0f));

        if(UIThemeIsDark())
        {
            setIfDefault(StyleVar_BackgroundColor, Vec4(0.04f, 0.05f, 0.08f, 0.55f));
            setIfDefault(StyleVar_BorderColor,     Vec4(0.0f, 0.0f, 0.0f, 0.0f));
            setIfDefault(StyleVar_TextColor,       Vec4(1.0f, 1.0f, 1.0f, 1.0f));
        }
        else
        {
            setIfDefault(StyleVar_BackgroundColor, Vec4(1.0f, 1.0f, 1.0f, 0.62f));
            setIfDefault(StyleVar_BorderColor,     Vec4(0.0f, 0.0f, 0.0f, 0.0f));
            setIfDefault(StyleVar_TextColor,       Vec4(0.08f, 0.08f, 0.10f, 1.0f));
        }

        PushParent(window);
        return HandleWidgetInteraction(window);
    }

    // Helper: get root (framebuffer) size
    static Vec2 UIGetFrameBufferSize()
    {
        return s_UIState->root_parent.size;
    }

    static UI_Widget* UIGetCurrentWindow()
    {
        auto& parents = s_UIState->parents;
        if(parents.Size() < 2)
            return nullptr;
        return parents.Back();
    }

    void UIWindowDock(UIDockPosition pos, float sizePercent)
    {
        UI_Widget* window = UIGetCurrentWindow();
        if(!window) return;

        Vec2 fb = UIGetFrameBufferSize();
        window->flags |= WidgetFlags_Floating_X | WidgetFlags_Floating_Y;

        switch(pos)
        {
        case Dock_Left:
            window->relative_position = { 0.0f, 0.0f };
            window->semantic_size[UIAxis_X] = { SizeKind_Pixels, fb.x * sizePercent };
            window->semantic_size[UIAxis_Y] = { SizeKind_Pixels, fb.y };
            break;
        case Dock_Right:
            window->relative_position = { fb.x * (1.0f - sizePercent), 0.0f };
            window->semantic_size[UIAxis_X] = { SizeKind_Pixels, fb.x * sizePercent };
            window->semantic_size[UIAxis_Y] = { SizeKind_Pixels, fb.y };
            break;
        case Dock_Top:
            window->relative_position = { 0.0f, 0.0f };
            window->semantic_size[UIAxis_X] = { SizeKind_Pixels, fb.x };
            window->semantic_size[UIAxis_Y] = { SizeKind_Pixels, fb.y * sizePercent };
            break;
        case Dock_Bottom:
            window->relative_position = { 0.0f, fb.y * (1.0f - sizePercent) };
            window->semantic_size[UIAxis_X] = { SizeKind_Pixels, fb.x };
            window->semantic_size[UIAxis_Y] = { SizeKind_Pixels, fb.y * sizePercent };
            break;
        case Dock_Fill:
            window->relative_position = { 0.0f, 0.0f };
            window->semantic_size[UIAxis_X] = { SizeKind_Pixels, fb.x };
            window->semantic_size[UIAxis_Y] = { SizeKind_Pixels, fb.y };
            break;
        }
    }

    void UIWindowCenter()
    {
        UI_Widget* window = UIGetCurrentWindow();
        if(!window) return;
        window->flags |= WidgetFlags_CentreX | WidgetFlags_CentreY;
    }

    void UIWindowFillScreen()
    {
        UIWindowDock(Dock_Fill);
    }

    void UIWindowSetSize(float wPercent, float hPercent)
    {
        UI_Widget* window = UIGetCurrentWindow();
        if(!window) return;

        Vec2 fb = UIGetFrameBufferSize();
        window->semantic_size[UIAxis_X] = { SizeKind_Pixels, fb.x * wPercent };
        window->semantic_size[UIAxis_Y] = { SizeKind_Pixels, fb.y * hPercent };
    }

    void UIWindowAnchor(UIAnchor anchor, float marginX, float marginY)
    {
        UI_Widget* window = UIGetCurrentWindow();
        if(!window) return;
        window->Anchor       = anchor;
        window->AnchorMargin = Vec2(marginX, marginY);
        window->flags |= WidgetFlags_Floating_X | WidgetFlags_Floating_Y;
    }

    UI_Interaction UILabelCStr(const char* str, const char* text)
    {
        return UILabel(str, Str8C((char*)text));
    }

    static UI_Interaction LabelWithFlags(const char* str, const String8& text, u32 extraFlags)
    {
        u64 hash;
        HandleUIString(str, &hash);

        UI_Widget* widget = PushWidget(WidgetFlags_DrawText | extraFlags,
                                       text,
                                       hash,
                                       { SizeKind_TextContent, 1.0f },
                                       { SizeKind_TextContent, 1.0f });

        widget->style_vars[StyleVar_Padding] = Vec4(2.0f * s_UIState->DPIScale, 2.0f * s_UIState->DPIScale, 0.0f, 0.0f);
        // a hovered clickable lerps its border toward the theme's hot colour: a link has none
        if(extraFlags & WidgetFlags_Clickable)
        {
            widget->style_vars[StyleVar_BorderColor]       = Vec4(0.0f);
            widget->style_vars[StyleVar_HotBorderColor]    = Vec4(0.0f);
            widget->style_vars[StyleVar_ActiveBorderColor] = Vec4(0.0f);
        }

        return HandleWidgetInteraction(widget);
    }

    UI_Interaction UILabel(const char* str, const String8& text) { return LabelWithFlags(str, text, 0); }

    static String8 UIWrapText(const String8& text, f32 fontSize, f32 maxWidth)
    {
        Arena* arena = s_UIState->UIFrameArena;
        // Worst case: a '\n' inserted per codepoint (hard-breaking).
        u8* out       = PushArray(arena, u8, text.size * 2 + 1);
        u64 outLen    = 0;
        u64 lineStart = 0;

        auto lineWidth = [&](u64 from, u64 to) -> f32 {
            String8 s;
            s.str  = out + from;
            s.size = to - from;
            return GetStringSize(s, fontSize).x;
        };

        auto emitWord = [&](u64 from, u64 to) {
            u64 wordLen = to - from;
            memcpy(out + outLen, text.str + from, wordLen);
            if(lineWidth(lineStart, outLen + wordLen) <= maxWidth)
            {
                outLen += wordLen;
                return;
            }
            u64 ci = from;
            while(ci < to)
            {
                UnicodeDecode dec = Utf8Decode(text.str + ci, to - ci);
                u64 cl            = dec.inc > 0 ? (u64)dec.inc : 1;
                memcpy(out + outLen, text.str + ci, cl);
                if(outLen > lineStart && lineWidth(lineStart, outLen + cl) > maxWidth)
                {
                    out[outLen++] = '\n';
                    lineStart     = outLen;
                    memcpy(out + outLen, text.str + ci, cl);
                }
                outLen += cl;
                ci += cl;
            }
        };

        u64 i = 0;
        while(i < text.size)
        {
            u8 c = text.str[i];
            if(c == '\n')
            {
                out[outLen++] = '\n';
                lineStart     = outLen;
                i++;
                continue;
            }
            if(c == ' ')
            {
                i++;
                continue;
            }

            u64 wEnd = i;
            while(wEnd < text.size && text.str[wEnd] != ' ' && text.str[wEnd] != '\n')
                wEnd++;

            if(outLen > lineStart)
            {
                // Try " word" appended to the current line.
                u64 wordLen = wEnd - i;
                out[outLen] = ' ';
                memcpy(out + outLen + 1, text.str + i, wordLen);
                if(lineWidth(lineStart, outLen + 1 + wordLen) <= maxWidth)
                {
                    outLen += 1 + wordLen;
                    i = wEnd;
                    continue;
                }
                // Doesn't fit — wrap before the word.
                out[outLen++] = '\n';
                lineStart     = outLen;
            }
            emitWord(i, wEnd);
            i = wEnd;
        }

        String8 result;
        result.str  = out;
        result.size = outLen;
        return result;
    }

    UI_Interaction UILabelWrapped(const char* str, const String8& text, float maxWidth)
    {
        maxWidth *= s_UIState->DPIScale;
        const f32 fontSize = s_UIState->style_variable_lists[StyleVar_FontSize].last->value.x;

        if(GetStringSize(text, fontSize).x <= maxWidth)
            return UILabel(str, text);

        return UILabel(str, UIWrapText(text, fontSize, maxWidth));
    }

    // text cut to fit maxWidth (logical px) with "...", or the text itself when it fits
    static String8 EllipsisText(const String8& text, float maxWidth)
    {
        maxWidth *= s_UIState->DPIScale;
        const f32 fontSize = s_UIState->style_variable_lists[StyleVar_FontSize].last->value.x;

        if(GetStringSize(text, fontSize).x <= maxWidth)
            return text;

        const f32 budget = maxWidth - GetStringSize(Str8Lit("..."), fontSize).x;

        // Longest codepoint-aligned prefix that fits the budget: binary search over the codepoint ends
        // (measuring every prefix in turn was quadratic - long truncated labels cost ~1ms each on a phone)
        ArenaTemp scratch = ScratchBegin(nullptr, 0);
        u64* ends         = PushArrayNoZero(scratch.arena, u64, text.size);
        u64 count         = 0;
        for(u64 ci = 0; ci < text.size;)
        {
            UnicodeDecode dec = Utf8Decode(text.str + ci, text.size - ci);
            ci += dec.inc > 0 ? (u64)dec.inc : 1;
            ends[count++] = ci;
        }
        u64 fit = 0;
        i64 lo = 0, hi = (i64)count - 1;
        while(lo <= hi)
        {
            i64 mid = (lo + hi) / 2;
            String8 prefix;
            prefix.str  = text.str;
            prefix.size = ends[mid];
            if(GetStringSize(prefix, fontSize).x <= budget)
            {
                fit = ends[mid];
                lo  = mid + 1;
            }
            else
                hi = mid - 1;
        }
        ScratchEnd(scratch);

        return PushStr8F(s_UIState->UIFrameArena, "%.*s...", (int)fit, (const char*)text.str);
    }

    UI_Interaction UILabelEllipsis(const char* str, const String8& text, float maxWidth) { return UILabel(str, EllipsisText(text, maxWidth)); }

    // a label that reports clicks: no background or border, so it looks like any other text
    UI_Interaction UILabelLink(const char* str, const String8& text, float maxWidth)
    {
        return LabelWithFlags(str, maxWidth > 0.0f ? EllipsisText(text, maxWidth) : text, WidgetFlags_Clickable);
    }

    UI_Interaction UIButton(const char* str)
    {
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        UI_Widget* widget = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawText | WidgetFlags_DrawBorder | WidgetFlags_DrawBackground | WidgetFlags_AnimateScale,
                                       text,
                                       hash,
                                       { SizeKind_TextContent, 1.0f },
                                       { SizeKind_TextContent, 1.0f });

        float d = s_UIState->DPIScale;
        auto setIfDefault = [&](StyleVar v, const Vec4& val) {
            if(s_UIState->style_variable_lists[v].count == 0)
                widget->style_vars[v] = val;
        };
        setIfDefault(StyleVar_Padding,      Vec4(8.0f * d, 5.0f * d, 0.0f, 0.0f));
        setIfDefault(StyleVar_CornerRadius, Vec4(5.0f * d, 0.0f, 0.0f, 0.0f));
        widget->TextAlignment = UI_Text_Alignment_Center_X | UI_Text_Alignment_Center_Y;

        if(UIThemeIsDark())
        {
            setIfDefault(StyleVar_BackgroundColor,       Vec4(0.25f, 0.25f, 0.25f, 1.0f));
            setIfDefault(StyleVar_HotBackgroundColor,    Vec4(0.30f, 0.30f, 0.30f, 1.0f));
            setIfDefault(StyleVar_ActiveBackgroundColor, Vec4(0.35f, 0.35f, 0.35f, 1.0f));
            setIfDefault(StyleVar_BorderColor,           Vec4(0.45f, 0.45f, 0.45f, 1.0f));
            setIfDefault(StyleVar_HotBorderColor,        Vec4(0.5f, 0.65f, 0.9f, 1.0f));
            setIfDefault(StyleVar_ActiveBorderColor,     Vec4(0.4f, 0.55f, 0.85f, 1.0f));
        }
        else
        {
            setIfDefault(StyleVar_BackgroundColor,       Vec4(0.85f, 0.85f, 0.85f, 1.0f));
            setIfDefault(StyleVar_HotBackgroundColor,    Vec4(0.75f, 0.75f, 0.75f, 1.0f));
            setIfDefault(StyleVar_ActiveBackgroundColor, Vec4(0.65f, 0.65f, 0.65f, 1.0f));
            setIfDefault(StyleVar_BorderColor,           Vec4(0.6f, 0.6f, 0.6f, 1.0f));
            setIfDefault(StyleVar_HotBorderColor,        Vec4(0.5f, 0.65f, 0.9f, 1.0f));
            setIfDefault(StyleVar_ActiveBorderColor,     Vec4(0.4f, 0.55f, 0.85f, 1.0f));
        }

        return HandleWidgetInteraction(widget);
    }

    UI_Interaction UIImage(const char* str,
                           Graphics::Texture2D* texture,
                           Vec2 scale)
    {
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        // Missing texture (failed load) → small placeholder box, no crash.
        float w = texture ? texture->GetWidth() * scale.x : 16.0f * s_UIState->DPIScale;
        float h = texture ? texture->GetHeight() * scale.y : 16.0f * s_UIState->DPIScale;

        UI_Widget* widget = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBorder | WidgetFlags_DrawBackground,
                                       text,
                                       hash,
                                       { SizeKind_Pixels, w },
                                       { SizeKind_Pixels, h });
        widget->texture   = texture;
        return HandleWidgetInteraction(widget);
    }

    UI_Interaction UISlider(const char* str,
                            float* value,
                            float min_value,
                            float max_value,
                            float width,
                            float height,
                            float handleSizeFraction)
    {
        width *= s_UIState->DPIScale;
        height *= s_UIState->DPIScale;
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        String8 spacerText = PushStr8F(s_UIState->UIFrameArena, "spacer###spacer%s", (char*)text.str);
        u64 hashSpacer;
        String8 SpacerText2 = HandleUIString((char*)spacerText.str, &hashSpacer);

        UI_Widget* spacer = PushWidget(WidgetFlags_StackHorizontally,
                                       SpacerText2,
                                       hashSpacer,
                                       { SizeKind_ChildSum, 1.0f },
                                       { SizeKind_MaxChild, 1.0f });
        spacer->style_vars[StyleVar_Padding] = Vec4(0.0f, 3.0f * s_UIState->DPIScale, 0.0f, 0.0f);
        spacer->style_vars[StyleVar_ItemSpacing] = Vec4(0.0f);

        UI_Interaction slider_interaction = {};

        PushParent(spacer);
        {
            float lSliderWidth  = width;
            float lSliderHeight = height;
            String8 parentText  = PushStr8F(s_UIState->UIFrameArena, "parent###parent%s", (char*)text.str);
            UI_Widget* parent   = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBorder | WidgetFlags_DrawBackground | WidgetFlags_CentreY,
                                             text,
                                             HashUIStr8Name(parentText),
                                             { SizeKind_Pixels, lSliderWidth },
                                             { SizeKind_Pixels, lSliderHeight });

            // Enhanced slider track appearance - theme aware
            float dpi = s_UIState->DPIScale;
            parent->style_vars[StyleVar_Padding]      = Vec4(0.0f);
            parent->style_vars[StyleVar_CornerRadius] = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);

            if(UIThemeIsDark())
            {
                // Subtle opaque track (rounded to a capsule), no border.
                parent->style_vars[StyleVar_BackgroundColor] = Vec4(0.16f, 0.17f, 0.21f, 1.0f);
                parent->style_vars[StyleVar_BorderColor]     = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
                parent->style_vars[StyleVar_Border]          = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
                parent->style_vars[StyleVar_CornerRadius]    = Vec4(lSliderHeight * 0.5f, 0.0f, 0.0f, 0.0f);
            }
            else
            {
                parent->style_vars[StyleVar_BackgroundColor] = Vec4(0.7f, 0.7f, 0.7f, 1.0f);
                parent->style_vars[StyleVar_BorderColor]     = Vec4(0.5f, 0.5f, 0.5f, 1.0f);
            }

            UI_Interaction parent_interaction = HandleWidgetInteraction(parent);
            PushParent(parent);

            UI_Widget* slider                    = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBorder | WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Draggable | WidgetFlags_AnimateScale,
                                                              text,
                                                              hash,
                                                              { SizeKind_PercentOfParent, handleSizeFraction },
                                                              { SizeKind_PercentOfParent, 1.0f });

            // Enhanced slider handle appearance - theme aware
            slider->style_vars[StyleVar_Border]       = Vec4(1.0f * dpi, 1.0f * dpi, 0.0f, 0.0f);
            slider->style_vars[StyleVar_Padding]      = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
            slider->style_vars[StyleVar_CornerRadius] = Vec4(lSliderHeight * 0.5f, 0.0f, 0.0f, 0.0f);

            if(UIThemeIsDark())
            {
                // Glowing white handle (soft blue-white bloom via the shadow blur).
                slider->style_vars[StyleVar_BorderColor]           = Vec4(1.0f, 1.0f, 1.0f, 0.9f);
                slider->style_vars[StyleVar_BackgroundColor]       = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
                slider->style_vars[StyleVar_HotBorderColor]        = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
                slider->style_vars[StyleVar_HotBackgroundColor]    = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
                slider->style_vars[StyleVar_ActiveBorderColor]     = Vec4(0.62f, 0.71f, 1.0f, 1.0f);
                slider->style_vars[StyleVar_ActiveBackgroundColor] = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
                slider->style_vars[StyleVar_ShadowColor]           = Vec4(0.62f, 0.71f, 1.0f, 0.55f);
                slider->style_vars[StyleVar_ShadowOffset]          = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
                slider->style_vars[StyleVar_ShadowBlur]            = Vec4(6.0f * dpi, 0.0f, 0.0f, 0.0f);
            }
            else
            {
                slider->style_vars[StyleVar_BorderColor]           = Vec4(0.3f, 0.3f, 0.3f, 1.0f);
                slider->style_vars[StyleVar_BackgroundColor]       = Vec4(0.9f, 0.9f, 0.9f, 1.0f);
                slider->style_vars[StyleVar_HotBorderColor]        = Vec4(0.4f, 0.6f, 1.0f, 1.0f);
                slider->style_vars[StyleVar_HotBackgroundColor]    = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
                slider->style_vars[StyleVar_ActiveBorderColor]     = Vec4(0.2f, 0.4f, 0.9f, 1.0f);
                slider->style_vars[StyleVar_ActiveBackgroundColor] = Vec4(0.95f, 0.95f, 0.95f, 1.0f);
            }
            slider_interaction                             = HandleWidgetInteraction(slider);

            PopParent(parent);

            slider->drag_constraint_y = true;

            // Clamp input value first
            *value = Maths::Clamp(*value, min_value, max_value);

            // Use parent size and fraction to compute slider width
            f32 parent_x      = parent->position.x;
            f32 parent_size_x = parent->size.x;
            f32 slider_size_x = parent_size_x * handleSizeFraction;


            // Helper: get mouse in UI coords
            Vec2 mouse = Input::Get().GetMousePosition() * s_UIState->InputScale - s_UIState->InputOffset;

            // If clicked on the parent track, set value based on mouse position
            if(parent_interaction.clicked)
            {
                f32 t  = (mouse.x - parent_x - slider_size_x * 0.5f) / (parent_size_x - slider_size_x);
                t      = Maths::Clamp(t, 0.0f, 1.0f);
                *value = min_value + (max_value - min_value) * t;
                slider_interaction.clicked = true;
            }

            // Compute t from value, unless dragging, in which case use mouse pos
            f32 t = (*value - min_value) / (max_value - min_value);
            if(slider->dragging)
            {
                Vec2 dragMouse = Input::Get().GetMousePosition() * s_UIState->InputScale - s_UIState->InputOffset;
                t              = (dragMouse.x - parent_x - slider_size_x * 0.5f) / (parent_size_x - slider_size_x);
            }

            // Final clamp and apply
            t      = Maths::Clamp(t, 0.0f, 1.0f);
            *value = Maths::Clamp(min_value + (max_value - min_value) * t, min_value, max_value);

            // Place the slider so it stays within the parent (relative position)
            slider->relative_position[UIAxis_X] = t * (parent_size_x - slider_size_x);

            String8 slider_text  = PushStr8F(s_UIState->UIFrameArena, "  %.*s: %.2f###slval", (int)text.size, (const char*)text.str, *value);
            u64 labelHash;
            String8 slider_label = HandleUIString((char*)slider_text.str, &labelHash);
            UI_Widget* widget    = PushWidget(WidgetFlags_DrawText | WidgetFlags_CentreY,
                                              slider_label,
                                              labelHash,
                                              { SizeKind_TextContent, 1.0f },
                                              { SizeKind_TextContent, 1.0f });
            UIAdoptText(widget, slider_label);
        }
        PopParent(spacer);

        return slider_interaction;
    }

    UI_Interaction UIToggle(const char* str,
                            bool* value)
    {
        UI_Interaction interaction = {};

        u64 hash;
        String8 text = HandleUIString(str, &hash);

        String8 spacerText = PushStr8F(s_UIState->UIFrameArena, "spacer###spacer%s", (char*)text.str);
        u64 hashSpacer;
        String8 SpacerText2 = HandleUIString((char*)spacerText.str, &hashSpacer);

        // Full-width row so the switch can pin to the right edge (label left).
        UI_Widget* spacer = PushWidget(WidgetFlags_StackHorizontally,
                                       SpacerText2,
                                       hashSpacer,
                                       { SizeKind_PercentOfParent, 1.0f },
                                       { SizeKind_ChildSum, 1.0f });
        spacer->style_vars[StyleVar_Padding] = Vec4(0.0f, 3.0f * s_UIState->DPIScale, 0.0f, 0.0f);
        spacer->style_vars[StyleVar_ItemSpacing] = Vec4(0.0f);
        PushParent(spacer);
        {
            float dpi        = s_UIState->DPIScale;
            float fontSize   = spacer->style_vars[StyleVar_FontSize].x;
            float textHeight = GetStringSize(Str8Lit("A"), fontSize).y;
            float trackWidth = textHeight * 1.9f;
            float trackHeight = textHeight;
            // finger-sized on touch screens
            if(s_UIState->ClickOnRelease && trackHeight < 26.0f * dpi)
            {
                trackHeight = 26.0f * dpi;
                trackWidth  = trackHeight * 1.65f;
                spacer->style_vars[StyleVar_Padding] = Vec4(0.0f, 6.0f * dpi, 0.0f, 0.0f);
            }
            float knobSize   = trackHeight - 4.0f * dpi;

            // Label first (left).
            String8 labelText = PushStr8F(s_UIState->UIFrameArena, "label###label%s", (char*)text.str);
            UI_Widget* label  = PushWidget(WidgetFlags_DrawText | WidgetFlags_CentreY,
                                           text,
                                           HashUIStr8Name(labelText),
                                           { SizeKind_TextContent, 1.0f },
                                           { SizeKind_TextContent, 1.0f });

            // Toggle track (background pill) pinned to the right edge.
            UI_Widget* toggle_track = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Clickable | WidgetFlags_CentreY | WidgetFlags_IsToggle | WidgetFlags_AlignRight,
                                                 text,
                                                 hash,
                                                 { SizeKind_Pixels, trackWidth },
                                                 { SizeKind_Pixels, trackHeight });

            toggle_track->style_vars[StyleVar_CornerRadius] = Vec4(trackHeight * 0.5f, 0.0f, 0.0f, 0.0f);
            toggle_track->style_vars[StyleVar_Border]       = Vec4(0.0f, 0.0f, 0.0f, 0.0f);

            interaction = HandleWidgetInteraction(toggle_track);
            if(interaction.clicked)
            {
                *value = !(*value);
            }

            // Get current animated position - animate towards target
            float target = *value ? 1.0f : 0.0f;
            if(toggle_track->FirstFrame)
                toggle_track->ToggleTransition = target;
            float speed = s_UIState->AnimationRateDT * 1.5f;
            if(toggle_track->ToggleTransition < target)
                toggle_track->ToggleTransition = Maths::Min(toggle_track->ToggleTransition + speed, target);
            else if(toggle_track->ToggleTransition > target)
                toggle_track->ToggleTransition = Maths::Max(toggle_track->ToggleTransition - speed, target);

            float t      = toggle_track->ToggleTransition;
            float easedT = EaseInOutCubic(t);

            // Interpolate track colors based on animation (green = on).
            Vec4 offColor, onColor;
            if(UIThemeIsDark())
            {
                offColor = Vec4(1.0f, 1.0f, 1.0f, 0.14f);
                onColor  = Vec4(0.37f, 0.82f, 0.54f, 1.0f);
            }
            else
            {
                offColor = Vec4(0.75f, 0.75f, 0.75f, 1.0f);
                onColor  = Vec4(0.32f, 0.78f, 0.5f, 1.0f);
            }
            if(s_UIState->AccentColour.w > 0.0f)
                onColor = Vec4(s_UIState->AccentColour.x, s_UIState->AccentColour.y, s_UIState->AccentColour.z, 1.0f);
            toggle_track->style_vars[StyleVar_BackgroundColor] = offColor.Lerp(onColor, easedT);

            // Toggle knob (sliding circle)
            String8 knobText = PushStr8F(s_UIState->UIFrameArena, "knob###knob%s", (char*)text.str);
            u64 knobHash;
            HandleUIString((char*)knobText.str, &knobHash);

            PushParent(toggle_track);
            UI_Widget* knob = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                         Str8Lit(""),
                                         knobHash,
                                         { SizeKind_Pixels, knobSize },
                                         { SizeKind_Pixels, knobSize });

            // Knob position: animate from left to right.
            knob->style_vars[StyleVar_Padding] = Vec4(0.0f);
            float knobPadding = 2.0f * dpi;
            float knobMinX = knobPadding;
            float knobMaxX = trackWidth - knobSize - knobPadding;
            float knobX = knobMinX + (knobMaxX - knobMinX) * easedT;
            float knobY = (trackHeight - knobSize) * 0.5f;
            knob->relative_position = Vec2(knobX, knobY);

            knob->style_vars[StyleVar_CornerRadius] = Vec4(knobSize * 0.5f, 0.0f, 0.0f, 0.0f);
            knob->style_vars[StyleVar_BackgroundColor] = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            knob->style_vars[StyleVar_ShadowColor] = Vec4(0.0f, 0.0f, 0.0f, 0.2f);
            knob->style_vars[StyleVar_ShadowOffset] = Vec4(0.0f, 1.0f * s_UIState->DPIScale, 0.0f, 0.0f);
            knob->style_vars[StyleVar_ShadowBlur] = Vec4(2.0f * s_UIState->DPIScale, 0.0f, 0.0f, 0.0f);
            PopParent(toggle_track);
        }

        PopParent(spacer);

        return interaction;
    }

    UI_Interaction UIColouredBox(const char* str, float width, float height,
                                 const Vec4& colour, float cornerRadius)
    {
        float dpi = s_UIState->DPIScale;
        u64 hash;
        String8 text     = HandleUIString(str, &hash);
        UI_Widget* box   = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_CentreY,
                                      text, hash,
                                      { SizeKind_Pixels, width * dpi },
                                      { SizeKind_Pixels, height * dpi });
        box->style_vars[StyleVar_BackgroundColor] = colour;
        box->style_vars[StyleVar_Border]          = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        box->style_vars[StyleVar_CornerRadius]    = Vec4(cornerRadius * dpi, 0.0f, 0.0f, 0.0f);
        return HandleWidgetInteraction(box);
    }

    static UI_Widget* PushFreeShape(const char* str, float x, float y, float width, float height, const Vec4& colour)
    {
        const float dpi = s_UIState->DPIScale;
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        // child of the root so the position is in screen space whatever panel is being built
        PushParent(&s_UIState->root_parent);
        UI_Widget* shape = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                      text, hash,
                                      { SizeKind_Pixels, width * dpi },
                                      { SizeKind_Pixels, height * dpi });
        PopParent(&s_UIState->root_parent);

        shape->relative_position                    = Vec2(x * dpi, y * dpi);
        shape->style_vars[StyleVar_BackgroundColor] = colour;
        shape->style_vars[StyleVar_BorderColor]     = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        shape->style_vars[StyleVar_Border]          = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        shape->style_vars[StyleVar_Padding]         = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        return shape;
    }

    void UIRect(const char* str, float x, float y, float width, float height, const Vec4& colour, float cornerRadius)
    {
        UI_Widget* rect                         = PushFreeShape(str, x, y, width, height, colour);
        rect->style_vars[StyleVar_CornerRadius] = Vec4(Maths::Max(cornerRadius, 0.0f) * s_UIState->DPIScale, 0.0f, 0.0f, 0.0f);
    }

    void UIRing(const char* str, float x, float y, float radius, const Vec4& colour, float thickness)
    {
        UI_Widget* ring = PushFreeShape(str, x - radius, y - radius, radius * 2.0f, radius * 2.0f, colour);
        // a negative corner radius draws an outline that many pixels thick
        ring->style_vars[StyleVar_CornerRadius] = Vec4(-Maths::Max(1.0f, thickness * s_UIState->DPIScale), 0.0f, 0.0f, 0.0f);
    }

    UI_Interaction UIBeginRadar(const char* str, float diameter,
                                const Vec4& bgColour, const Vec4& ringColour)
    {
        float dpi = s_UIState->DPIScale;
        diameter *= dpi;

        u64 hash;
        String8 text = HandleUIString(str, &hash);

        UI_Widget* radar = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawBorder,
                                      text, hash,
                                      { SizeKind_Pixels, diameter },
                                      { SizeKind_Pixels, diameter });

        radar->style_vars[StyleVar_BackgroundColor] = bgColour;
        radar->style_vars[StyleVar_BorderColor]     = ringColour;
        radar->style_vars[StyleVar_Border]          = Vec4(2.0f * dpi, 2.0f * dpi, 0.0f, 0.0f);
        radar->style_vars[StyleVar_CornerRadius]    = Vec4(diameter * 0.5f, 0.0f, 0.0f, 0.0f);
        radar->style_vars[StyleVar_Padding]         = Vec4(0.0f, 0.0f, 0.0f, 0.0f);

        UI_Interaction interaction = HandleWidgetInteraction(radar);
        PushParent(radar);
        return interaction;
    }

    void UIEndRadar()
    {
        UI_Widget* radar = GetCurrentParent();
        PopParent(radar);
    }

    UI_Interaction UIRadarBlip(const char* str, float x, float y, float size,
                               const Vec4& tint, Graphics::Texture2D* texture)
    {
        float dpi = s_UIState->DPIScale;
        x    *= dpi;
        y    *= dpi;
        size *= dpi;

        UI_Widget* radar   = GetCurrentParent();
        float radarRadius  = radar->semantic_size[UIAxis_X].value * 0.5f;

        u64 hash;
        String8 text = HandleUIString(str, &hash);

        UI_Widget* blip = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Clickable | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                     text, hash,
                                     { SizeKind_Pixels, size },
                                     { SizeKind_Pixels, size });

        blip->relative_position                    = Vec2(radarRadius + x - size * 0.5f, radarRadius + y - size * 0.5f);
        blip->style_vars[StyleVar_BackgroundColor] = tint;
        blip->style_vars[StyleVar_Border]          = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        blip->style_vars[StyleVar_CornerRadius]    = Vec4(texture ? 0.0f : size * 0.5f, 0.0f, 0.0f, 0.0f);
        blip->texture                              = texture;

        return HandleWidgetInteraction(blip);
    }

    void UIRadarRing(const char* str, float radius, const Vec4& colour, float thickness)
    {
        float dpi = s_UIState->DPIScale;
        radius *= dpi;
        thickness = Maths::Max(1.0f, thickness * dpi);

        UI_Widget* radar  = GetCurrentParent();
        float radarRadius = radar->semantic_size[UIAxis_X].value * 0.5f;

        u64 hash;
        String8 text = HandleUIString(str, &hash);

        UI_Widget* ring = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                     text, hash,
                                     { SizeKind_Pixels, radius * 2.0f },
                                     { SizeKind_Pixels, radius * 2.0f });

        ring->relative_position                    = Vec2(radarRadius - radius, radarRadius - radius);
        ring->style_vars[StyleVar_BackgroundColor] = colour;
        ring->style_vars[StyleVar_BorderColor]     = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        ring->style_vars[StyleVar_Border]          = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        ring->style_vars[StyleVar_CornerRadius]    = Vec4(-thickness, 0.0f, 0.0f, 0.0f);
        ring->style_vars[StyleVar_Padding]         = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    UI_Interaction UISliderRow(const char* str, float* value,
                               float min_value, float max_value, const char* valueFmt)
    {
        float dpi = s_UIState->DPIScale;
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        const float trackHeight = 5.0f * dpi;
        const float handleD     = 15.0f * dpi;

        UI_Interaction interaction = {};

        // Outer vertical container, full width.
        String8 outerText = PushStr8F(s_UIState->UIFrameArena, "srouter###srouter%s", (char*)text.str);
        UI_Widget* outer  = PushWidget(WidgetFlags_StackVertically,
                                       outerText, HashUIStr8Name(outerText),
                                       { SizeKind_PercentOfParent, 1.0f },
                                       { SizeKind_ChildSum, 1.0f });
        outer->style_vars[StyleVar_Padding]     = Vec4(0.0f, 3.0f * s_UIState->DPIScale, 0.0f, 0.0f);
        outer->style_vars[StyleVar_ItemSpacing] = Vec4(0.0f, 4.0f * dpi, 0.0f, 0.0f);
        PushParent(outer);
        {
            // Header row: label (left) + value (right).
            String8 headText = PushStr8F(s_UIState->UIFrameArena, "srhead###srhead%s", (char*)text.str);
            UI_Widget* head  = PushWidget(WidgetFlags_StackHorizontally,
                                          headText, HashUIStr8Name(headText),
                                          { SizeKind_PercentOfParent, 1.0f },
                                          { SizeKind_MaxChild, 1.0f });
            head->style_vars[StyleVar_Padding]     = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
            head->style_vars[StyleVar_ItemSpacing] = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
            PushParent(head);
            {
                String8 lblText = PushStr8F(s_UIState->UIFrameArena, "srlbl###srlbl%s", (char*)text.str);
                PushWidget(WidgetFlags_DrawText, text, HashUIStr8Name(lblText),
                           { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });

                String8 valStr  = PushStr8F(s_UIState->UIFrameArena, valueFmt, *value);
                String8 valText = PushStr8F(s_UIState->UIFrameArena, "srval###srval%s", (char*)text.str);
                UI_Widget* val  = PushWidget(WidgetFlags_DrawText | WidgetFlags_AlignRight,
                                             valStr, HashUIStr8Name(valText),
                                             { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
                UIAdoptText(val, valStr);
            }
            PopParent(head);

            // Track row (holds the full-width track; handle floats over it).
            String8 trackText = PushStr8F(s_UIState->UIFrameArena, "srtrackrow###srtrackrow%s", (char*)text.str);
            UI_Widget* trackRow = PushWidget(WidgetFlags_StackHorizontally,
                                             trackText, HashUIStr8Name(trackText),
                                             { SizeKind_PercentOfParent, 1.0f },
                                             { SizeKind_Pixels, handleD });
            trackRow->style_vars[StyleVar_Padding]     = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
            trackRow->style_vars[StyleVar_ItemSpacing] = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
            PushParent(trackRow);
            {
                String8 tkText  = PushStr8F(s_UIState->UIFrameArena, "srtrack###srtrack%s", (char*)text.str);
                UI_Widget* track = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Clickable | WidgetFlags_CentreY,
                                              tkText, HashUIStr8Name(tkText),
                                              { SizeKind_PercentOfParent, 1.0f },
                                              { SizeKind_Pixels, trackHeight });
                track->style_vars[StyleVar_BackgroundColor] = Vec4(0.16f, 0.17f, 0.21f, 1.0f);
                track->style_vars[StyleVar_Border]          = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
                track->style_vars[StyleVar_Padding]         = Vec4(0.0f);
                track->style_vars[StyleVar_CornerRadius]    = Vec4(trackHeight * 0.5f, 0.0f, 0.0f, 0.0f);

                UI_Interaction trackInteraction = HandleWidgetInteraction(track);

                PushParent(track);
                UI_Widget* handle = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | WidgetFlags_Draggable,
                                               tkText, hash,
                                               { SizeKind_Pixels, handleD },
                                               { SizeKind_Pixels, handleD });
                handle->style_vars[StyleVar_CornerRadius]    = Vec4(handleD * 0.5f, 0.0f, 0.0f, 0.0f);
                handle->style_vars[StyleVar_Border]          = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
                handle->style_vars[StyleVar_BackgroundColor] = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
                handle->style_vars[StyleVar_ShadowColor]     = Vec4(0.62f, 0.71f, 1.0f, 0.55f);
                handle->style_vars[StyleVar_ShadowOffset]    = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
                handle->style_vars[StyleVar_ShadowBlur]      = Vec4(6.0f * dpi, 0.0f, 0.0f, 0.0f);
                interaction = HandleWidgetInteraction(handle);
                handle->drag_constraint_y = true;
                PopParent(track);

                // Positions use last frame's resolved track size (immediate-mode lag).
                f32 parent_x      = track->position.x;
                f32 parent_size_x = track->size.x;
                f32 handle_w      = handleD;

                *value = Maths::Clamp(*value, min_value, max_value);
                Vec2 mouse = Input::Get().GetMousePosition() * s_UIState->InputScale - s_UIState->InputOffset;

                if(trackInteraction.clicked)
                {
                    f32 tt = (mouse.x - parent_x - handle_w * 0.5f) / Maths::Max(1.0f, parent_size_x - handle_w);
                    tt     = Maths::Clamp(tt, 0.0f, 1.0f);
                    *value = min_value + (max_value - min_value) * tt;
                    interaction.clicked = true;
                }

                f32 t = (*value - min_value) / (max_value - min_value);
                if(handle->dragging)
                    t = (mouse.x - parent_x - handle_w * 0.5f) / Maths::Max(1.0f, parent_size_x - handle_w);
                t      = Maths::Clamp(t, 0.0f, 1.0f);
                *value = Maths::Clamp(min_value + (max_value - min_value) * t, min_value, max_value);

                handle->relative_position[UIAxis_X] = t * (parent_size_x - handle_w);
                handle->relative_position[UIAxis_Y] = (trackHeight - handleD) * 0.5f;
            }
            PopParent(trackRow);
        }
        PopParent(outer);

        return interaction;
    }

    UI_Interaction UICheckbox(const char* str, bool* value)
    {
        UI_Interaction interaction = {};

        u64 hash;
        String8 text = HandleUIString(str, &hash);

        String8 rowText = PushStr8F(s_UIState->UIFrameArena, "cbrow###cbrow%s", (char*)text.str);
        u64 hashRow;
        String8 rowText2 = HandleUIString((char*)rowText.str, &hashRow);

        UI_Widget* row = PushWidget(WidgetFlags_StackHorizontally,
                                    rowText2,
                                    hashRow,
                                    { SizeKind_ChildSum, 1.0f },
                                    { SizeKind_ChildSum, 1.0f });
        row->style_vars[StyleVar_Padding] = Vec4(0.0f, 3.0f * s_UIState->DPIScale, 0.0f, 0.0f);
        row->style_vars[StyleVar_ItemSpacing] = Vec4(0.0f);
        PushParent(row);
        {
            float fontSize   = row->style_vars[StyleVar_FontSize].x;
            float textHeight = GetStringSize(Str8Lit("A"), fontSize).y;
            float boxSize    = textHeight;

            String8 boxText = PushStr8F(s_UIState->UIFrameArena, "cbbox###cbbox%s", (char*)text.str);
            u64 boxHash;
            HandleUIString((char*)boxText.str, &boxHash);

            UI_Widget* box = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_CentreY,
                                        Str8Lit(""),
                                        boxHash,
                                        { SizeKind_Pixels, boxSize },
                                        { SizeKind_Pixels, boxSize });

            float dp = s_UIState->DPIScale;
            box->style_vars[StyleVar_CornerRadius] = Vec4(3.0f * dp, 0.0f, 0.0f, 0.0f);
            box->style_vars[StyleVar_Border]       = Vec4(1.0f * dp, 1.0f * dp, 0.0f, 0.0f);
            box->style_vars[StyleVar_Padding]      = Vec4(0.0f);

            interaction = HandleWidgetInteraction(box);
            if(interaction.clicked)
                *value = !(*value);

            if(*value)
            {
                box->style_vars[StyleVar_BackgroundColor] = Vec4(0.3f, 0.6f, 0.95f, 1.0f);
                box->style_vars[StyleVar_BorderColor]     = Vec4(0.25f, 0.5f, 0.9f, 1.0f);
            }
            else
            {
                if(UIThemeIsDark())
                {
                    box->style_vars[StyleVar_BackgroundColor] = Vec4(0.2f, 0.2f, 0.2f, 1.0f);
                    box->style_vars[StyleVar_BorderColor]     = Vec4(0.4f, 0.4f, 0.4f, 1.0f);
                }
                else
                {
                    box->style_vars[StyleVar_BackgroundColor] = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
                    box->style_vars[StyleVar_BorderColor]     = Vec4(0.7f, 0.7f, 0.7f, 1.0f);
                }
            }

            // Label
            String8 labelText = PushStr8F(s_UIState->UIFrameArena, "cblbl###cblbl%s", (char*)text.str);
            UI_Widget* label  = PushWidget(WidgetFlags_DrawText | WidgetFlags_CentreY,
                                           text,
                                           HashUIStr8Name(labelText),
                                           { SizeKind_TextContent, 1.0f },
                                           { SizeKind_TextContent, 1.0f });
        }
        PopParent(row);

        return interaction;
    }

    UI_Interaction UIImageWarp(const char* str, Graphics::Texture2D* texture, const Vec2& pos, const Vec2& size,
                               const Vec2 corners[4], float depthLeft, float depthRight,
                               const Vec4& topColour, const Vec4& bottomColour, bool flipV, bool clickable, float cornerRadius, bool vertical)
    {
        const float dpi = s_UIState->DPIScale;
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        UI_Widget* widget = PushWidget((clickable ? WidgetFlags_Clickable : 0) | WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                       Str8Lit(""),
                                       hash,
                                       { SizeKind_Pixels, Maths::Max(1.0f, size.x * dpi) },
                                       { SizeKind_Pixels, Maths::Max(1.0f, size.y * dpi) });
        (void)text;
        widget->relative_position = pos * dpi;
        widget->texture           = texture;
        for(int i = 0; i < 4; i++)
            widget->WarpCorners[i] = corners[i] * dpi;
        widget->WarpDepth[0]     = Maths::Max(0.0001f, depthLeft);
        widget->WarpDepth[1]     = Maths::Max(0.0001f, depthRight);
        widget->WarpColourBottom = bottomColour;
        widget->WarpFlipV        = flipV;
        widget->WarpVertical     = vertical;
        // more strips when the edges differ a lot in depth
        float ratio        = Maths::Max(depthLeft, depthRight) / Maths::Max(0.0001f, Maths::Min(depthLeft, depthRight));
        widget->WarpStrips = (u8)Maths::Clamp((int)(ratio * 8.0f), 1, 32);

        // colours set on the widget directly: hovered clickables otherwise take the theme's hot fill
        widget->style_vars[StyleVar_BackgroundColor]       = topColour;
        widget->style_vars[StyleVar_HotBackgroundColor]    = topColour;
        widget->style_vars[StyleVar_ActiveBackgroundColor] = topColour;
        widget->style_vars[StyleVar_Border]                = Vec4(0.0f);
        widget->style_vars[StyleVar_BorderColor]           = Vec4(0.0f);
        widget->style_vars[StyleVar_HotBorderColor]        = Vec4(0.0f);
        widget->style_vars[StyleVar_ActiveBorderColor]     = Vec4(0.0f);
        widget->style_vars[StyleVar_ShadowColor]           = Vec4(0.0f);
        widget->style_vars[StyleVar_CornerRadius]          = Vec4(cornerRadius * dpi);
        widget->style_vars[StyleVar_Padding]               = Vec4(0.0f);
        return HandleWidgetInteraction(widget);
    }

    UI_Interaction UIScrubBar(const char* str, float* value, float width, float height,
                              const Vec4& fillColour, const Vec4& trackColour, bool* active)
    {
        static u64 s_ActiveScrub = 0;
        const float dpi          = s_UIState->DPIScale;
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        // Hit area is taller than the visible bar so it's easy to grab
        const float hitH = Maths::Max(height * 3.0f, 12.0f) * dpi;
        UI_Widget* area  = PushWidget(WidgetFlags_Clickable | WidgetFlags_CentreY | WidgetFlags_StackHorizontally,
                                      text,
                                      hash,
                                      { SizeKind_Pixels, width * dpi },
                                      { SizeKind_Pixels, hitH });
        area->style_vars[StyleVar_Padding]           = Vec4(0.0f);
        area->style_vars[StyleVar_Border]            = Vec4(0.0f);
        area->style_vars[StyleVar_BorderColor]       = Vec4(0.0f);
        area->style_vars[StyleVar_HotBorderColor]    = Vec4(0.0f);
        area->style_vars[StyleVar_ActiveBorderColor] = Vec4(0.0f);
        area->style_vars[StyleVar_ShadowColor]       = Vec4(0.0f);
        UI_Interaction interaction                   = HandleWidgetInteraction(area);

        Vec2 mouse = Input::Get().GetMousePosition() * s_UIState->InputScale - s_UIState->InputOffset;
        if(interaction.hovering && Input::Get().GetMouseClicked(InputCode::MouseKey::ButtonLeft))
            s_ActiveScrub = area->hash;
        bool dragging = s_ActiveScrub == area->hash;
        if(dragging && !Input::Get().GetMouseHeld(InputCode::MouseKey::ButtonLeft))
        {
            s_ActiveScrub = 0;
            dragging      = false;
        }
        if(dragging && area->size.x > 0.0f)
            *value = Maths::Clamp((mouse.x - area->position.x) / area->size.x, 0.0f, 1.0f);
        if(active)
            *active = dragging;

        const float t     = Maths::Clamp(*value, 0.0f, 1.0f);
        const float barH  = height * dpi;
        const bool emph   = dragging || interaction.hovering;
        const float drawH = emph ? barH * 1.5f : barH;

        PushParent(area);
        {
            UI_Widget* bar = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_CentreY | WidgetFlags_StackHorizontally,
                                        Str8Lit(""),
                                        UIHashCombine(area->hash, 0x5343525542ull),
                                        { SizeKind_PercentOfParent, 1.0f },
                                        { SizeKind_Pixels, drawH });
            bar->style_vars[StyleVar_Padding]         = Vec4(0.0f);
            bar->style_vars[StyleVar_Border]          = Vec4(0.0f);
            bar->style_vars[StyleVar_BackgroundColor] = trackColour;
            bar->style_vars[StyleVar_CornerRadius]    = Vec4(drawH * 0.5f, 0.0f, 0.0f, 0.0f);
            bar->style_vars[StyleVar_ShadowColor]     = Vec4(0.0f);

            PushParent(bar);
            UI_Widget* fill = PushWidget(t > 0.0f ? WidgetFlags_DrawBackground : 0,
                                         Str8Lit(""),
                                         UIHashCombine(area->hash, 0x46494C4Cull),
                                         { SizeKind_PercentOfParent, Maths::Max(t, 0.0001f) },
                                         { SizeKind_PercentOfParent, 1.0f });
            fill->style_vars[StyleVar_Padding]         = Vec4(0.0f);
            fill->style_vars[StyleVar_Border]          = Vec4(0.0f);
            fill->style_vars[StyleVar_BackgroundColor] = fillColour;
            fill->style_vars[StyleVar_CornerRadius]    = Vec4(drawH * 0.5f, 0.0f, 0.0f, 0.0f);
            fill->style_vars[StyleVar_ShadowColor]     = Vec4(0.0f);
            PopParent(bar);

            if(emph)
            {
                const float knob = barH * 4.0f;
                UI_Widget* k     = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                              Str8Lit(""),
                                              UIHashCombine(area->hash, 0x4B4E4F42ull),
                                              { SizeKind_Pixels, knob },
                                              { SizeKind_Pixels, knob });
                k->relative_position                     = Vec2(t * area->size.x - knob * 0.5f, (hitH - knob) * 0.5f);
                k->style_vars[StyleVar_Padding]          = Vec4(0.0f);
                k->style_vars[StyleVar_Border]           = Vec4(0.0f);
                k->style_vars[StyleVar_BackgroundColor]  = Vec4(1.0f);
                k->style_vars[StyleVar_CornerRadius]     = Vec4(knob * 0.5f, 0.0f, 0.0f, 0.0f);
                k->style_vars[StyleVar_ShadowColor]      = Vec4(0.0f, 0.0f, 0.0f, 0.35f);
            }
        }
        PopParent(area);
        return interaction;
    }

    UI_Interaction UIProgressBar(const char* str,
                                  float progress,
                                  float width,
                                  float height)
    {
        width *= s_UIState->DPIScale;
        height *= s_UIState->DPIScale;
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        progress = Maths::Clamp(progress, 0.0f, 1.0f);

        // Outer row for label + bar
        String8 rowText = PushStr8F(s_UIState->UIFrameArena, "row###row%s", (char*)text.str);
        u64 hashRow;
        HandleUIString((char*)rowText.str, &hashRow);

        UI_Widget* row = PushWidget(WidgetFlags_StackHorizontally,
                                    text,
                                    hashRow,
                                    { SizeKind_ChildSum, 1.0f },
                                    { SizeKind_MaxChild, 1.0f });
        row->style_vars[StyleVar_Padding] = Vec4(0.0f, 3.0f * s_UIState->DPIScale, 0.0f, 0.0f);
        row->style_vars[StyleVar_ItemSpacing] = Vec4(0.0f);
        PushParent(row);

        if(text.size > 0)
        {
            String8 labelText = PushStr8F(s_UIState->UIFrameArena, "label###label%s", (char*)text.str);
            u64 hashLabel;
            HandleUIString((char*)labelText.str, &hashLabel);
            PushWidget(WidgetFlags_DrawText | WidgetFlags_CentreY,
                       text,
                       hashLabel,
                       { SizeKind_TextContent, 1.0f },
                       { SizeKind_TextContent, 1.0f });
        }

        // Bar container
        String8 containerText = PushStr8F(s_UIState->UIFrameArena, "container###container%s", (char*)text.str);
        u64 hashContainer;
        HandleUIString((char*)containerText.str, &hashContainer);

        UI_Widget* container = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_CentreY,
                                          Str8Lit(""),
                                          hashContainer,
                                          { SizeKind_Pixels, width },
                                          { SizeKind_Pixels, height });

        {float dp = s_UIState->DPIScale;
        container->style_vars[StyleVar_CornerRadius] = Vec4(4.0f * dp, 0.0f, 0.0f, 0.0f);
        container->style_vars[StyleVar_Padding]      = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        container->style_vars[StyleVar_Border]       = Vec4(1.0f * dp, 1.0f * dp, 0.0f, 0.0f);}

        if(UIThemeIsDark())
        {
            container->style_vars[StyleVar_BackgroundColor] = Vec4(0.15f, 0.15f, 0.15f, 1.0f);
            container->style_vars[StyleVar_BorderColor]     = Vec4(0.35f, 0.35f, 0.35f, 1.0f);
        }
        else
        {
            container->style_vars[StyleVar_BackgroundColor] = Vec4(0.75f, 0.75f, 0.75f, 1.0f);
            container->style_vars[StyleVar_BorderColor]     = Vec4(0.5f, 0.5f, 0.5f, 1.0f);
        }

        PushParent(container);

        // Fill bar - use pixels based on progress
        float fillWidth = width * progress;
        UI_Widget* fill = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_CentreY,
                                     Str8Lit(""),
                                     hash,
                                     { SizeKind_Pixels, fillWidth },
                                     { SizeKind_Pixels, height });

        fill->style_vars[StyleVar_CornerRadius]    = Vec4(3.0f * s_UIState->DPIScale, 0.0f, 0.0f, 0.0f);
        fill->style_vars[StyleVar_Padding]         = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        fill->style_vars[StyleVar_Border]          = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        {
            Vec4 fillCol = fill->style_vars[StyleVar_ActiveBackgroundColor];
            bool isThemeDefault = s_UIState->style_variable_lists[StyleVar_ActiveBackgroundColor].count == 0;
            fill->style_vars[StyleVar_BackgroundColor] = isThemeDefault ? Vec4(0.3f, 0.6f, 0.95f, 1.0f) : fillCol;
        }

        PopParent(container);
        PopParent(row);

        return HandleWidgetInteraction(container);
    }

    UI_Interaction UISliderInt(const char* str,
                                int* value,
                                int min_value,
                                int max_value,
                                float width,
                                float height)
    {
        width *= s_UIState->DPIScale;
        height *= s_UIState->DPIScale;
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        String8 spacerText = PushStr8F(s_UIState->UIFrameArena, "spacer###spacer%s", (char*)text.str);
        u64 hashSpacer;
        String8 SpacerText2 = HandleUIString((char*)spacerText.str, &hashSpacer);

        UI_Widget* spacer = PushWidget(WidgetFlags_StackHorizontally,
                                       SpacerText2,
                                       hashSpacer,
                                       { SizeKind_ChildSum, 1.0f },
                                       { SizeKind_MaxChild, 1.0f });
        spacer->style_vars[StyleVar_Padding] = Vec4(0.0f, 3.0f * s_UIState->DPIScale, 0.0f, 0.0f);
        spacer->style_vars[StyleVar_ItemSpacing] = Vec4(0.0f);

        UI_Interaction slider_interaction = {};

        PushParent(spacer);
        {
            float lSliderWidth  = width;
            float lSliderHeight = height;
            String8 parentText  = PushStr8F(s_UIState->UIFrameArena, "parent###parenti%s", (char*)text.str);
            UI_Widget* parent   = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBorder | WidgetFlags_DrawBackground | WidgetFlags_CentreY,
                                             text,
                                             HashUIStr8Name(parentText),
                                             { SizeKind_Pixels, lSliderWidth },
                                             { SizeKind_Pixels, lSliderHeight });

            {float dp = s_UIState->DPIScale;
            parent->style_vars[StyleVar_Padding]      = Vec4(0.0f);
            parent->style_vars[StyleVar_CornerRadius] = Vec4(4.0f * dp, 0.0f, 0.0f, 0.0f);}

            if(UIThemeIsDark())
            {
                parent->style_vars[StyleVar_BackgroundColor] = Vec4(0.25f, 0.25f, 0.25f, 1.0f);
                parent->style_vars[StyleVar_BorderColor]     = Vec4(0.4f, 0.4f, 0.4f, 1.0f);
            }
            else
            {
                parent->style_vars[StyleVar_BackgroundColor] = Vec4(0.7f, 0.7f, 0.7f, 1.0f);
                parent->style_vars[StyleVar_BorderColor]     = Vec4(0.5f, 0.5f, 0.5f, 1.0f);
            }

            UI_Interaction parent_interaction = HandleWidgetInteraction(parent);
            PushParent(parent);

            float handleSizeFraction = 0.15f;
            UI_Widget* slider = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBorder | WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Draggable | WidgetFlags_AnimateScale,
                                           text,
                                           hash,
                                           { SizeKind_PercentOfParent, handleSizeFraction },
                                           { SizeKind_PercentOfParent, 1.0f });

            {float dp = s_UIState->DPIScale;
            slider->style_vars[StyleVar_Border]       = Vec4(1.0f * dp, 1.0f * dp, 0.0f, 0.0f);
            slider->style_vars[StyleVar_Padding]      = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
            slider->style_vars[StyleVar_CornerRadius] = Vec4(5.0f * dp, 0.0f, 0.0f, 0.0f);}

            if(UIThemeIsDark())
            {
                slider->style_vars[StyleVar_BorderColor]           = Vec4(0.5f, 0.5f, 0.5f, 1.0f);
                slider->style_vars[StyleVar_BackgroundColor]       = Vec4(0.35f, 0.35f, 0.35f, 1.0f);
                slider->style_vars[StyleVar_HotBorderColor]        = Vec4(0.4f, 0.6f, 1.0f, 1.0f);
                slider->style_vars[StyleVar_HotBackgroundColor]    = Vec4(0.40f, 0.40f, 0.40f, 1.0f);
                slider->style_vars[StyleVar_ActiveBorderColor]     = Vec4(0.2f, 0.4f, 0.9f, 1.0f);
                slider->style_vars[StyleVar_ActiveBackgroundColor] = Vec4(0.45f, 0.45f, 0.45f, 1.0f);
            }
            else
            {
                slider->style_vars[StyleVar_BorderColor]           = Vec4(0.3f, 0.3f, 0.3f, 1.0f);
                slider->style_vars[StyleVar_BackgroundColor]       = Vec4(0.9f, 0.9f, 0.9f, 1.0f);
                slider->style_vars[StyleVar_HotBorderColor]        = Vec4(0.4f, 0.6f, 1.0f, 1.0f);
                slider->style_vars[StyleVar_HotBackgroundColor]    = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
                slider->style_vars[StyleVar_ActiveBorderColor]     = Vec4(0.2f, 0.4f, 0.9f, 1.0f);
                slider->style_vars[StyleVar_ActiveBackgroundColor] = Vec4(0.95f, 0.95f, 0.95f, 1.0f);
            }
            slider_interaction = HandleWidgetInteraction(slider);

            PopParent(parent);

            slider->drag_constraint_y = true;

            *value = Maths::Clamp(*value, min_value, max_value);

            f32 parent_x      = parent->position.x;
            f32 parent_size_x = parent->size.x;
            f32 slider_size_x = parent_size_x * handleSizeFraction;

            Vec2 mouse = Input::Get().GetMousePosition() * s_UIState->InputScale - s_UIState->InputOffset;

            if(parent_interaction.clicked)
            {
                f32 t  = (mouse.x - parent_x - slider_size_x * 0.5f) / (parent_size_x - slider_size_x);
                t      = Maths::Clamp(t, 0.0f, 1.0f);
                *value = min_value + (int)Maths::Round((float)(max_value - min_value) * t);
            }

            f32 t = (float)(*value - min_value) / (float)(max_value - min_value);
            if(slider->dragging)
            {
                Vec2 dragMouse = Input::Get().GetMousePosition() * s_UIState->InputScale - s_UIState->InputOffset;
                t              = (dragMouse.x - parent_x - slider_size_x * 0.5f) / (parent_size_x - slider_size_x);
            }

            t      = Maths::Clamp(t, 0.0f, 1.0f);
            *value = Maths::Clamp(min_value + (int)Maths::Round((float)(max_value - min_value) * t), min_value, max_value);

            slider->relative_position[UIAxis_X] = t * (parent_size_x - slider_size_x);

            // Integer display label — stable "###" id, value excluded from hash
            String8 slider_text  = PushStr8F(s_UIState->UIFrameArena, "  %.*s: %d###slvali", (int)text.size, (const char*)text.str, *value);
            u64 labelHash;
            String8 slider_label = HandleUIString((char*)slider_text.str, &labelHash);
            UIAdoptText(PushWidget(WidgetFlags_DrawText | WidgetFlags_CentreY,
                                   slider_label,
                                   labelHash,
                                   { SizeKind_TextContent, 1.0f },
                                   { SizeKind_TextContent, 1.0f }),
                        slider_label);
        }
        PopParent(spacer);

        return slider_interaction;
    }

    void UISeparator(float width)
    {
        String8 sepText = PushStr8F(s_UIState->UIFrameArena, "separator###sep%llu", s_UIState->WidgetIdCounter++);
        u64 hash;
        String8 text = HandleUIString((char*)sepText.str, &hash);

        SizeKind sizeKind = width > 0.0f ? SizeKind_Pixels : SizeKind_PercentOfParent;
        float sizeValue   = width > 0.0f ? width : 1.0f;

        UI_Widget* separator = PushWidget(WidgetFlags_DrawBackground,
                                          text,
                                          hash,
                                          { sizeKind, sizeValue },
                                          { SizeKind_Pixels, 2.0f });

        separator->style_vars[StyleVar_Padding] = Vec4(0.0f, 3.0f * s_UIState->DPIScale, 0.0f, 0.0f);
        separator->style_vars[StyleVar_Border] = Vec4(0.0f, 0.0f, 0.0f, 0.0f);

        if(s_UIState->style_variable_lists[StyleVar_BackgroundColor].count == 0)
        {
            if(UIThemeIsDark())
            {
                separator->style_vars[StyleVar_BackgroundColor] = Vec4(0.4f, 0.4f, 0.4f, 1.0f);
            }
            else
            {
                separator->style_vars[StyleVar_BackgroundColor] = Vec4(0.7f, 0.7f, 0.7f, 1.0f);
            }
        }
    }

    void UISpacer(float size)
    {
        size *= s_UIState->DPIScale;
        String8 spacerText = PushStr8F(s_UIState->UIFrameArena, "spacer###spacer%llu", s_UIState->WidgetIdCounter++);
        u64 hash;
        String8 text = HandleUIString((char*)spacerText.str, &hash);

        // Check parent layout direction
        UI_Widget* parent = GetCurrentParent();
        bool isHorizontal = parent && (parent->flags & WidgetFlags_StackHorizontally);

        if(isHorizontal)
        {
            PushWidget(0, text, hash,
                       { SizeKind_Pixels, size },
                       { SizeKind_Pixels, 1.0f });
        }
        else
        {
            PushWidget(0, text, hash,
                       { SizeKind_Pixels, 1.0f },
                       { SizeKind_Pixels, size });
        }
    }

    void UIBeginRow()
    {
        String8 rowText = PushStr8F(s_UIState->UIFrameArena, "row###row%llu", s_UIState->WidgetIdCounter++);
        u64 hash;
        String8 text = HandleUIString((char*)rowText.str, &hash);

        UI_Widget* row = PushWidget(WidgetFlags_StackHorizontally,
                                    text,
                                    hash,
                                    { SizeKind_ChildSum, 1.0f },
                                    { SizeKind_MaxChild, 1.0f });

        row->style_vars[StyleVar_Padding] = Vec4(0.0f, 3.0f * s_UIState->DPIScale, 0.0f, 0.0f);
        row->style_vars[StyleVar_ItemSpacing] = Vec4(4.0f * s_UIState->DPIScale, 0.0f, 0.0f, 0.0f);
        PushParent(row);
    }

    void UIBeginRowFullWidth()
    {
        UIBeginRow();
        GetCurrentParent()->semantic_size[UIAxis_X] = { SizeKind_PercentOfParent, 1.0f };
    }

    void UIEndRow()
    {
        PopParent(GetCurrentParent());
    }

    void UIBeginColumn()
    {
        String8 colText = PushStr8F(s_UIState->UIFrameArena, "col###col%llu", s_UIState->WidgetIdCounter++);
        u64 hash;
        String8 text = HandleUIString((char*)colText.str, &hash);

        UI_Widget* col = PushWidget(WidgetFlags_StackVertically,
                                    text,
                                    hash,
                                    { SizeKind_MaxChild, 1.0f },
                                    { SizeKind_ChildSum, 1.0f });

        col->style_vars[StyleVar_Padding] = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        PushParent(col);
    }

    void UIEndColumn()
    {
        PopParent(GetCurrentParent());
    }

    UI_Interaction UIExpander(const char* str, bool* expanded)
    {
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        String8 rowText = PushStr8F(s_UIState->UIFrameArena, "exprow###exprow%s", (char*)text.str);
        u64 hashRow;
        HandleUIString((char*)rowText.str, &hashRow);

        UI_Widget* row = PushWidget(WidgetFlags_StackHorizontally | WidgetFlags_Clickable,
                                    text,
                                    hashRow,
                                    { SizeKind_ChildSum, 1.0f },
                                    { SizeKind_MaxChild, 1.0f });
        float dp = s_UIState->DPIScale;
        row->style_vars[StyleVar_Padding] = Vec4(2.0f * dp, 1.0f * dp, 0.0f, 0.0f);

        UI_Interaction interaction = HandleWidgetInteraction(row);
        if(interaction.clicked)
        {
            *expanded = !(*expanded);
        }

        PushParent(row);

        // Arrow indicator
        String8 arrowText = *expanded ? Str8Lit("-") : Str8Lit("+");
        String8 arrowId   = PushStr8F(s_UIState->UIFrameArena, "arrow###arrow%s", (char*)text.str);
        u64 hashArrow;
        HandleUIString((char*)arrowId.str, &hashArrow);

        UI_Widget* arrow = PushWidget(WidgetFlags_DrawText | WidgetFlags_CentreY,
                                      arrowText,
                                      hashArrow,
                                      { SizeKind_TextContent, 1.0f },
                                      { SizeKind_TextContent, 1.0f });
        arrow->style_vars[StyleVar_Padding] = Vec4(2.0f * dp, 0.0f, 0.0f, 0.0f);

        // Label
        String8 labelId = PushStr8F(s_UIState->UIFrameArena, "label###explabel%s", (char*)text.str);
        u64 hashLabel;
        HandleUIString((char*)labelId.str, &hashLabel);

        UI_Widget* label = PushWidget(WidgetFlags_DrawText | WidgetFlags_CentreY,
                                      text,
                                      hashLabel,
                                      { SizeKind_TextContent, 1.0f },
                                      { SizeKind_TextContent, 1.0f });

        PopParent(row);

        return interaction;
    }

    static UI_Widget* s_ExpanderContentWidget = nullptr;

    void UIBeginExpanderContent(const char* str)
    {
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        String8 contentId = PushStr8F(s_UIState->UIFrameArena, "expcontent###expcontent%s", (char*)text.str);
        u64 hashContent;
        HandleUIString((char*)contentId.str, &hashContent);

        float dp = s_UIState->DPIScale;
        UI_Widget* content = PushWidget(WidgetFlags_StackVertically,
                                        Str8Lit(""),
                                        hashContent,
                                        { SizeKind_PercentOfParent, 1.0f },
                                        { SizeKind_ChildSum, 1.0f });
        content->style_vars[StyleVar_Padding] = Vec4(16.0f * dp, 2.0f * dp, 0.0f, 0.0f);
        s_ExpanderContentWidget = content;
        PushParent(content);
    }

    void UIEndExpanderContent()
    {
        if(s_ExpanderContentWidget)
        {
            PopParent(s_ExpanderContentWidget);
            s_ExpanderContentWidget = nullptr;
        }
    }

    static UI_Widget* s_ScrollAreaWidget = nullptr;
    static float* s_ScrollAreaOffset     = nullptr;
    static float s_ScrollAreaHeight      = 0.0f;
    static float s_ScrollAreaContentHeight = 0.0f;
    // Touch content-drag: which area (widget hash) owns the active pan.
    static u64 s_ScrollDragHash   = 0;
    static float s_ScrollDragLastY = 0.0f;

    // Areas nest (a list inside a scrolling dock panel): the outer one waits here.
    struct ScrollAreaFrame
    {
        UI_Widget* widget;
        float* offset;
        float height;
    };
    static ScrollAreaFrame s_ScrollAreaStack[8];
    static i32 s_ScrollAreaDepth = 0;
    // The innermost hovered area takes the wheel; the ones around it leave it alone that frame.
    static u64 s_WheelFrame        = 0;
    static UI_State* s_WheelState  = nullptr;

    void UIBeginScrollArea(const char* str, float height, float* scroll_offset)
    {
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        UI_Widget* area = PushWidget(WidgetFlags_StackVertically | WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_Clickable,
                                     text,
                                     hash,
                                     { SizeKind_PercentOfParent, 1.0f },
                                     { SizeKind_Pixels, height });

        area->style_vars[StyleVar_Padding]      = Vec4(3.0f * s_UIState->DPIScale, 3.0f * s_UIState->DPIScale, 0.0f, 0.0f);
        area->style_vars[StyleVar_CornerRadius] = Vec4(3.0f * s_UIState->DPIScale, 0.0f, 0.0f, 0.0f);

        if(UIThemeIsDark())
        {
            area->style_vars[StyleVar_BackgroundColor] = Vec4(0.12f, 0.12f, 0.12f, 1.0f);
            area->style_vars[StyleVar_BorderColor]     = Vec4(0.3f, 0.3f, 0.3f, 1.0f);
        }
        else
        {
            area->style_vars[StyleVar_BackgroundColor] = Vec4(0.97f, 0.97f, 0.97f, 1.0f);
            area->style_vars[StyleVar_BorderColor]     = Vec4(0.7f, 0.7f, 0.7f, 1.0f);
        }

        area->Clip        = true;
        area->ChildOffset = Vec2(0.0f, -(*scroll_offset));

        if(s_ScrollAreaDepth < 8)
            s_ScrollAreaStack[s_ScrollAreaDepth] = { s_ScrollAreaWidget, s_ScrollAreaOffset, s_ScrollAreaHeight };
        s_ScrollAreaDepth++;

        s_ScrollAreaWidget = area;
        s_ScrollAreaOffset = scroll_offset;
        s_ScrollAreaHeight = height;

        PushParent(area);
    }

    static UI_Widget* UIRootChildOf(UI_Widget* w)
    {
        for(int depth = 0; w->parent && w->parent->parent && depth < 128; depth++)
            w = w->parent;
        return w;
    }

    // true when whatever the pointer is on (last frame's hot / active widget) sits outside
    // this area - a popup or backdrop drawn over it. An area never claims hot itself, so a
    // scrim BEHIND it is often the hot widget: that one doesn't count.
    static bool UIPointerOwnedElsewhere(UI_Widget* area)
    {
        const u64 owner = s_UIState->hot_widget ? s_UIState->hot_widget : s_UIState->active_widget;
        if(!owner)
            return false;
        UI_Widget* found = nullptr;
        HashMapFind(&s_UIState->widgets, owner, &found);
        if(!found)
            return false;
        UI_Widget* w = found;
        for(int depth = 0; w && depth < 128; depth++, w = w->parent)
            if(w == area)
                return false;
        UI_Widget* ownerTop = UIRootChildOf(found);
        for(UI_Widget* s = UIRootChildOf(area)->prev; s; s = s->prev)
            if(s == ownerTop)
                return false;
        return true;
    }

    // ---- horizontal scroll row: a clipped strip that slides sideways (wheel / trackpad x,
    // shift + wheel, or a finger drag on touch screens)

    static UI_Widget* s_HScrollWidget = nullptr;
    static f32* s_HScrollOffset       = nullptr;
    static u64 s_HScrollDragHash      = 0;
    static f32 s_HScrollDragLastX     = 0.0f;
    static Vec2 s_HScrollStart        = Vec2(0.0f);
    static int s_HScrollAxis          = 0;     // 0 undecided, 1 sideways (ours), 2 up / down (the page's)
    static bool s_HScrollOwnsDrag     = false; // this frame's finger drag moved a row: the page stays put

    void UIBeginHScrollArea(const char* str, float* offset)
    {
        u64 hash;
        HandleUIString(str, &hash);
        UI_Widget* area = PushWidget(WidgetFlags_StackHorizontally, Str8Lit(""), hash,
                                     { SizeKind_PercentOfParent, 1.0f }, { SizeKind_MaxChild, 1.0f });
        area->style_vars[StyleVar_Padding] = Vec4(0.0f);
        area->style_vars[StyleVar_Border]  = Vec4(0.0f);
        area->Clip                         = true;
        area->ChildOffset                  = Vec2(-(*offset), 0.0f);
        s_HScrollWidget                    = area;
        s_HScrollOffset                    = offset;
        PushParent(area);
    }

    void UIEndHScrollArea()
    {
        UI_Widget* area = s_HScrollWidget;
        if(!area)
            return;
        const float dpi = s_UIState->DPIScale;
        float contentW  = 0.0f;
        int n           = 0;
        for(UI_Widget* c = area->first; c; c = c->next)
        {
            if(c->flags & WidgetFlags_Floating_X)
                continue;
            const UI_Size& sx = c->semantic_size[UIAxis_X];
            contentW += sx.kind == SizeKind_Pixels ? sx.value : c->size.x;
            n++;
        }
        if(n > 1)
            contentW += area->style_vars[StyleVar_ItemSpacing].x * (f32)(n - 1);
        const float maxScroll = Maths::Max(0.0f, contentW - area->size.x);

        Vec2 mousePos = Input::Get().GetMousePosition() * s_UIState->InputScale - s_UIState->InputOffset;
        bool hovering = mousePos.x >= area->position.x && mousePos.x <= area->position.x + area->size.x &&
                        mousePos.y >= area->position.y && mousePos.y <= area->position.y + area->size.y;
        if(hovering && UIPointerOwnedElsewhere(area))
            hovering = false;
        float& off = *s_HScrollOffset;
        ScrollbarFadeState* phys = s_ScrollPhysics ? &ScrollbarFadeFor(area->hash, off) : nullptr;
        const float dt           = Maths::Max(s_UIFrameDT, 0.0001f);
        if(phys && Maths::Abs(off - phys->physOffset) > 0.5f)
            phys->velocity = 0.0f; // set from outside: stop coasting
        const float viewW = Maths::Max(1.0f, area->size.x);
        auto push         = [&](float d) {
            // past an end a push only gets part of the way (rubber band)
            if(phys && ((off < 0.0f && d < 0.0f) || (off > maxScroll && d > 0.0f)))
                d /= 1.0f + (off < 0.0f ? -off : off - maxScroll) / (0.12f * viewW);
            off += d;
        };
        bool input = false;
        if(hovering)
        {
            float dx = Input::Get().GetScrollOffsetX();
            const bool shift = Input::Get().GetKeyHeldRaw(InputCode::Key::LeftShift) || Input::Get().GetKeyHeldRaw(InputCode::Key::RightShift);
            if(shift && dx == 0.0f)
                dx = Input::Get().GetScrollOffset();
            if(dx != 0.0f)
            {
                // wheel / trackpad stop at the ends (the rubber band is for fingers)
                off = Maths::Clamp(off - dx * 30.0f * dpi, Maths::Min(off, 0.0f), Maths::Max(off, maxScroll));
                input = true;
                if(phys)
                    phys->velocity = 0.0f; // trackpads coast by themselves
            }
        }
        if(s_UIState->ClickOnRelease && maxScroll > 0.0f)
        {
            const bool held = Input::Get().GetMouseHeld(Lumos::InputCode::MouseKey::ButtonLeft);
            if(held && (hovering || s_HScrollDragHash == area->hash))
            {
                if(s_HScrollDragHash != area->hash)
                {
                    s_HScrollDragHash  = area->hash;
                    s_HScrollStart     = mousePos;
                    s_HScrollAxis      = 0;
                    if(phys)
                        phys->velocity = 0.0f; // a finger down catches it
                }
                else
                {
                    // decide the direction once the finger has moved a little; up / down belongs to the page
                    if(s_HScrollAxis == 0)
                    {
                        const Vec2 moved = mousePos - s_HScrollStart;
                        if(Maths::Abs(moved.x) > 6.0f * dpi || Maths::Abs(moved.y) > 6.0f * dpi)
                            s_HScrollAxis = Maths::Abs(moved.x) > Maths::Abs(moved.y) ? 1 : 2;
                    }
                    if(s_HScrollAxis == 1)
                    {
                        const float d = -(mousePos.x - s_HScrollDragLastX);
                        push(d);
                        if(phys)
                            phys->velocity = Maths::Lerp(phys->velocity, d / dt, 0.35f);
                        s_HScrollOwnsDrag = true;
                    }
                }
                s_HScrollDragLastX = mousePos.x;
                input              = s_HScrollAxis != 2;
            }
            else if(s_HScrollDragHash == area->hash)
                s_HScrollDragHash = 0;
        }
        if(phys)
        {
            if(!input)
            {
                // coast, then spring back from past either end
                off += phys->velocity * dt;
                const bool out = off < 0.0f || off > maxScroll;
                phys->velocity *= expf(-dt * (out ? 18.0f : 2.2f));
                if(Maths::Abs(phys->velocity) < 20.0f * dpi)
                    phys->velocity = 0.0f;
                if(off < 0.0f)
                    off *= expf(-dt * 12.0f);
                else if(off > maxScroll)
                    off = maxScroll + (off - maxScroll) * expf(-dt * 12.0f);
                if(off < 0.0f && off > -0.5f)
                    off = 0.0f;
                if(off > maxScroll && off < maxScroll + 0.5f)
                    off = maxScroll;
            }
            const float limit = 0.35f * viewW;
            off               = Maths::Clamp(off, -limit, maxScroll + limit);
            phys->physOffset  = off;
            phys->lastFrame   = s_UIState->FrameIndex;
        }
        else
            off = Maths::Clamp(off, 0.0f, maxScroll);
        area->ChildOffset = Vec2(-off, 0.0f);
        PopParent(area);
        s_HScrollWidget = nullptr;
        s_HScrollOffset = nullptr;
    }

    static void UIEndScrollAreaInner();

    void UIEndScrollArea()
    {
        UIEndScrollAreaInner();

        if(s_ScrollAreaDepth > 0)
        {
            s_ScrollAreaDepth--;
            if(s_ScrollAreaDepth < 8)
            {
                const ScrollAreaFrame& outer = s_ScrollAreaStack[s_ScrollAreaDepth];
                s_ScrollAreaWidget           = outer.widget;
                s_ScrollAreaOffset           = outer.offset;
                s_ScrollAreaHeight           = outer.height;
            }
        }
    }

    static void UIEndScrollAreaInner()
    {
        if(!s_ScrollAreaWidget)
        {
            s_ScrollAreaOffset = nullptr;
            return;
        }
        if(!s_ScrollAreaOffset)
        {
            PopParent(s_ScrollAreaWidget);
            s_ScrollAreaWidget = nullptr;
            return;
        }

        float dpi = s_UIState->DPIScale;

        float contentHeight = 0.0f;
        i32 contentCount    = 0;
        for(UI_Widget* child = s_ScrollAreaWidget->first; child; child = child->next)
        {
            if(child->flags & WidgetFlags_Floating_Y)
                continue;
            // fixed-height children built this frame have no laid-out size yet (jumps, virtualised lists)
            const UI_Size& sy = child->semantic_size[UIAxis_Y];
            contentHeight += sy.kind == SizeKind_Pixels ? sy.value : child->size.y;
            contentCount++;
        }
        if(contentCount > 1)
            contentHeight += s_ScrollAreaWidget->style_vars[StyleVar_ItemSpacing].y * (f32)(contentCount - 1);
        s_ScrollAreaContentHeight = contentHeight;

        float padding       = s_ScrollAreaWidget->style_vars[StyleVar_Padding].y;
        float visibleHeight = s_ScrollAreaHeight - padding * 2.0f;

        float maxScroll = Maths::Max(0.0f, contentHeight - visibleHeight);

        // Mouse wheel while hovering (mouse in the same space as widget rects)
        Vec2 mousePos  = Input::Get().GetMousePosition() * s_UIState->InputScale - s_UIState->InputOffset;
        Vec2 areaMin   = s_ScrollAreaWidget->position;
        Vec2 areaMax   = areaMin + s_ScrollAreaWidget->size;
        bool hovering  = mousePos.x >= areaMin.x && mousePos.x <= areaMax.x &&
                         mousePos.y >= areaMin.y && mousePos.y <= areaMax.y;
        if(hovering && UIPointerOwnedElsewhere(s_ScrollAreaWidget))
            hovering = false;
        float& off = *s_ScrollAreaOffset;
        ScrollbarFadeState* phys = s_ScrollPhysics ? &ScrollbarFadeFor(s_ScrollAreaWidget->hash, off) : nullptr;
        const float dt = Maths::Max(s_UIFrameDT, 0.0001f);
        if(phys && Maths::Abs(off - phys->physOffset) > 0.5f)
            phys->velocity = 0.0f; // someone set it (jump to a letter, keep a row in view): stop coasting
        // how far past either end, and a move that pushes further out only gets part of the way
        auto overscroll = [&]() { return off < 0.0f ? -off : Maths::Max(0.0f, off - maxScroll); };
        auto push       = [&](float d) {
            if(phys && ((off < 0.0f && d < 0.0f) || (off > maxScroll && d > 0.0f)))
                d /= 1.0f + overscroll() / (0.12f * visibleHeight);
            off += d;
        };
        bool input = false;

        const bool wheelTaken = s_WheelState == s_UIState && s_WheelFrame == s_UIState->FrameIndex;
        if(hovering && !wheelTaken)
        {
            float scrollDelta = Input::Get().GetScrollOffset() * 30.0f * dpi;
            if(scrollDelta != 0.0f && maxScroll > 0.0f)
            {
                s_WheelFrame = s_UIState->FrameIndex;
                s_WheelState = s_UIState;
                // wheel / trackpad stop at the ends: macOS keeps sending momentum events after the
                // fingers lift, which dragged the rubber band far out and held it there
                off = Maths::Clamp(off - scrollDelta, Maths::Min(off, 0.0f), Maths::Max(off, maxScroll));
                input = true;
                if(phys)
                    phys->velocity = 0.0f; // trackpads send their own momentum
            }
        }

        if(s_UIState->ClickOnRelease && maxScroll > 0.0f)
        {
            const u64 areaHash = s_ScrollAreaWidget->hash;
            const bool held    = Input::Get().GetMouseHeld(Lumos::InputCode::MouseKey::ButtonLeft);
            if(held && (hovering || s_ScrollDragHash == areaHash))
            {
                if(s_ScrollDragHash == areaHash && !s_HScrollOwnsDrag)
                {
                    const float d = -(mousePos.y - s_ScrollDragLastY);
                    push(d);
                    if(phys)
                        phys->velocity = Maths::Lerp(phys->velocity, d / dt, 0.35f);
                }
                else if(phys)
                    phys->velocity = 0.0f; // a finger down catches a coasting list
                s_ScrollDragHash  = areaHash;
                s_ScrollDragLastY = mousePos.y;
                input             = true;
            }
            else if(s_ScrollDragHash == areaHash)
            {
                s_ScrollDragHash = 0;
            }
        }

        if(phys)
        {
            if(!input)
            {
                // coast, slowing like iOS; hard braking once past an end
                off += phys->velocity * dt;
                const bool out = off < 0.0f || off > maxScroll;
                phys->velocity *= expf(-dt * (out ? 18.0f : 2.2f));
                if(Maths::Abs(phys->velocity) < 20.0f * dpi)
                    phys->velocity = 0.0f;
                // spring back to the end
                if(off < 0.0f)
                    off *= expf(-dt * 12.0f);
                else if(off > maxScroll)
                    off = maxScroll + (off - maxScroll) * expf(-dt * 12.0f);
                if(off < 0.0f && off > -0.5f)
                    off = 0.0f;
                if(off > maxScroll && off < maxScroll + 0.5f)
                    off = maxScroll;
            }
            const float limit = 0.35f * visibleHeight;
            off = Maths::Clamp(off, -limit, maxScroll + limit);
            phys->physOffset = off;
            phys->lastFrame  = s_UIState->FrameIndex;
        }
        else
            off = Maths::Clamp(off, 0.0f, maxScroll);

        // Only show scrollbar if content exceeds visible area
        if(contentHeight > visibleHeight)
        {
            float scrollbarWidth = 8.0f * dpi;
            float trackHeight = visibleHeight;
            float thumbRatio = visibleHeight / contentHeight;
            float thumbHeight = Maths::Max(20.0f * dpi, trackHeight * thumbRatio);
            float scrollRatio = (maxScroll > 0.0f) ? Maths::Clamp(*s_ScrollAreaOffset / maxScroll, 0.0f, 1.0f) : 0.0f;
            float thumbOffset = scrollRatio * (trackHeight - thumbHeight);

            // fade: visible while it moves, while dragged, or with the pointer at the bar
            float barAlpha = 1.0f;
            if(s_ScrollbarFade)
            {
                const u64 thumbId          = UIHashCombine(s_ScrollAreaWidget->hash, 0x5448554Dull);
                ScrollbarFadeState& fade   = ScrollbarFadeFor(s_ScrollAreaWidget->hash, *s_ScrollAreaOffset);
                const float edgeX          = s_ScrollAreaWidget->position.x + s_ScrollAreaWidget->size.x;
                const bool atBar           = hovering && mousePos.x > edgeX - 18.0f * dpi && !s_UIState->ClickOnRelease;
                const bool moving          = Maths::Abs(*s_ScrollAreaOffset - fade.lastOffset) > 0.5f;
                if(moving || atBar || s_UIState->active_widget == thumbId || s_ScrollDragHash == s_ScrollAreaWidget->hash)
                    fade.idle = 0.0f;
                else
                    fade.idle += s_UIFrameDT;
                const float target = fade.idle < 0.8f ? 1.0f : 0.0f;
                const float rate   = target > fade.alpha ? 12.0f : 4.0f;
                fade.alpha += Maths::Clamp(target - fade.alpha, -rate * s_UIFrameDT, rate * s_UIFrameDT);
                fade.lastOffset = *s_ScrollAreaOffset;
                fade.lastFrame  = s_UIState->FrameIndex;
                barAlpha        = fade.alpha;
            }

            // Scrollbar track. Ids come from the area's own hash: a frame counter shifts whenever the
            // number of widgets before it changes (virtualised lists), which dropped thumb drags mid-way.
            const u64 trackHash = UIHashCombine(s_ScrollAreaWidget->hash, 0x5452434Bull);

            UI_Widget* track = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                          Str8Lit(""),
                                          trackHash,
                                          { SizeKind_Pixels, scrollbarWidth },
                                          { SizeKind_Pixels, trackHeight });

            track->relative_position = Vec2(s_ScrollAreaWidget->size.x - scrollbarWidth - 2.0f * dpi, padding);
            track->style_vars[StyleVar_Padding] = Vec4(0.0f);

            if(UIThemeIsDark())
                track->style_vars[StyleVar_BackgroundColor] = Vec4(0.2f, 0.2f, 0.2f, 0.5f * barAlpha);
            else
                track->style_vars[StyleVar_BackgroundColor] = Vec4(0.85f, 0.85f, 0.85f, 0.5f * barAlpha);
            // a fading bar is the thumb alone, like macOS
            if(s_ScrollbarFade)
                track->style_vars[StyleVar_BackgroundColor].w = 0.0f;

            track->style_vars[StyleVar_CornerRadius] = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);

            const u64 thumbHash = UIHashCombine(s_ScrollAreaWidget->hash, 0x5448554Dull);

            UI_Widget* thumb = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | WidgetFlags_Clickable,
                                          Str8Lit(""),
                                          thumbHash,
                                          { SizeKind_Pixels, scrollbarWidth - 2.0f * dpi },
                                          { SizeKind_Pixels, thumbHeight });

            thumb->relative_position = Vec2(s_ScrollAreaWidget->size.x - scrollbarWidth - 1.0f * dpi, padding + thumbOffset);
            thumb->style_vars[StyleVar_Padding] = Vec4(0.0f);

            if(UIThemeIsDark())
                thumb->style_vars[StyleVar_BackgroundColor] = Vec4(0.5f, 0.5f, 0.5f, 0.8f * barAlpha);
            else
                thumb->style_vars[StyleVar_BackgroundColor] = Vec4(0.6f, 0.6f, 0.6f, 0.8f * barAlpha);

            thumb->style_vars[StyleVar_CornerRadius] = Vec4(3.0f * dpi, 0.0f, 0.0f, 0.0f);

            UI_Interaction thumbInteraction = HandleWidgetInteraction(thumb);

            float trackTop = s_ScrollAreaWidget->position.y + padding;
            if(thumbInteraction.clicked)
            {
                thumb->drag_offset.y = mousePos.y - (trackTop + thumbOffset);
            }
            if(s_UIState->active_widget == thumb->hash && maxScroll > 0.0f)
            {
                float t = (mousePos.y - thumb->drag_offset.y - trackTop) / Maths::Max(1.0f, trackHeight - thumbHeight);
                *s_ScrollAreaOffset = Maths::Clamp(t, 0.0f, 1.0f) * maxScroll;
            }
        }

        s_ScrollAreaWidget->ChildOffset = Vec2(0.0f, -(*s_ScrollAreaOffset));

        PopParent(s_ScrollAreaWidget);
        s_ScrollAreaWidget = nullptr;
        s_ScrollAreaOffset = nullptr;
    }

    struct AutoScrollEntry
    {
        u64 hash;
        f32 offset;
    };
    static AutoScrollEntry s_AutoScrollOffsets[32];
    static u32 s_AutoScrollCount = 0;

    void UIBeginScrollAreaAuto(const char* str, float height)
    {
        u64 hash;
        HandleUIString(str, &hash); // same parent-chained hash the widget gets
        f32* offset = nullptr;
        for(u32 i = 0; i < s_AutoScrollCount; i++)
        {
            if(s_AutoScrollOffsets[i].hash == hash)
            {
                offset = &s_AutoScrollOffsets[i].offset;
                break;
            }
        }
        if(!offset)
        {
            if(s_AutoScrollCount >= 32)
                s_AutoScrollCount = 0; // wrap: stale areas just lose their scroll position
            s_AutoScrollOffsets[s_AutoScrollCount] = { hash, 0.0f };
            offset = &s_AutoScrollOffsets[s_AutoScrollCount++].offset;
        }
        UIBeginScrollArea(str, height, offset);
        s_ScrollAreaWidget->style_vars[StyleVar_BackgroundColor] = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        s_ScrollAreaWidget->style_vars[StyleVar_BorderColor]     = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
    }

#ifdef LUMOS_PLATFORM_IOS
    extern "C" void OpeniOSKeyboard(std::string* cppString);
    extern "C" void CloseiOSKeyboard();
    // never destroyed: UIKit can still end editing (and write here) while the app tears down
    static std::string& s_NativeKeyboardText = *new std::string();
    static std::string& s_NativeKeyboardSeen = *new std::string(); // last native text copied in
    static u64 s_NativeKeyboardHash = 0;
#endif

    // One-shot: the next UITextInput built claims keyboard focus.
    static bool s_ClaimNextTextInputFocus = false;

    void UIFocusNextTextInput()
    {
        s_ClaimNextTextInputFocus = true;
    }

    // ---- text field extras: drawn caret + selection, mouse placement / selection

    static u64 s_TextDragHash   = 0;
    static f32 s_CaretTime      = 0.0f;
    static u32 s_CaretLastIndex = ~0u;

    static bool IsWordByte(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || (u8)c >= 0x80;
    }

    static u32 PrevWordIndex(const char* buf, u32 i)
    {
        while(i > 0 && !IsWordByte(buf[i - 1])) i--;
        while(i > 0 && IsWordByte(buf[i - 1])) i--;
        return i;
    }

    static u32 NextWordIndex(const char* buf, u32 i, u32 len)
    {
        while(i < len && !IsWordByte(buf[i])) i++;
        while(i < len && IsWordByte(buf[i])) i++;
        return i;
    }

    static f32 TextWidthTo(const char* buf, u32 i, f32 fontSize)
    {
        return i == 0 ? 0.0f : GetStringSize(Str8((u8*)buf, i), fontSize).x;
    }

    static u32 TextIndexAt(const char* buf, u32 len, f32 fontSize, f32 x)
    {
        u32 best    = 0;
        f32 bestGap = Maths::Abs(x);
        for(u32 i = 1; i <= len; i++)
        {
            if(i < len && ((u8)buf[i] & 0xC0) == 0x80)
                continue; // inside a UTF-8 sequence
            const f32 gap = Maths::Abs(x - TextWidthTo(buf, i, fontSize));
            if(gap < bestGap)
            {
                bestGap = gap;
                best    = i;
            }
        }
        return best;
    }

    // Call after the field's focus handling. Places the cursor where it was clicked, selects by
    // dragging / a word by double-clicking, and draws the selection and a softly pulsing caret.
    // first byte shown by the focused field when its text is wider than it is
    static u64 s_TextScrollHash  = 0;
    static u32 s_TextScrollStart = 0;

    static u32 Utf8Next(const char* buf, u32 i, u32 len)
    {
        if(i < len) i++;
        while(i < len && ((u8)buf[i] & 0xC0) == 0x80) i++;
        return i;
    }

    static u32 Utf8Prev(const char* buf, u32 i)
    {
        if(i > 0) i--;
        while(i > 0 && ((u8)buf[i] & 0xC0) == 0x80) i--;
        return i;
    }

    // The slice of the buffer that fits the field, kept round the cursor while focused.
    static void TextFieldView(UI_Widget* field, const char* buffer, u64 fieldHash, u32* outStart, u32* outEnd, const Vec2* padding = nullptr)
    {
        const u32 len   = (u32)strlen(buffer);
        const f32 fs    = field->style_vars[StyleVar_FontSize].x;
        const f32 avail = field->size.x - (padding ? padding->x : field->style_vars[StyleVar_Padding].x) * 2.0f;
        *outStart       = 0;
        *outEnd         = len;
        // size is last frame's: nothing to fit against on the first one
        if(len == 0 || avail <= fs || GetStringSize(Str8((u8*)buffer, len), fs).x <= avail)
        {
            if(s_TextScrollHash == fieldHash)
                s_TextScrollStart = 0;
            return;
        }

        u32 start = 0;
        if(s_UIState->FocusedTextInput == fieldHash)
        {
            if(s_TextScrollHash != fieldHash)
            {
                s_TextScrollHash  = fieldHash;
                s_TextScrollStart = 0;
            }
            const u32 cursor = Maths::Min(s_UIState->TextInputCursor, len);
            start            = Maths::Min(s_TextScrollStart, len);
            if(cursor < start)
                start = cursor;
            while(start < cursor && GetStringSize(Str8((u8*)buffer + start, cursor - start), fs).x > avail)
                start = Utf8Next(buffer, start, len);
            // no empty space at the end while there is text off the front
            while(start > 0)
            {
                const u32 prev = Utf8Prev(buffer, start);
                if(GetStringSize(Str8((u8*)buffer + prev, len - prev), fs).x > avail)
                    break;
                start = prev;
            }
            s_TextScrollStart = start;
        }

        u32 end = start;
        while(end < len)
        {
            const u32 next = Utf8Next(buffer, end, len);
            if(GetStringSize(Str8((u8*)buffer + start, next - start), fs).x > avail)
                break;
            end = next;
        }
        *outStart = start;
        *outEnd   = end;
    }

    static void UITextFieldExtras(UI_Widget* field, const char* fullBuffer, u64 fieldHash, bool placeholder,
                                  const UI_Interaction& in, const Vec4& caretColour, u32 viewStart = 0, u32 viewEnd = ~0u,
                                  const Vec2* padding = nullptr)
    {
        if(s_UIState->FocusedTextInput != fieldHash)
        {
            if(s_TextDragHash == fieldHash)
                s_TextDragHash = 0;
            return;
        }
        const f32 dpi    = s_UIState->DPIScale;
        const f32 fs     = field->style_vars[StyleVar_FontSize].x;
        const Vec2 pad   = padding ? *padding : field->style_vars[StyleVar_Padding].ToVector2();
        const u32 fullLen = placeholder ? 0 : (u32)strlen(fullBuffer);
        viewEnd           = Maths::Min(viewEnd, fullLen);
        viewStart         = Maths::Min(viewStart, viewEnd);
        // everything below works on the visible slice; indices are shifted back at the end
        const char* buffer = fullBuffer + viewStart;
        const u32 len      = viewEnd - viewStart;
        const f32 textW    = len > 0 ? GetStringSize(Str8((u8*)buffer, len), fs).x : 0.0f;
        const f32 originX  = (field->TextAlignment & UI_Text_Alignment_Center_X) ? Maths::Max(pad.x, (field->size.x - textW) * 0.5f) : pad.x;
        const Vec2 mouse   = Input::Get().GetMousePosition() * s_UIState->InputScale - s_UIState->InputOffset;
        const f32 localX   = mouse.x - (field->position.x + originX);
        const bool held  = Input::Get().GetMouseHeld(Lumos::InputCode::MouseKey::ButtonLeft);

        // a press anywhere else lets go of the field (and puts a phone's keyboard away)
        if(Input::Get().GetMouseClicked(Lumos::InputCode::MouseKey::ButtonLeft) && !in.pressed && !in.clicked && !in.hovering)
        {
            s_UIState->FocusedTextInput    = 0;
            s_UIState->TextInputBuffer     = nullptr;
            s_UIState->TextInputBufferSize = 0;
            s_UIState->TextInputSelStart = s_UIState->TextInputSelEnd = 0;
            s_TextDragHash                 = 0;
#ifdef LUMOS_PLATFORM_IOS
            s_NativeKeyboardHash = 0;
            CloseiOSKeyboard();
#endif
            return;
        }

        auto toView = [&](u32 i) { return Maths::Clamp(i, viewStart, viewEnd) - viewStart; };
        u32 cursor   = toView(s_UIState->TextInputCursor);
        u32 selStart = toView(s_UIState->TextInputSelStart);
        u32 selEnd   = toView(s_UIState->TextInputSelEnd);
        const u32 cursorBefore = cursor, selStartBefore = selStart, selEndBefore = selEnd;

        if(in.double_clicked && len > 0)
        {
            u32 i = TextIndexAt(buffer, len, fs, localX);
            u32 a = i, b = i;
            while(a > 0 && IsWordByte(buffer[a - 1])) a--;
            while(b < len && IsWordByte(buffer[b])) b++;
            selStart = a;
            selEnd = cursor = b;
            s_TextDragHash  = 0;
        }
        else if(s_UIState->ClickOnRelease ? in.clicked : in.pressed)
        {
            const u32 i       = TextIndexAt(buffer, len, fs, localX);
            const bool shift  = Input::Get().GetKeyHeldRaw(InputCode::Key::LeftShift) || Input::Get().GetKeyHeldRaw(InputCode::Key::RightShift);
            if(shift)
                selEnd = i;
            else
                selStart = selEnd = i;
            cursor         = i;
            s_TextDragHash = s_UIState->ClickOnRelease ? 0 : fieldHash;
        }
        else if(s_TextDragHash == fieldHash)
        {
            if(held)
            {
                selEnd = cursor = TextIndexAt(buffer, len, fs, localX);
                // dragging past an end walks the view along
                if(localX > textW + pad.x && viewEnd < fullLen)
                    s_UIState->TextInputSelEnd = s_UIState->TextInputCursor = Utf8Next(fullBuffer, viewEnd, fullLen);
                else if(localX < -pad.x && viewStart > 0)
                    s_UIState->TextInputSelEnd = s_UIState->TextInputCursor = Utf8Prev(fullBuffer, viewStart);
            }
            else
                s_TextDragHash = 0;
        }

        cursor   = Maths::Min(cursor, len);
        selStart = Maths::Min(selStart, len);
        selEnd   = Maths::Min(selEnd, len);
        // only write back what the mouse changed: a selection can reach outside the view
        if(cursor != cursorBefore)
            s_UIState->TextInputCursor = cursor + viewStart;
        if(selStart != selStartBefore)
            s_UIState->TextInputSelStart = selStart + viewStart;
        if(selEnd != selEndBefore)
            s_UIState->TextInputSelEnd = selEnd + viewStart;

        // text sits at the top padding (or centred): the caret is centred on that line
        const f32 textH = GetStringSize(Str8Lit("A"), fs).y;
        const f32 lineH = Maths::Min(fs * 1.15f, Maths::Max(textH, field->size.y - 6.0f * dpi));
        const f32 textY = (field->TextAlignment & UI_Text_Alignment_Center_Y) ? (field->size.y - textH) * 0.5f : pad.y;
        const f32 y     = Maths::Max(0.0f, textY + (textH - lineH) * 0.5f);
        PushParent(field);
        if(selStart != selEnd)
        {
            const f32 x0 = TextWidthTo(buffer, Maths::Min(selStart, selEnd), fs);
            const f32 x1 = TextWidthTo(buffer, Maths::Max(selStart, selEnd), fs);
            UI_Widget* sel = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y, Str8Lit(""),
                                        UIHashCombine(fieldHash, 0x53454Cull), { SizeKind_Pixels, x1 - x0 }, { SizeKind_Pixels, lineH });
            sel->relative_position                 = Vec2(originX + x0, y);
            sel->style_vars[StyleVar_Padding]      = Vec4(0.0f);
            sel->style_vars[StyleVar_Border]       = Vec4(0.0f);
            sel->style_vars[StyleVar_CornerRadius] = Vec4(2.0f * dpi, 0.0f, 0.0f, 0.0f);
            sel->style_vars[StyleVar_BackgroundColor] = Vec4(caretColour.x, caretColour.y, caretColour.z, 0.35f);
        }
        // solid while typing / moving, then a slow pulse
        if(cursor + viewStart != s_CaretLastIndex)
        {
            s_CaretLastIndex = cursor + viewStart;
            s_CaretTime      = 0.0f;
        }
        s_CaretTime += s_UIFrameDT;
        const f32 pulse = s_CaretTime < 0.5f ? 1.0f : 0.55f + 0.45f * cosf((s_CaretTime - 0.5f) * 5.0f);
        UI_Widget* caret = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y, Str8Lit(""),
                                      UIHashCombine(fieldHash, 0x435254ull), { SizeKind_Pixels, Maths::Max(1.0f, 1.5f * dpi) }, { SizeKind_Pixels, lineH });
        caret->relative_position                    = Vec2(originX + TextWidthTo(buffer, placeholder ? 0 : cursor, fs) - (len > 0 && cursor == 0 ? 1.0f * dpi : 0.0f), y);
        caret->style_vars[StyleVar_Padding]         = Vec4(0.0f);
        caret->style_vars[StyleVar_Border]          = Vec4(0.0f);
        caret->style_vars[StyleVar_CornerRadius]    = Vec4(0.0f);
        caret->style_vars[StyleVar_BackgroundColor] = Vec4(caretColour.x, caretColour.y, caretColour.z, caretColour.w * pulse);
        PopParent(field);
    }

    static Vec4 CaretColour(const Vec4& text)
    {
        return s_UIState->AccentColour.w > 0.0f ? Vec4(s_UIState->AccentColour.x, s_UIState->AccentColour.y, s_UIState->AccentColour.z, 1.0f) : text;
    }

    UI_Interaction UITextInput(const char* str, char* buffer, u32 buffer_size, u32* cursor_pos)
    {
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        String8 rowText = PushStr8F(s_UIState->UIFrameArena, "inputrow###inputrow%s", (char*)text.str);
        u64 hashRow;
        HandleUIString((char*)rowText.str, &hashRow);

        UI_Widget* row = PushWidget(WidgetFlags_StackHorizontally,
                                    Str8Lit(""),
                                    hashRow,
                                    { SizeKind_ChildSum, 1.0f },
                                    { SizeKind_MaxChild, 1.0f });
        row->style_vars[StyleVar_Padding] = Vec4(0.0f, 3.0f * s_UIState->DPIScale, 0.0f, 0.0f);
        row->style_vars[StyleVar_ItemSpacing] = Vec4(0.0f);

        PushParent(row);

        // Label
        String8 labelId = PushStr8F(s_UIState->UIFrameArena, "label###inputlabel%s", (char*)text.str);
        u64 hashLabel;
        HandleUIString((char*)labelId.str, &hashLabel);

        UI_Widget* label = PushWidget(WidgetFlags_DrawText | WidgetFlags_CentreY,
                                      text,
                                      hashLabel,
                                      { SizeKind_TextContent, 1.0f },
                                      { SizeKind_TextContent, 1.0f });

        u64 fieldHash = UIHashCombine(row->hash, hash);

        // Build display text with cursor if focused
        bool isFocused     = (s_UIState->FocusedTextInput == fieldHash);

#ifdef LUMOS_PLATFORM_IOS
        // Mirror native-keyboard edits into the caller's buffer.
        // only when the keyboard changed it: the field can also be edited from here (or cleared by its owner)
        if(isFocused && s_NativeKeyboardHash == fieldHash && s_NativeKeyboardText != s_NativeKeyboardSeen)
        {
            s_NativeKeyboardSeen = s_NativeKeyboardText;
            strncpy(buffer, s_NativeKeyboardText.c_str(), buffer_size - 1);
            buffer[buffer_size - 1]    = 0;
            s_UIState->TextInputCursor = (u32)strlen(buffer);
        }
#endif

        String8 displayStr = Str8C(buffer);

        // Input field
        UI_Widget* field = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_DrawText | WidgetFlags_CentreY,
                                      displayStr,
                                      hash,
                                      { SizeKind_Pixels, 150.0f * s_UIState->DPIScale },
                                      { SizeKind_TextContent, 1.0f });

        {float dp = s_UIState->DPIScale;
        field->style_vars[StyleVar_CornerRadius] = Vec4(3.0f * dp, 0.0f, 0.0f, 0.0f);
        field->style_vars[StyleVar_Padding]      = Vec4(5.0f * dp, 3.0f * dp, 0.0f, 0.0f);}

        if(UIThemeIsDark())
        {
            field->style_vars[StyleVar_BackgroundColor]       = Vec4(0.1f, 0.1f, 0.1f, 1.0f);
            field->style_vars[StyleVar_BorderColor]           = isFocused ? Vec4(0.4f, 0.6f, 1.0f, 1.0f) : Vec4(0.4f, 0.4f, 0.4f, 1.0f);
            field->style_vars[StyleVar_HotBorderColor]        = Vec4(0.5f, 0.65f, 0.95f, 1.0f);
            field->style_vars[StyleVar_ActiveBorderColor]     = Vec4(0.4f, 0.55f, 0.9f, 1.0f);
            field->style_vars[StyleVar_ActiveBackgroundColor] = Vec4(0.15f, 0.15f, 0.15f, 1.0f);
        }
        else
        {
            field->style_vars[StyleVar_BackgroundColor]       = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            field->style_vars[StyleVar_BorderColor]           = isFocused ? Vec4(0.4f, 0.6f, 1.0f, 1.0f) : Vec4(0.6f, 0.6f, 0.6f, 1.0f);
            field->style_vars[StyleVar_HotBorderColor]        = Vec4(0.5f, 0.65f, 0.95f, 1.0f);
            field->style_vars[StyleVar_ActiveBorderColor]     = Vec4(0.4f, 0.55f, 0.9f, 1.0f);
            field->style_vars[StyleVar_ActiveBackgroundColor] = Vec4(0.98f, 0.98f, 0.98f, 1.0f);
        }

        PopParent(row);

        // Register as focusable
        s_UIState->FocusableWidgets.PushBack(fieldHash);

        UI_Interaction interaction = HandleWidgetInteraction(field);

        UIScribbleRegister(field, fieldHash, false);

        // Handle click focus
        if(interaction.clicked)
        {
            s_UIState->FocusedTextInput    = fieldHash;
            s_UIState->TextInputBuffer     = buffer;
            s_UIState->TextInputBufferSize = buffer_size;
            s_UIState->TextInputCursor     = (u32)strlen(buffer);
            s_UIState->TextInputSelStart   = 0;
            s_UIState->TextInputSelEnd     = 0;
            s_UIState->FocusIndex = (i32)(s_UIState->FocusableWidgets.Size() - 1);
#ifdef LUMOS_PLATFORM_IOS
            // Tapping a field brings up the system keyboard.
            s_NativeKeyboardText = buffer;
            s_NativeKeyboardSeen = buffer;
            s_NativeKeyboardHash = fieldHash;
            OpeniOSKeyboard(&s_NativeKeyboardText);
#endif
        }

        if(s_ClaimNextTextInputFocus)
        {
            s_ClaimNextTextInputFocus     = false;
            s_UIState->PendingFocusWidget = fieldHash;
        }

        if(s_UIState->PendingFocusWidget == fieldHash)
        {
            s_UIState->PendingFocusWidget  = 0;
            s_UIState->FocusedTextInput    = fieldHash;
            s_UIState->TextInputBuffer     = buffer;
            s_UIState->TextInputBufferSize = buffer_size;
            s_UIState->TextInputCursor     = (u32)strlen(buffer);
            s_UIState->TextInputSelStart   = 0;
            s_UIState->TextInputSelEnd     = (u32)strlen(buffer); // Select all on focus
#ifdef LUMOS_PLATFORM_IOS
            s_NativeKeyboardText = buffer;
            s_NativeKeyboardSeen = buffer;
            s_NativeKeyboardHash = fieldHash;
            OpeniOSKeyboard(&s_NativeKeyboardText);
#endif
        }

        UITextFieldExtras(field, buffer, fieldHash, false, interaction,
                          CaretColour(UIThemeIsDark() ? Vec4(0.95f, 0.95f, 0.97f, 1.0f) : Vec4(0.1f, 0.1f, 0.12f, 1.0f)));

        // Update external cursor if provided
        if(cursor_pos && isFocused)
        {
            *cursor_pos = s_UIState->TextInputCursor;
        }

        return interaction;
    }

    // The owner can replace the text under a focused field (e.g. clearing a search): keep the
    // cursor / selection inside it or edits land past the terminator and vanish
    static void ClampTextCursor()
    {
        u32 len = (u32)strlen(s_UIState->TextInputBuffer);
        s_UIState->TextInputCursor   = Maths::Min(s_UIState->TextInputCursor, len);
        s_UIState->TextInputSelStart = Maths::Min(s_UIState->TextInputSelStart, len);
        s_UIState->TextInputSelEnd   = Maths::Min(s_UIState->TextInputSelEnd, len);
    }

    static void QueueKeyEvent(u16 key, char character)
    {
        if(s_UIState->KeySinkCount >= (u32)ArrayCount(s_UIState->KeySinkEvents))
            return;
        Input& input      = Input::Get();
        UIKeyEvent& event = s_UIState->KeySinkEvents[s_UIState->KeySinkCount++];
        event.Key         = key;
        event.Character   = character;
        event.Shift       = input.GetKeyHeldRaw(InputCode::Key::LeftShift) || input.GetKeyHeldRaw(InputCode::Key::RightShift);
        event.Ctrl        = input.GetKeyHeldRaw(InputCode::Key::LeftControl) || input.GetKeyHeldRaw(InputCode::Key::RightControl);
#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS)
        event.Ctrl = event.Ctrl || input.GetKeyHeldRaw(InputCode::Key::LeftSuper) || input.GetKeyHeldRaw(InputCode::Key::RightSuper);
#endif
        event.Alt = input.GetKeyHeldRaw(InputCode::Key::LeftAlt) || input.GetKeyHeldRaw(InputCode::Key::RightAlt);
    }

    static bool KeySinkFocused()
    {
        return s_UIState->KeySink != 0 && s_UIState->FocusedTextInput == s_UIState->KeySink;
    }

#ifdef LUMOS_PLATFORM_IOS
    extern "C" void iOSSetKeyboardStyle(int style);
    extern "C" float iOSKeyboardCover();
#endif

    // the UI that asked for the on-screen keyboard: only it puts it away
    static UI_State* s_KeyboardOwner = nullptr;
    static u32 s_KeyboardUnfocusedFrames = 0;

    void UIShowKeyboard(bool show)
    {
        if(show)
        {
            s_KeyboardOwner           = s_UIState;
            s_KeyboardUnfocusedFrames = 0;
        }
        else if(s_KeyboardOwner == s_UIState)
            s_KeyboardOwner = nullptr;
        OS::Get().ShowKeyboard(show);
    }

    // The sink that wanted the keyboard lost the keys to something else: put it away.
    // (A few frames' grace: focus asked for this frame lands on the next build.)
    static void UIKeyboardFrameCheck()
    {
        if(s_KeyboardOwner != s_UIState)
            return;
        if(KeySinkFocused())
            s_KeyboardUnfocusedFrames = 0;
        else if(++s_KeyboardUnfocusedFrames > 3)
            UIShowKeyboard(false);
    }

    // ---- handwriting targets ----
    // Where the text fields built this frame are, per UI, for the platform's handwriting
    // (iOS Scribble) to ask about. Rects are in raw input pixels.
    struct ScribbleTarget
    {
        u64 Hash;
        Vec4 Rect; // x y w h
        bool Number;
    };
    struct ScribbleList
    {
        UI_State* Owner = nullptr;
        u32 Count       = 0;
        u64 Wanted      = 0; // a number field asked to switch to typing
        ScribbleTarget Items[96];
    };
    static ScribbleList s_Scribble[4];

    static ScribbleList& ScribbleFor(UI_State* ui)
    {
        for(ScribbleList& list : s_Scribble)
            if(list.Owner == ui)
                return list;
        for(ScribbleList& list : s_Scribble)
            if(!list.Owner)
            {
                list.Owner = ui;
                return list;
            }
        s_Scribble[0].Owner = ui;
        s_Scribble[0].Count = 0;
        return s_Scribble[0];
    }

    static void UIScribbleBeginFrame()
    {
        ScribbleFor(s_UIState).Count = 0;
    }

    void UIScribbleRegister(UI_Widget* widget, u64 hash, bool number)
    {
        ScribbleList& list = ScribbleFor(s_UIState);
        if(!widget || list.Count >= (u32)ArrayCount(list.Items) || widget->size.x < 4.0f || widget->size.y < 4.0f)
            return;
        // what is left of it inside its clip
        const Vec4& clip = widget->ClipRect;
        const f32 x0     = Maths::Max(widget->position.x, clip.x), y0 = Maths::Max(widget->position.y, clip.y);
        const f32 x1     = Maths::Min(widget->position.x + widget->size.x, clip.z), y1 = Maths::Min(widget->position.y + widget->size.y, clip.w);
        if(x1 - x0 < 4.0f || y1 - y0 < 4.0f)
            return;
        const f32 inputScale   = Maths::Max(s_UIState->InputScale, 0.0001f);
        const Vec2 scale       = Vec2(inputScale, inputScale);
        ScribbleTarget& target = list.Items[list.Count++];
        target.Hash            = hash;
        target.Number          = number;
        target.Rect            = Vec4((x0 + s_UIState->InputOffset.x) / scale.x, (y0 + s_UIState->InputOffset.y) / scale.y, (x1 - x0) / scale.x, (y1 - y0) / scale.y);
    }

    bool UIScribbleTake(u64 hash)
    {
        ScribbleList& list = ScribbleFor(s_UIState);
        if(list.Wanted != hash || hash == 0)
            return false;
        list.Wanted = 0;
        return true;
    }

    // for the platform layer (iOSOS.mm)
    extern "C" u32 UIScribbleTargets(u64* hashes, float* rects, u32 maxCount)
    {
        u32 count = 0;
        for(ScribbleList& list : s_Scribble)
            for(u32 i = 0; list.Owner && i < list.Count && count < maxCount; i++)
            {
                hashes[count]        = list.Items[i].Hash;
                rects[count * 4 + 0] = list.Items[i].Rect.x;
                rects[count * 4 + 1] = list.Items[i].Rect.y;
                rects[count * 4 + 2] = list.Items[i].Rect.z;
                rects[count * 4 + 3] = list.Items[i].Rect.w;
                count++;
            }
        return count;
    }

    extern "C" bool UIScribbleIsFocused(u64 hash)
    {
        for(ScribbleList& list : s_Scribble)
            if(list.Owner && hash != 0 && list.Owner->FocusedTextInput == hash)
                return true;
        return false;
    }

    extern "C" void UIScribbleFocus(u64 hash)
    {
        for(ScribbleList& list : s_Scribble)
            for(u32 i = 0; list.Owner && i < list.Count; i++)
                if(list.Items[i].Hash == hash)
                {
                    if(list.Items[i].Number)
                        list.Wanted = hash;
                    else
                        list.Owner->PendingFocusWidget = hash;
                    return;
                }
    }

    static int s_RequestedCursor = -1;

    void UISetCursor(UICursor cursor)
    {
        s_RequestedCursor = (int)cursor;
    }

    int UITakeCursor()
    {
        const int cursor  = s_RequestedCursor;
        s_RequestedCursor = -1;
        return cursor;
    }

    void UISetKeyboardStyle(UIKeyboardStyle style)
    {
#ifdef LUMOS_PLATFORM_IOS
        iOSSetKeyboardStyle((int)style);
#endif
    }

    float UIKeyboardCover()
    {
#ifdef LUMOS_PLATFORM_IOS
        return iOSKeyboardCover();
#else
        return 0.0f;
#endif
    }

    void UISetKeySink(u64 hash)
    {
        // letting go of the keys puts the keyboard away
        if(hash == 0 && s_KeyboardOwner == s_UIState)
            UIShowKeyboard(false);
        s_UIState->KeySink             = hash;
        s_UIState->KeySinkCount        = 0;
        s_UIState->FocusedTextInput    = hash;
        s_UIState->TextInputBuffer     = nullptr;
        s_UIState->TextInputBufferSize = 0;
    }

    bool UIIsKeySink(u64 hash)
    {
        return hash != 0 && s_UIState->KeySink == hash && s_UIState->FocusedTextInput == hash;
    }

    u32 UITakeKeyEvents(u64 hash, UIKeyEvent* out, u32 maxEvents)
    {
        if(!UIIsKeySink(hash))
            return 0;
        const u32 count = Maths::Min(s_UIState->KeySinkCount, maxEvents);
        for(u32 i = 0; i < count; i++)
            out[i] = s_UIState->KeySinkEvents[i];
        s_UIState->KeySinkCount = 0;
        return count;
    }

    void UIProcessKeyTyped(char character)
    {
        if(KeySinkFocused())
        {
            if(character >= 32 && character < 127)
                QueueKeyEvent(0, character);
            return;
        }
        if(s_UIState->FocusedTextInput == 0 || !s_UIState->TextInputBuffer)
            return;
        ClampTextCursor();

        // Printable characters
        if(character >= 32 && character < 127)
        {
            // Delete selection first if any
            if(s_UIState->TextInputSelStart != s_UIState->TextInputSelEnd)
            {
                u32 selStart = Maths::Min(s_UIState->TextInputSelStart, s_UIState->TextInputSelEnd);
                u32 selEnd = Maths::Max(s_UIState->TextInputSelStart, s_UIState->TextInputSelEnd);
                u32 len = (u32)strlen(s_UIState->TextInputBuffer);
                u32 selLen = selEnd - selStart;
                for(u32 i = selStart; i <= len - selLen; i++)
                    s_UIState->TextInputBuffer[i] = s_UIState->TextInputBuffer[i + selLen];
                s_UIState->TextInputCursor = selStart;
                s_UIState->TextInputSelStart = s_UIState->TextInputSelEnd = selStart;
            }

            u32 len = (u32)strlen(s_UIState->TextInputBuffer);
            if(len + 1 < s_UIState->TextInputBufferSize)
            {
                u32 cursor = s_UIState->TextInputCursor;
                // Shift characters after cursor
                for(u32 i = len; i > cursor; i--)
                {
                    s_UIState->TextInputBuffer[i] = s_UIState->TextInputBuffer[i - 1];
                }
                s_UIState->TextInputBuffer[cursor] = character;
                s_UIState->TextInputBuffer[len + 1] = '\0';
                s_UIState->TextInputCursor++;
                s_UIState->TextInputSelStart = s_UIState->TextInputSelEnd = s_UIState->TextInputCursor;
            }
        }
    }

    void UIProcessKeyPressed(InputCode::Key key)
    {
        // a key sink takes everything, Tab included
        if(KeySinkFocused())
        {
            QueueKeyEvent((u16)key, 0);
            return;
        }

        // Tab focus navigation (works even without focused text input)
        if(key == InputCode::Key::Tab)
        {
            // Ctrl+Tab belongs to whoever owns the window (a panel switcher), not to field focus
            if(Input::Get().GetKeyHeldRaw(InputCode::Key::LeftControl) || Input::Get().GetKeyHeldRaw(InputCode::Key::RightControl))
                return;
            if(Input::Get().GetKeyHeldRaw(InputCode::Key::LeftShift) || Input::Get().GetKeyHeldRaw(InputCode::Key::RightShift))
                s_UIState->ShiftTabPressed = true;
            else
                s_UIState->TabPressed = true;
            return;
        }

        if(s_UIState->FocusedTextInput == 0 || !s_UIState->TextInputBuffer)
            return;
        ClampTextCursor();

        u32 len = (u32)strlen(s_UIState->TextInputBuffer);
        bool shift = Input::Get().GetKeyHeldRaw(InputCode::Key::LeftShift) || Input::Get().GetKeyHeldRaw(InputCode::Key::RightShift);
        bool ctrl = Input::Get().GetKeyHeldRaw(InputCode::Key::LeftControl) || Input::Get().GetKeyHeldRaw(InputCode::Key::RightControl);
#ifdef LUMOS_PLATFORM_MACOS
        ctrl = ctrl || Input::Get().GetKeyHeldRaw(InputCode::Key::LeftSuper) || Input::Get().GetKeyHeldRaw(InputCode::Key::RightSuper);
#endif

        // Helper to delete selection
        auto deleteSelection = [&]() -> bool {
            if(s_UIState->TextInputSelStart == s_UIState->TextInputSelEnd)
                return false;
            u32 selStart = Maths::Min(s_UIState->TextInputSelStart, s_UIState->TextInputSelEnd);
            u32 selEnd = Maths::Max(s_UIState->TextInputSelStart, s_UIState->TextInputSelEnd);
            u32 selLen = selEnd - selStart;
            for(u32 i = selStart; i <= len - selLen; i++)
                s_UIState->TextInputBuffer[i] = s_UIState->TextInputBuffer[i + selLen];
            s_UIState->TextInputCursor = selStart;
            s_UIState->TextInputSelStart = s_UIState->TextInputSelEnd = selStart;
            return true;
        };

        // Helper for selection with shift
        auto updateSelection = [&](u32 newCursor) {
            if(shift)
            {
                if(s_UIState->TextInputSelStart == s_UIState->TextInputSelEnd)
                    s_UIState->TextInputSelStart = s_UIState->TextInputCursor;
                s_UIState->TextInputSelEnd = newCursor;
            }
            else
            {
                s_UIState->TextInputSelStart = s_UIState->TextInputSelEnd = newCursor;
            }
            s_UIState->TextInputCursor = newCursor;
        };

        // word / line modifiers: Alt = word and Cmd = line on macOS, Ctrl = word elsewhere
        const bool alt = Input::Get().GetKeyHeldRaw(InputCode::Key::LeftAlt) || Input::Get().GetKeyHeldRaw(InputCode::Key::RightAlt);
#ifdef LUMOS_PLATFORM_MACOS
        const bool lineMod = Input::Get().GetKeyHeldRaw(InputCode::Key::LeftSuper) || Input::Get().GetKeyHeldRaw(InputCode::Key::RightSuper);
        const bool wordMod = alt;
#else
        const bool lineMod = false;
        const bool wordMod = ctrl;
#endif
        char* buf = s_UIState->TextInputBuffer;
        auto deleteRange = [&](u32 a, u32 b) {
            if(b <= a) return;
            memmove(buf + a, buf + b, len - b + 1);
            s_UIState->TextInputCursor   = a;
            s_UIState->TextInputSelStart = s_UIState->TextInputSelEnd = a;
        };
        const u32 cur        = s_UIState->TextInputCursor;
        const bool hasSel    = s_UIState->TextInputSelStart != s_UIState->TextInputSelEnd;
        const u32 selLo      = Maths::Min(s_UIState->TextInputSelStart, s_UIState->TextInputSelEnd);
        const u32 selHi      = Maths::Max(s_UIState->TextInputSelStart, s_UIState->TextInputSelEnd);

        switch(key)
        {
        case InputCode::Key::Backspace:
            if(deleteSelection())
                break;
            if(lineMod)
                deleteRange(0, cur);
            else if(wordMod)
                deleteRange(PrevWordIndex(buf, cur), cur);
            else if(cur > 0)
            {
                u32 a = cur - 1;
                while(a > 0 && ((u8)buf[a] & 0xC0) == 0x80) a--; // whole UTF-8 character
                deleteRange(a, cur);
            }
            break;

        case InputCode::Key::Delete:
            if(deleteSelection())
                break;
            if(lineMod)
                deleteRange(cur, len);
            else if(wordMod)
                deleteRange(cur, NextWordIndex(buf, cur, len));
            else if(cur < len)
            {
                u32 b = cur + 1;
                while(b < len && ((u8)buf[b] & 0xC0) == 0x80) b++;
                deleteRange(cur, b);
            }
            break;

        case InputCode::Key::Left:
            if(hasSel && !shift)
                updateSelection(selLo);
            else if(lineMod)
                updateSelection(0);
            else if(wordMod)
                updateSelection(PrevWordIndex(buf, cur));
            else if(cur > 0)
            {
                u32 a = cur - 1;
                while(a > 0 && ((u8)buf[a] & 0xC0) == 0x80) a--;
                updateSelection(a);
            }
            break;

        case InputCode::Key::Right:
            if(hasSel && !shift)
                updateSelection(selHi);
            else if(lineMod)
                updateSelection(len);
            else if(wordMod)
                updateSelection(NextWordIndex(buf, cur, len));
            else if(cur < len)
            {
                u32 b = cur + 1;
                while(b < len && ((u8)buf[b] & 0xC0) == 0x80) b++;
                updateSelection(b);
            }
            break;

        case InputCode::Key::Up:
            updateSelection(0);
            break;

        case InputCode::Key::Down:
            updateSelection(len);
            break;

        case InputCode::Key::Home:
            updateSelection(0);
            break;

        case InputCode::Key::End:
            updateSelection(len);
            break;

        case InputCode::Key::A:
            if(ctrl) // Select all
            {
                s_UIState->TextInputSelStart = 0;
                s_UIState->TextInputSelEnd = len;
                s_UIState->TextInputCursor = len;
            }
            break;

        case InputCode::Key::C:
            if(ctrl && s_UIState->TextInputSelStart != s_UIState->TextInputSelEnd)
            {
                // Copy to clipboard
                u32 selStart = Maths::Min(s_UIState->TextInputSelStart, s_UIState->TextInputSelEnd);
                u32 selEnd = Maths::Max(s_UIState->TextInputSelStart, s_UIState->TextInputSelEnd);
                char temp[256];
                u32 copyLen = Maths::Min(selEnd - selStart, (u32)255);
                memcpy(temp, s_UIState->TextInputBuffer + selStart, copyLen);
                temp[copyLen] = '\0';
                Input::Get().SetClipboard(temp);
            }
            break;

        case InputCode::Key::X:
            if(ctrl && s_UIState->TextInputSelStart != s_UIState->TextInputSelEnd)
            {
                // Cut: copy then delete
                u32 selStart = Maths::Min(s_UIState->TextInputSelStart, s_UIState->TextInputSelEnd);
                u32 selEnd = Maths::Max(s_UIState->TextInputSelStart, s_UIState->TextInputSelEnd);
                char temp[256];
                u32 copyLen = Maths::Min(selEnd - selStart, (u32)255);
                memcpy(temp, s_UIState->TextInputBuffer + selStart, copyLen);
                temp[copyLen] = '\0';
                Input::Get().SetClipboard(temp);
                deleteSelection();
            }
            break;

        case InputCode::Key::V:
            if(ctrl)
            {
                // Paste from clipboard
                deleteSelection();
                std::string clipboard = Input::Get().GetClipboard();
                u32 pasteLen = (u32)clipboard.length();
                u32 newLen = len + pasteLen;
                if(newLen < s_UIState->TextInputBufferSize)
                {
                    u32 cursor = s_UIState->TextInputCursor;
                    // Shift existing text
                    for(u32 i = len; i >= cursor && i != (u32)-1; i--)
                        s_UIState->TextInputBuffer[i + pasteLen] = s_UIState->TextInputBuffer[i];
                    // Insert paste
                    memcpy(s_UIState->TextInputBuffer + cursor, clipboard.c_str(), pasteLen);
                    s_UIState->TextInputCursor += pasteLen;
                }
            }
            break;

        case InputCode::Key::Escape:
        case InputCode::Key::Enter:
            s_UIState->FocusedTextInput    = 0;
            s_UIState->TextInputBuffer     = nullptr;
            s_UIState->TextInputBufferSize = 0;
            s_UIState->TextInputSelStart   = 0;
            s_UIState->TextInputSelEnd     = 0;
            break;

        default:
            break;
        }
    }

    UI_Interaction UIDropdown(const char* str, int* selected_index, const char** options, int option_count)
    {
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        String8 rowText = PushStr8F(s_UIState->UIFrameArena, "droprow###droprow%s", (char*)text.str);
        u64 hashRow;
        HandleUIString((char*)rowText.str, &hashRow);

        UI_Widget* row = PushWidget(WidgetFlags_StackHorizontally,
                                    Str8Lit(""),
                                    hashRow,
                                    { SizeKind_ChildSum, 1.0f },
                                    { SizeKind_MaxChild, 1.0f });
        row->style_vars[StyleVar_Padding] = Vec4(0.0f, 3.0f * s_UIState->DPIScale, 0.0f, 0.0f);
        row->style_vars[StyleVar_ItemSpacing] = Vec4(0.0f);

        PushParent(row);

        // Label
        String8 labelId = PushStr8F(s_UIState->UIFrameArena, "label###droplabel%s", (char*)text.str);
        u64 hashLabel;
        HandleUIString((char*)labelId.str, &hashLabel);

        PushWidget(WidgetFlags_DrawText | WidgetFlags_CentreY,
                   text,
                   hashLabel,
                   { SizeKind_TextContent, 1.0f },
                   { SizeKind_TextContent, 1.0f });

        // Current selection display
        const char* currentOption = (*selected_index >= 0 && *selected_index < option_count)
                                        ? options[*selected_index]
                                        : "Select...";

        String8 buttonText = PushStr8F(s_UIState->UIFrameArena, "%s", currentOption);

        UI_Widget* button = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_DrawText | WidgetFlags_CentreY,
                                       buttonText,
                                       hash,
                                       { SizeKind_Pixels, 150.0f * s_UIState->DPIScale },
                                       { SizeKind_TextContent, 1.0f });

        {float dp = s_UIState->DPIScale;
        button->style_vars[StyleVar_CornerRadius] = Vec4(3.0f * dp, 0.0f, 0.0f, 0.0f);
        button->style_vars[StyleVar_Padding]      = Vec4(5.0f * dp, 3.0f * dp, 0.0f, 0.0f);}

        bool isOpen = (s_UIState->OpenDropdown == button->hash);

        if(UIThemeIsDark())
        {
            button->style_vars[StyleVar_BackgroundColor]       = Vec4(0.2f, 0.2f, 0.2f, 1.0f);
            button->style_vars[StyleVar_BorderColor]           = isOpen ? Vec4(0.4f, 0.6f, 1.0f, 1.0f) : Vec4(0.4f, 0.4f, 0.4f, 1.0f);
            button->style_vars[StyleVar_HotBackgroundColor]    = Vec4(0.25f, 0.25f, 0.25f, 1.0f);
            button->style_vars[StyleVar_ActiveBackgroundColor] = Vec4(0.3f, 0.3f, 0.3f, 1.0f);
        }
        else
        {
            button->style_vars[StyleVar_BackgroundColor]       = Vec4(0.95f, 0.95f, 0.95f, 1.0f);
            button->style_vars[StyleVar_BorderColor]           = isOpen ? Vec4(0.4f, 0.6f, 1.0f, 1.0f) : Vec4(0.6f, 0.6f, 0.6f, 1.0f);
            button->style_vars[StyleVar_HotBackgroundColor]    = Vec4(0.9f, 0.9f, 0.9f, 1.0f);
            button->style_vars[StyleVar_ActiveBackgroundColor] = Vec4(0.85f, 0.85f, 0.85f, 1.0f);
        }

        PopParent(row);

        UI_Interaction interaction = HandleWidgetInteraction(button);

        if(interaction.clicked)
        {
            if(isOpen)
                s_UIState->OpenDropdown = 0;
            else
                s_UIState->OpenDropdown = button->hash;

            interaction.clicked = false;
        }

        // Draw dropdown list if open - push to root for correct z-order
        if(isOpen)
        {
            // Save current parent stack and push to root so list renders on top
            UI_Widget* savedParent = s_UIState->parents.Back();
            s_UIState->parents.PushBack(&s_UIState->root_parent);

            String8 listId = PushStr8F(s_UIState->UIFrameArena, "list###droplist%s", (char*)text.str);
            u64 hashList;
            HandleUIString((char*)listId.str, &hashList);

            UI_Widget* list = PushWidget(WidgetFlags_StackVertically | WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | WidgetFlags_TopMost,
                                         Str8Lit(""),
                                         hashList,
                                         { SizeKind_MaxChild, 1.0f },
                                         { SizeKind_ChildSum, 1.0f });

            const Vec2& rootPos = s_UIState->root_parent.position;
            list->relative_position.x = button->position.x - rootPos.x;
            list->relative_position.y = button->position.y + button->size.y - rootPos.y;

            {float dp = s_UIState->DPIScale;
            list->style_vars[StyleVar_CornerRadius] = Vec4(3.0f * dp, 0.0f, 0.0f, 0.0f);
            list->style_vars[StyleVar_Padding]      = Vec4(1.0f * dp, 1.0f * dp, 0.0f, 0.0f);}

            if(UIThemeIsDark())
            {
                list->style_vars[StyleVar_BackgroundColor] = Vec4(0.18f, 0.18f, 0.18f, 1.0f);
                list->style_vars[StyleVar_BorderColor]     = Vec4(0.4f, 0.4f, 0.4f, 1.0f);
            }
            else
            {
                list->style_vars[StyleVar_BackgroundColor] = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
                list->style_vars[StyleVar_BorderColor]     = Vec4(0.6f, 0.6f, 0.6f, 1.0f);
            }

            PushParent(list);

            for(int i = 0; i < option_count; i++)
            {
                String8 optionId = PushStr8F(s_UIState->UIFrameArena, "opt%d###opt%d%s", i, i, (char*)text.str);
                u64 hashOption;
                HandleUIString((char*)optionId.str, &hashOption);

                bool isSelected = (i == *selected_index);

                UI_Widget* option = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground | WidgetFlags_DrawText,
                                               Str8C((char*)options[i]),
                                               hashOption,
                                               { SizeKind_TextContent, 1.0f },
                                               { SizeKind_TextContent, 1.0f });

                option->style_vars[StyleVar_Padding] = Vec4(4.0f * s_UIState->DPIScale, 2.0f * s_UIState->DPIScale, 0.0f, 0.0f);

                if(UIThemeIsDark())
                {
                    option->style_vars[StyleVar_BackgroundColor]    = isSelected ? Vec4(0.3f, 0.5f, 0.8f, 1.0f) : Vec4(0.18f, 0.18f, 0.18f, 0.0f);
                    option->style_vars[StyleVar_HotBackgroundColor] = Vec4(0.28f, 0.28f, 0.28f, 1.0f);
                }
                else
                {
                    option->style_vars[StyleVar_BackgroundColor]    = isSelected ? Vec4(0.3f, 0.5f, 0.8f, 1.0f) : Vec4(1.0f, 1.0f, 1.0f, 0.0f);
                    option->style_vars[StyleVar_HotBackgroundColor] = Vec4(0.9f, 0.9f, 0.9f, 1.0f);
                }

                UI_Interaction optInteraction = HandleWidgetInteraction(option);
                if(optInteraction.clicked)
                {
                    *selected_index         = i;
                    s_UIState->OpenDropdown = 0;
                    interaction.clicked     = true;
                }
            }

            PopParent(list);

            // Block input to widgets behind the dropdown overlay
            Vec2 mouse = Input::Get().GetMousePosition() * s_UIState->InputScale - s_UIState->InputOffset;
            if(mouse.x >= list->position.x && mouse.x <= list->position.x + list->size.x &&
               mouse.y >= list->position.y && mouse.y <= list->position.y + list->size.y)
            {
                s_UIState->OverlayBlocksInput = true;
            }

            // Restore parent stack (remove the root we pushed)
            s_UIState->parents.PopBack();
        }

        return interaction;
    }

    void UITooltip(const char* text)
    {
        // Get the last created widget's hash (the widget this tooltip is for)
        UI_Widget* parent = s_UIState->parents.Back();
        UI_Widget* lastWidget = parent ? parent->last : nullptr;
        u64 targetWidget = lastWidget ? lastWidget->hash : 0;

        // Only show tooltip if hovering the target widget
        if(s_UIState->hot_widget == targetWidget && targetWidget != 0)
        {
            if(s_UIState->HoveredWidget != targetWidget)
            {
                s_UIState->HoveredWidget  = targetWidget;
                s_UIState->HoverStartTime = s_UIState->CurrentTime;
                s_UIState->ShowTooltip    = false;
            }

            float hoverDuration = s_UIState->CurrentTime - s_UIState->HoverStartTime;
            if(hoverDuration >= s_UIState->TooltipDelay)
            {
                s_UIState->ShowTooltip = true;
                size_t n = 0;
                for(; text[n] && n < sizeof(s_UIState->TooltipBuffer) - 1; n++)
                    s_UIState->TooltipBuffer[n] = text[n];
                s_UIState->TooltipBuffer[n] = 0;
                s_UIState->TooltipText      = Str8((u8*)s_UIState->TooltipBuffer, n);
            }
        }
        else if(s_UIState->HoveredWidget == targetWidget)
        {
            // No longer hovering target - reset
            s_UIState->ShowTooltip = false;
        }

        if(s_UIState->ShowTooltip && s_UIState->TooltipText.size > 0 && s_UIState->hot_widget == targetWidget)
        {
            // Store position for deferred creation in UIEndBuild
            s_UIState->TooltipPos      = Input::Get().GetMousePosition() * s_UIState->InputScale - s_UIState->InputOffset;
            s_UIState->TooltipFontSize = s_UIState->style_variable_lists[StyleVar_FontSize].last->value.x;
        }
    }

    // ============================================
    // Context Menu
    // ============================================

    static UI_Widget* s_ContextMenuWidget = nullptr;

    bool UIBeginContextMenu(const char* str)
    {
        if(!s_UIState->ContextMenuOpen)
            return false;

        u64 hash;
        String8 text = HandleUIString(str, &hash);

        s_UIState->parents.PushBack(&s_UIState->root_parent);

        UI_Widget* menu = PushWidget(WidgetFlags_StackVertically | WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | WidgetFlags_TopMost,
                                     text,
                                     hash,
                                     { SizeKind_MaxChild, 1.0f },
                                     { SizeKind_ChildSum, 1.0f });

        menu->relative_position = s_UIState->ContextMenuPos - s_UIState->root_parent.position;
        {float dp = s_UIState->DPIScale;
        menu->style_vars[StyleVar_Padding] = Vec4(2.0f * dp, 2.0f * dp, 0.0f, 0.0f);
        menu->style_vars[StyleVar_CornerRadius] = Vec4(3.0f * dp, 0.0f, 0.0f, 0.0f);}

        if(UIThemeIsDark())
        {
            menu->style_vars[StyleVar_BackgroundColor] = Vec4(0.15f, 0.15f, 0.15f, 0.98f);
            menu->style_vars[StyleVar_BorderColor] = Vec4(0.4f, 0.4f, 0.4f, 1.0f);
        }
        else
        {
            menu->style_vars[StyleVar_BackgroundColor] = Vec4(0.98f, 0.98f, 0.98f, 0.98f);
            menu->style_vars[StyleVar_BorderColor] = Vec4(0.6f, 0.6f, 0.6f, 1.0f);
        }

        s_ContextMenuWidget = menu;
        PushParent(menu);
        return true;
    }

    void UIEndContextMenu()
    {
        if(s_ContextMenuWidget)
        {
            PopParent(s_ContextMenuWidget);
            s_UIState->parents.PopBack(); // root pushed by UIBeginContextMenu
            s_ContextMenuWidget = nullptr;
        }
    }

    UI_Interaction UIContextMenuItem(const char* str)
    {
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        UI_Widget* item = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawText | WidgetFlags_DrawBackground,
                                     text,
                                     hash,
                                     { SizeKind_TextContent, 1.0f },
                                     { SizeKind_TextContent, 1.0f });

        item->style_vars[StyleVar_Padding] = Vec4(8.0f * s_UIState->DPIScale, 3.0f * s_UIState->DPIScale, 0.0f, 0.0f);
        item->TextAlignment = UI_Text_Alignment_Center_Y;

        if(UIThemeIsDark())
        {
            item->style_vars[StyleVar_BackgroundColor] = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
            item->style_vars[StyleVar_HotBackgroundColor] = Vec4(0.3f, 0.5f, 0.8f, 1.0f);
            item->style_vars[StyleVar_TextColor] = Vec4(0.9f, 0.9f, 0.9f, 1.0f);
        }
        else
        {
            item->style_vars[StyleVar_BackgroundColor] = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
            item->style_vars[StyleVar_HotBackgroundColor] = Vec4(0.2f, 0.4f, 0.8f, 1.0f);
            item->style_vars[StyleVar_HotTextColor] = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            item->style_vars[StyleVar_TextColor] = Vec4(0.1f, 0.1f, 0.1f, 1.0f);
        }

        UI_Interaction interaction = HandleWidgetInteraction(item);
        if(interaction.clicked)
        {
            s_UIState->ContextMenuOpen = false;
        }
        return interaction;
    }

    // ============================================
    // Tab Bar
    // ============================================

    static UI_Widget* s_TabBarWidget = nullptr;
    static int s_TabIndex = 0;

    bool UIBeginTabBar(const char* str)
    {
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        UI_Widget* tabBar = PushWidget(WidgetFlags_StackHorizontally | WidgetFlags_DrawBackground,
                                       text,
                                       hash,
                                       { SizeKind_PercentOfParent, 1.0f },
                                       { SizeKind_ChildSum, 1.0f });

        tabBar->style_vars[StyleVar_Padding] = Vec4(0.0f, 3.0f * s_UIState->DPIScale, 0.0f, 0.0f);
        tabBar->style_vars[StyleVar_ItemSpacing] = Vec4(1.0f * s_UIState->DPIScale, 0.0f, 0.0f, 0.0f);

        if(UIThemeIsDark())
            tabBar->style_vars[StyleVar_BackgroundColor] = Vec4(0.12f, 0.12f, 0.12f, 1.0f);
        else
            tabBar->style_vars[StyleVar_BackgroundColor] = Vec4(0.9f, 0.9f, 0.9f, 1.0f);

        s_TabBarWidget = tabBar;
        s_TabIndex = 0;
        PushParent(tabBar);
        return true;
    }

    void UIEndTabBar()
    {
        if(s_TabBarWidget)
        {
            PopParent(s_TabBarWidget);
            s_TabBarWidget = nullptr;
        }
    }

    bool UITabItem(const char* str, bool* open)
    {
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        int thisTabIndex = s_TabIndex++;
        bool isActive = s_TabBarWidget && (thisTabIndex == (int)s_TabBarWidget->StateValue);

        UI_Widget* tab = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawText | WidgetFlags_DrawBackground | WidgetFlags_DrawBorder,
                                    text,
                                    hash,
                                    { SizeKind_TextContent, 1.0f },
                                    { SizeKind_TextContent, 1.0f });

        tab->style_vars[StyleVar_Padding] = Vec4(10.0f * s_UIState->DPIScale, 5.0f * s_UIState->DPIScale, 0.0f, 0.0f);
        tab->TextAlignment = UI_Text_Alignment_Center_X | UI_Text_Alignment_Center_Y;

        if(UIThemeIsDark())
        {
            if(isActive)
            {
                tab->style_vars[StyleVar_BackgroundColor] = Vec4(0.2f, 0.2f, 0.2f, 1.0f);
                tab->style_vars[StyleVar_BorderColor] = Vec4(0.4f, 0.6f, 0.9f, 1.0f);
            }
            else
            {
                tab->style_vars[StyleVar_BackgroundColor] = Vec4(0.15f, 0.15f, 0.15f, 1.0f);
                tab->style_vars[StyleVar_BorderColor] = Vec4(0.3f, 0.3f, 0.3f, 0.0f);
                tab->style_vars[StyleVar_HotBackgroundColor] = Vec4(0.18f, 0.18f, 0.18f, 1.0f);
            }
            tab->style_vars[StyleVar_TextColor] = Vec4(0.9f, 0.9f, 0.9f, 1.0f);
        }
        else
        {
            if(isActive)
            {
                tab->style_vars[StyleVar_BackgroundColor] = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
                tab->style_vars[StyleVar_BorderColor] = Vec4(0.3f, 0.5f, 0.8f, 1.0f);
            }
            else
            {
                tab->style_vars[StyleVar_BackgroundColor] = Vec4(0.92f, 0.92f, 0.92f, 1.0f);
                tab->style_vars[StyleVar_BorderColor] = Vec4(0.7f, 0.7f, 0.7f, 0.0f);
                tab->style_vars[StyleVar_HotBackgroundColor] = Vec4(0.95f, 0.95f, 0.95f, 1.0f);
            }
            tab->style_vars[StyleVar_TextColor] = Vec4(0.1f, 0.1f, 0.1f, 1.0f);
        }

        UI_Interaction interaction = HandleWidgetInteraction(tab);
        if(interaction.clicked && s_TabBarWidget)
        {
            s_TabBarWidget->StateValue = (u32)thisTabIndex;
        }

        if(open)
            *open = isActive;

        return isActive;
    }

    // ============================================
    // Modal Dialog
    // ============================================

    static UI_Widget* s_ModalWidget = nullptr;
    static UI_Widget* s_ModalOverlay = nullptr;

    bool UIBeginModal(const char* str, bool* open)
    {
        if(!open || !*open)
            return false;

        u64 hash;
        String8 text = HandleUIString(str, &hash);

        s_UIState->parents.PushBack(&s_UIState->root_parent);

        // Dark overlay behind modal
        String8 overlayId = PushStr8F(s_UIState->UIFrameArena, "modaloverlay###mo%llu", hash);
        u64 overlayHash;
        HandleUIString((char*)overlayId.str, &overlayHash);

        s_ModalOverlay = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                    Str8Lit(""),
                                    overlayHash,
                                    { SizeKind_Pixels, s_UIState->root_parent.size.x },
                                    { SizeKind_Pixels, s_UIState->root_parent.size.y });

        s_ModalOverlay->relative_position = Vec2(0.0f, 0.0f);
        s_ModalOverlay->style_vars[StyleVar_Padding]         = Vec4(0.0f);
        s_ModalOverlay->style_vars[StyleVar_BackgroundColor] = Vec4(0.0f, 0.0f, 0.0f, 0.5f);
        s_ModalOverlay->style_vars[StyleVar_CornerRadius]    = Vec4(0.0f);

        // Close modal when clicking overlay
        UI_Interaction overlayInteraction = HandleWidgetInteraction(s_ModalOverlay);
        if(overlayInteraction.clicked)
        {
            *open = false;
            s_UIState->parents.PopBack(); // root
            s_ModalOverlay = nullptr;
            return false;
        }

        s_ModalWidget = PushWidget(WidgetFlags_StackVertically | WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | WidgetFlags_CentreX | WidgetFlags_CentreY,
                                   text,
                                   hash,
                                   { SizeKind_ChildSum, 1.0f },
                                   { SizeKind_ChildSum, 1.0f });

        {float dp = s_UIState->DPIScale;
        s_ModalWidget->style_vars[StyleVar_Padding] = Vec4(14.0f * dp, 14.0f * dp, 0.0f, 0.0f);
        s_ModalWidget->style_vars[StyleVar_CornerRadius] = Vec4(6.0f * dp, 0.0f, 0.0f, 0.0f);}

        if(UIThemeIsDark())
        {
            s_ModalWidget->style_vars[StyleVar_BackgroundColor] = Vec4(0.18f, 0.18f, 0.18f, 1.0f);
            s_ModalWidget->style_vars[StyleVar_BorderColor] = Vec4(0.4f, 0.4f, 0.4f, 1.0f);
        }
        else
        {
            s_ModalWidget->style_vars[StyleVar_BackgroundColor] = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            s_ModalWidget->style_vars[StyleVar_BorderColor] = Vec4(0.6f, 0.6f, 0.6f, 1.0f);
        }

        PushParent(s_ModalWidget);
        return true;
    }

    void UIEndModal()
    {
        if(s_ModalWidget)
        {
            PopParent(s_ModalWidget);
            s_UIState->parents.PopBack(); // root pushed by UIBeginModal
            s_ModalWidget = nullptr;
            s_ModalOverlay = nullptr;
            s_UIState->OverlayBlocksInput = true;
        }
    }

    // ============================================
    // Tree View
    // ============================================

    static TDArray<UI_Widget*> s_TreeNodeStack;

    bool UITreeNode(const char* str, bool* expanded)
    {
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        float dp = s_UIState->DPIScale;
        float indent = s_UIState->TreeIndentLevel * 20.0f * dp;

        // Row container
        String8 rowId = PushStr8F(s_UIState->UIFrameArena, "treerow###tr%llu", hash);
        u64 rowHash;
        HandleUIString((char*)rowId.str, &rowHash);

        UI_Widget* row = PushWidget(WidgetFlags_StackHorizontally | WidgetFlags_Clickable,
                                    Str8Lit(""),
                                    rowHash,
                                    { SizeKind_PercentOfParent, 1.0f },
                                    { SizeKind_ChildSum, 1.0f });

        row->style_vars[StyleVar_Padding] = Vec4(indent, 0.0f, 0.0f, 0.0f);
        PushParent(row);

        // Arrow/expander
        bool isExpanded = expanded ? *expanded : false;
        String8 arrowText = isExpanded ? Str8Lit("v") : Str8Lit(">");

        String8 arrowId = PushStr8F(s_UIState->UIFrameArena, "%s###arrow%llu", (char*)arrowText.str, hash);
        u64 arrowHash;
        HandleUIString((char*)arrowId.str, &arrowHash);

        UI_Widget* arrow = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawText,
                                      arrowText,
                                      arrowHash,
                                      { SizeKind_Pixels, 20.0f * dp },
                                      { SizeKind_TextContent, 1.0f });

        arrow->style_vars[StyleVar_Padding] = Vec4(2.0f * dp, 2.0f * dp, 0.0f, 0.0f);
        arrow->TextAlignment = UI_Text_Alignment_Center_Y;

        if(UIThemeIsDark())
            arrow->style_vars[StyleVar_TextColor] = Vec4(0.7f, 0.7f, 0.7f, 1.0f);
        else
            arrow->style_vars[StyleVar_TextColor] = Vec4(0.4f, 0.4f, 0.4f, 1.0f);

        UI_Interaction arrowInteraction = HandleWidgetInteraction(arrow);

        // Label
        UI_Widget* label = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawText | WidgetFlags_DrawBackground,
                                      text,
                                      hash,
                                      { SizeKind_TextContent, 1.0f },
                                      { SizeKind_TextContent, 1.0f });

        label->style_vars[StyleVar_Padding] = Vec4(4.0f * dp, 2.0f * dp, 0.0f, 0.0f);
        label->TextAlignment = UI_Text_Alignment_Center_Y;

        if(UIThemeIsDark())
        {
            label->style_vars[StyleVar_BackgroundColor] = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
            label->style_vars[StyleVar_HotBackgroundColor] = Vec4(0.25f, 0.25f, 0.25f, 1.0f);
            label->style_vars[StyleVar_TextColor] = Vec4(0.9f, 0.9f, 0.9f, 1.0f);
        }
        else
        {
            label->style_vars[StyleVar_BackgroundColor] = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
            label->style_vars[StyleVar_HotBackgroundColor] = Vec4(0.85f, 0.85f, 0.85f, 1.0f);
            label->style_vars[StyleVar_TextColor] = Vec4(0.1f, 0.1f, 0.1f, 1.0f);
        }

        UI_Interaction labelInteraction = HandleWidgetInteraction(label);

        PopParent(row);

        // Toggle expanded on click
        if(arrowInteraction.clicked || labelInteraction.clicked)
        {
            if(expanded)
            {
                *expanded = !*expanded;
                isExpanded = *expanded;
            }
        }

        if(isExpanded)
        {
            s_UIState->TreeIndentLevel++;
            s_TreeNodeStack.PushBack(row);
        }

        return isExpanded;
    }

    void UITreePop()
    {
        if(s_UIState->TreeIndentLevel > 0)
        {
            s_UIState->TreeIndentLevel--;
            if(s_TreeNodeStack.Size() > 0)
                s_TreeNodeStack.PopBack();
        }
    }

    // ============================================
    // Color Picker
    // ============================================

    static void RGBtoHSV(float r, float g, float b, float& h, float& s, float& v)
    {
        float maxC = Maths::Max(r, Maths::Max(g, b));
        float minC = Maths::Min(r, Maths::Min(g, b));
        float delta = maxC - minC;

        v = maxC;
        s = (maxC > 0.0f) ? (delta / maxC) : 0.0f;

        if(delta < 0.00001f)
        {
            h = 0.0f;
        }
        else if(maxC == r)
        {
            h = 60.0f * fmodf((g - b) / delta, 6.0f);
        }
        else if(maxC == g)
        {
            h = 60.0f * ((b - r) / delta + 2.0f);
        }
        else
        {
            h = 60.0f * ((r - g) / delta + 4.0f);
        }

        if(h < 0.0f) h += 360.0f;
    }

    static void HSVtoRGB(float h, float s, float v, float& r, float& g, float& b)
    {
        float c = v * s;
        float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
        float m = v - c;

        if(h < 60.0f)      { r = c; g = x; b = 0; }
        else if(h < 120.0f) { r = x; g = c; b = 0; }
        else if(h < 180.0f) { r = 0; g = c; b = x; }
        else if(h < 240.0f) { r = 0; g = x; b = c; }
        else if(h < 300.0f) { r = x; g = 0; b = c; }
        else               { r = c; g = 0; b = x; }

        r += m; g += m; b += m;
    }

    bool UIColorEdit3(const char* str, float* rgb)
    {
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        bool changed = false;
        float dp     = s_UIState->DPIScale;

        // Header row: label + color preview
        String8 rowId = PushStr8F(s_UIState->UIFrameArena, "colorow###cr%llu", hash);
        u64 rowHash;
        HandleUIString((char*)rowId.str, &rowHash);

        UI_Widget* row = PushWidget(WidgetFlags_StackHorizontally,
                                    Str8Lit(""),
                                    rowHash,
                                    { SizeKind_PercentOfParent, 1.0f },
                                    { SizeKind_ChildSum, 1.0f });
        row->style_vars[StyleVar_Padding]      = Vec4(0.0f, 3.0f * s_UIState->DPIScale, 0.0f, 0.0f);
        row->style_vars[StyleVar_ItemSpacing]  = Vec4(0.0f);
        PushParent(row);

        UI_Widget* label = PushWidget(WidgetFlags_DrawText | WidgetFlags_CentreY,
                                      text, hash,
                                      { SizeKind_TextContent, 1.0f },
                                      { SizeKind_TextContent, 1.0f });
        label->style_vars[StyleVar_Padding]   = Vec4(4.0f * dp, 2.0f * dp, 0.0f, 0.0f);
        label->style_vars[StyleVar_TextColor] = (UIThemeIsDark())
                                                    ? Vec4(0.9f, 0.9f, 0.9f, 1.0f)
                                                    : Vec4(0.1f, 0.1f, 0.1f, 1.0f);

        String8 previewId = PushStr8F(s_UIState->UIFrameArena, "colorpreview###cp%llu", hash);
        u64 previewHash;
        HandleUIString((char*)previewId.str, &previewHash);

        UI_Widget* preview = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_CentreY,
                                        Str8Lit(""), previewHash,
                                        { SizeKind_Pixels, 30.0f * dp },
                                        { SizeKind_Pixels, 20.0f * dp });
        preview->style_vars[StyleVar_BackgroundColor] = Vec4(rgb[0], rgb[1], rgb[2], 1.0f);
        preview->style_vars[StyleVar_BorderColor]     = Vec4(0.5f, 0.5f, 0.5f, 1.0f);
        preview->style_vars[StyleVar_CornerRadius]    = Vec4(3.0f * dp, 0.0f, 0.0f, 0.0f);

        PopParent(row);

        // Convert RGB -> HSV for editing
        float h, s, v;
        RGBtoHSV(rgb[0], rgb[1], rgb[2], h, s, v);

        float hPrev = h, sPrev = s, vPrev = v;

        String8 hId = PushStr8F(s_UIState->UIFrameArena, "H###h%llu", hash);
        String8 sId = PushStr8F(s_UIState->UIFrameArena, "S###s%llu", hash);
        String8 vId = PushStr8F(s_UIState->UIFrameArena, "V###v%llu", hash);

        UISlider((char*)hId.str, &h, 0.0f, 360.0f, 80.0f, 16.0f, 0.05f);
        UISlider((char*)sId.str, &s, 0.0f,   1.0f, 80.0f, 16.0f, 0.05f);
        UISlider((char*)vId.str, &v, 0.0f,   1.0f, 80.0f, 16.0f, 0.05f);

        if(h != hPrev || s != sPrev || v != vPrev)
        {
            HSVtoRGB(h, s, v, rgb[0], rgb[1], rgb[2]);
            changed = true;
        }

        return changed;
    }

    bool UIColorEdit4(const char* str, float* rgba)
    {
        bool changed = UIColorEdit3(str, rgba);

        u64 hash;
        String8 text = HandleUIString(str, &hash);

        String8 aId = PushStr8F(s_UIState->UIFrameArena, "A###a%llu", hash);
        float aVal = rgba[3];
        UISlider((char*)aId.str, &rgba[3], 0.0f, 1.0f, 80.0f, 16.0f, 0.05f);
        if(rgba[3] != aVal) changed = true;

        return changed;
    }

    // ============================================
    // Focus Navigation
    // ============================================

    void UISetFocusNext()
    {
        s_UIState->TabPressed = true;
    }

    void UISetFocusPrev()
    {
        s_UIState->ShiftTabPressed = true;
    }

    Vec2 GetStringSizeFont(String8 text, float size, bool mono)
    {
        auto font = mono ? Graphics::Font::GetMonoFont() : Graphics::Font::GetDefaultFont();
        return font->CalculateTextSize(text, size);
    }

    Vec2 GetStringSize(String8 text, float size)
    {
        return GetStringSizeFont(text, size, s_UIState && s_UIState->MonoTextDepth > 0);
    }

    UI_Widget* UIWidgetRecurseDepthFirstPreOrder(UI_Widget* Node)
    {
        UI_Widget* Next = NULL;

        if(Node->first)
        {
            Next = Node->first;
        }
        else
        {
            for(UI_Widget* Parent = Node; Parent != 0; Parent = Parent->parent)
            {
                if(Parent->next)
                {
                    Next = Parent->next;
                    break;
                }
            }
        }

        return Next;
    }

    // SubtreeRoot bounds the walk: when climbing out of a node we stop rather
    // than continuing into the root's own siblings. A walk started mid-tree
    // (the recursive size solve) would otherwise escape and re-walk the rest of
    // the whole tree. Pass null for an unbounded walk.
    UI_Widget* UIWidgetRecurseDepthFirstPostOrder(UI_Widget* Node, UI_Widget* SubtreeRoot)
    {
        if(Node->last != 0)
            return Node->last;

        for(UI_Widget* P = Node; P != 0; P = P->parent)
        {
            if(P == SubtreeRoot)
                break;
            if(P->prev != 0)
                return P->prev;
        }

        return 0;
    }

    void UILayoutSolveStandaloneSizes(UI_Widget* Root, UIAxis Axis)
    {
        const f32 viewportSize = s_UIState->root_parent.size[Axis];
        for(UI_Widget* Widget = Root; Widget; Widget = UIWidgetRecurseDepthFirstPreOrder(Widget))
        {
            UI_Size* Size  = &Widget->semantic_size[Axis];
            float fontSize = Widget->style_vars[StyleVar_FontSize].x;
            switch(Size->kind)
            {
            case SizeKind_Pixels:
            {
                Widget->size[Axis] = Size->value;
            }
            break;

            case SizeKind_TextContent:
            {
                Vec2 padding = Widget->style_vars[StyleVar_Padding].ToVector2();
                Vec2 text_size = GetStringSizeFont(Widget->text, fontSize, Widget->MonoText);
                if(Axis == UIAxis_Y)
                    text_size.y = Maths::Max(text_size.y, GetStringSizeFont(Str8Lit("Ay"), fontSize, Widget->MonoText).y);
                f32 value = Widget->semantic_size[Axis].value;
                // Add padding for both sides (left+right or top+bottom)
                Widget->size[Axis] = (text_size[Axis] + padding[Axis] * 2.0f) * value;
            }
            break;

            case SizeKind_PercentOfViewport:
            {
                Widget->size[Axis] = viewportSize * Size->value;
            }
            break;
            }
        }
    }

    void UILayoutSolveUpwardsSizes(UI_Widget* Root, UIAxis Axis)
    {
        const u32 floatFlag = (Axis == UIAxis_X) ? WidgetFlags_Floating_X : WidgetFlags_Floating_Y;
        const u32 stackFlag = (Axis == UIAxis_X) ? WidgetFlags_StackHorizontally : WidgetFlags_StackVertically;

        for(UI_Widget* Widget = Root; Widget; Widget = UIWidgetRecurseDepthFirstPreOrder(Widget))
        {
            if(!Widget->first)
                continue;

            const f32 ownPad    = Widget->style_vars[StyleVar_Padding].ToVector2()[Axis];
            const f32 content   = Maths::Max(0.0f, Widget->size[Axis] - ownPad * 2.0f);
            const bool stacking = (Widget->flags & stackFlag) != 0;

            f32 used       = 0.0f;
            f32 growWeight = 0.0f;
            i32 flowCount  = 0;
            for(UI_Widget* Child = Widget->first; Child; Child = Child->next)
            {
                UI_Size* Size = &Child->semantic_size[Axis];

                if(Size->kind == SizeKind_PercentOfParent)
                {
                    // Box model: fraction of the parent's CONTENT box.
                    Child->size[Axis] = content * Size->value;
                }

                if(Child->flags & floatFlag)
                    continue;

                if(Size->kind == SizeKind_Grow)
                {
                    growWeight += Maths::Max(Size->value, 0.0f);
                    flowCount++;
                    continue;
                }

                if(stacking)
                    used += Child->size[Axis];
                flowCount++;
            }

            if(growWeight > 0.0f)
            {
                f32 avail;
                if(stacking)
                {
                    const f32 gap  = Widget->style_vars[StyleVar_ItemSpacing].ToVector2()[Axis];
                    const f32 gaps = flowCount > 1 ? gap * (f32)(flowCount - 1) : 0.0f;
                    avail          = Maths::Max(0.0f, content - used - gaps);
                }
                else
                {
                    // Cross axis: grow = fill the content box.
                    avail = content;
                }

                for(UI_Widget* Child = Widget->first; Child; Child = Child->next)
                {
                    if(Child->semantic_size[Axis].kind != SizeKind_Grow)
                        continue;
                    if(Child->flags & floatFlag)
                        continue;
                    const f32 w       = Maths::Max(Child->semantic_size[Axis].value, 0.0f);
                    Child->size[Axis] = stacking ? avail * (growWeight > 0.0f ? w / growWeight : 0.0f) : avail;
                }
            }
        }
    }

    // Bumped once per top-level pass; widgets stamp themselves as they are
    // solved so the recursion below and the outer walk don't redo each other's
    // work. This walk visits a parent BEFORE its children, so a ChildSum parent
    // has to solve its ChildSum/MaxChild children itself - without the stamp,
    // every level re-solved the whole subtree under it and a panel of nested
    // slider rows cost over a second a frame to lay out.
    //
    // The stamp alone is not enough: recursing into an already-solved child
    // still WALKED its whole subtree to find nothing to do, so the cost stayed
    // exponential in tree depth (each level re-walks every level below it).
    // With two deep panels up at once - the trade terminal over a station
    // callout - that reached minutes per frame, i.e. a hang. Both call sites
    // below now check the stamp BEFORE recursing.
    static u64 s_DownwardsSizePass = 0;

    static void UILayoutSolveDownwardsSizesInner(UI_Widget* Root, UIAxis Axis)
    {
        // Cycle guard. A widget linked into its own subtree (the same id pushed
        // as a parent twice in one frame) makes this walk never terminate, and
        // the app hangs with no output at all. Bail out and name the widget
        // instead: a mis-laid-out panel is recoverable, a freeze is not.
        const u64 walkCap = (u64)s_UIState->widgets.length * 8 + 4096;
        u64 walked        = 0;

        for(UI_Widget* Widget = Root; Widget; Widget = UIWidgetRecurseDepthFirstPostOrder(Widget, Root))
        {
            if(++walked > walkCap)
            {
                LWARN("UI layout: widget tree walk exceeded %llu steps at '%.*s' (axis %d) - cycle in the tree, layout abandoned this frame",
                      (unsigned long long)walkCap,
                      (int)Widget->text.size, Widget->text.size ? (const char*)Widget->text.str : "",
                      (int)Axis);
                break;
            }
            UI_Size* Size = &Widget->semantic_size[Axis];

            if(Size->kind == SizeKind_ChildSum || Size->kind == SizeKind_MaxChild)
            {
                if(Widget->DownwardsSizePass[Axis] == s_DownwardsSizePass)
                    continue;
                Widget->DownwardsSizePass[Axis] = s_DownwardsSizePass;
            }

            switch(Size->kind)
            {
            case SizeKind_ChildSum:
            {
                const u32 floatFlag = (Axis == UIAxis_X) ? WidgetFlags_Floating_X : WidgetFlags_Floating_Y;
                f32 Sum = 0;
                i32 childCount = 0;
                for(UI_Widget* Child = Widget->first; Child; Child = Child->next)
                {
                    if(Child->semantic_size[Axis].kind == SizeKind_PercentOfParent || Child->semantic_size[Axis].kind == SizeKind_Grow)
                        continue;

                    if(Child->flags & floatFlag)
                        continue;

                    if((Child->semantic_size[Axis].kind == SizeKind_ChildSum || Child->semantic_size[Axis].kind == SizeKind_MaxChild)
                       && Child->DownwardsSizePass[Axis] != s_DownwardsSizePass)
                    {
                        UILayoutSolveDownwardsSizesInner(Child, Axis);
                    }

                    WidgetFlags flag = (Axis == 0 ? WidgetFlags_StackHorizontally : WidgetFlags_StackVertically);
                    if(Widget->flags & flag)
                    {
                        Sum += Child->size[Axis];
                        childCount++;
                    }
                    else
                    {
                        // Cross axis: max, not sum.
                        Sum = Maths::Max(Sum, Child->size[Axis]);
                    }
                }

                // Add inter-widget gaps
                if(childCount > 1)
                {
                    Vec2 gap = Widget->style_vars[StyleVar_ItemSpacing].ToVector2();
                    Sum += gap[Axis] * (f32)(childCount - 1);
                }

                const f32 ownPad   = Widget->style_vars[StyleVar_Padding].ToVector2()[Axis];
                Widget->size[Axis] = Sum + ownPad * 2.0f;
            }
            break;
            case SizeKind_MaxChild:
            {
                const u32 floatFlag = (Axis == UIAxis_X) ? WidgetFlags_Floating_X : WidgetFlags_Floating_Y;
                f32 Sum = 0;
                for(UI_Widget* Child = Widget->first; Child; Child = Child->next)
                {
                    if(Child->semantic_size[Axis].kind == SizeKind_PercentOfParent || Child->semantic_size[Axis].kind == SizeKind_Grow)
                        continue;

                    // Floating children take no layout space (see ChildSum).
                    if(Child->flags & floatFlag)
                        continue;

                    if((Child->semantic_size[Axis].kind == SizeKind_ChildSum || Child->semantic_size[Axis].kind == SizeKind_MaxChild)
                       && Child->DownwardsSizePass[Axis] != s_DownwardsSizePass)
                    {
                        UILayoutSolveDownwardsSizesInner(Child, Axis);
                    }

                    // Child size = its border box; no child padding (box model).
                    Sum = Maths::Max(Sum, Child->size[Axis]);
                }

                // Own padding reserved around the content (see ChildSum).
                const f32 ownPad   = Widget->style_vars[StyleVar_Padding].ToVector2()[Axis];
                Widget->size[Axis] = Sum + ownPad * 2.0f;
            }
            break;
            }
        }
    }

    void UILayoutSolveDownwardsSizes(UI_Widget* Root, UIAxis Axis)
    {
        ++s_DownwardsSizePass;
        UILayoutSolveDownwardsSizesInner(Root, Axis);
    }

    void UILayoutFinalisePositions(UI_Widget* Root, UIAxis Axis)
    {
        for(UI_Widget* Parent = Root; Parent != 0; Parent = UIWidgetRecurseDepthFirstPreOrder(Parent))
        {
            const f32 parentPad = Parent->style_vars[StyleVar_Padding].ToVector2()[Axis];
            f32 LayoutPosition = parentPad; // flow cursor starts at the content rect
            bool anyPlaced = false;         // gap goes BEFORE each stacked child after the first
            Vec2 itemSpacing = Parent->style_vars[StyleVar_ItemSpacing].ToVector2();
            const bool centreChildrenX = (Parent->flags & WidgetFlags_CentreChildrenX) != 0;
            const bool centreChildrenY = (Parent->flags & WidgetFlags_CentreChildrenY) != 0;
            for(UI_Widget* Child = Parent->first; Child != 0; Child = Child->next)
            {
                const bool wantCentreX = (Child->flags & WidgetFlags_CentreX) || centreChildrenX;
                const bool wantCentreY = (Child->flags & WidgetFlags_CentreY) || centreChildrenY;

                if(Axis == UIAxis_X)
                {
                    bool floating = Child->flags & WidgetFlags_Floating_X;
                    if(Child->flags & WidgetFlags_AlignRight)
                    {
                        Child->relative_position[Axis] = Parent->size[Axis] - parentPad - Child->size[Axis];
                    }
                    else if(!floating)
                    {
                        if(Parent->flags & WidgetFlags_StackHorizontally)
                        {
                            if(anyPlaced)
                                LayoutPosition += itemSpacing.x;
                            // Centring on the STACKING axis is meaningless - the flow
                            // cursor still advances, so an offset just shoves the widget
                            // into its siblings. Only the cross axis centres.
                            f32 xOffset = 0.0f;
                            Child->relative_position[Axis] = LayoutPosition + xOffset;
                            LayoutPosition += Child->size[Axis];
                            anyPlaced = true;
                        }
                        else if(wantCentreX)
                        {
                            Child->relative_position[Axis] = Parent->size[Axis] * 0.5f - Child->size[Axis] * 0.5f;
                        }
                        else
                        {
                            Child->relative_position[Axis] = parentPad;
                        }
                    }
                    else if(wantCentreX)
                    {
                        Child->relative_position[Axis] = Parent->size[Axis] * 0.5f - Child->size[Axis] * 0.5f;
                    }
                }
                if(Axis == UIAxis_Y)
                {
                    bool floating = Child->flags & WidgetFlags_Floating_Y;
                    if(!floating)
                    {
                        if(Parent->flags & WidgetFlags_StackVertically)
                        {
                            if(anyPlaced)
                                LayoutPosition += itemSpacing.y;
                            f32 yOffset = 0.0f; // see the X axis note above
                            Child->relative_position[Axis] = LayoutPosition + yOffset;
                            LayoutPosition += Child->size[Axis];
                            anyPlaced = true;
                        }
                        else if(wantCentreY)
                        {
                            Child->relative_position[Axis] = Parent->size[Axis] * 0.5f - Child->size[Axis] * 0.5f;
                        }
                        else
                        {
                            Child->relative_position[Axis] = parentPad;
                        }
                    }
                    else if(wantCentreY)
                    {
                        Child->relative_position[Axis] = Parent->size[Axis] * 0.5f - Child->size[Axis] * 0.5f;
                    }
                }

                if(Axis == UIAxis_X)
                {
                    f32 off = (Child->flags & WidgetFlags_Floating_X) ? 0.0f : Parent->ChildOffset.x;
                    Child->position.x = Parent->position.x + Child->relative_position[Axis] + off;
                }
                else if(Axis == UIAxis_Y)
                {
                    f32 off = (Child->flags & WidgetFlags_Floating_Y) ? 0.0f : Parent->ChildOffset.y;
                    Child->position.y = Parent->position.y + Child->relative_position[Axis] + off;
                }
            }
        }
    }

    void UIBeginBuild()
    {
        LUMOS_PROFILE_FUNCTION();
        s_HScrollOwnsDrag = false;
        s_ScrollAreaDepth  = 0;
        s_ScrollAreaWidget = nullptr;
        s_ScrollAreaOffset = nullptr;
        UIWidgetsBeginBuild();
    }

    // Root-level widgets flagged TopMost go to the end of the list (in the order they
    // were built), so a popup opened from an early panel still draws over later ones.
    static void UIRaiseTopMost()
    {
        UI_Widget* root     = &s_UIState->root_parent;
        UI_Widget* topFirst = nullptr;
        UI_Widget* topLast  = nullptr;
        u32 guard           = 0;
        for(UI_Widget* w = root->first; w && guard < 100000; guard++)
        {
            UI_Widget* next = w->next;
            if(w->flags & WidgetFlags_TopMost)
            {
                if(w->prev)
                    w->prev->next = w->next;
                else
                    root->first = w->next;
                if(w->next)
                    w->next->prev = w->prev;
                else
                    root->last = w->prev;

                w->prev = topLast;
                w->next = nullptr;
                if(topLast)
                    topLast->next = w;
                else
                    topFirst = w;
                topLast = w;
            }
            w = next;
        }

        if(topFirst)
        {
            if(root->last)
            {
                root->last->next = topFirst;
                topFirst->prev   = root->last;
            }
            else
                root->first = topFirst;
            root->last = topLast;
        }
    }

    void UIEndBuild()
    {
        LUMOS_PROFILE_FUNCTION();

        UIWidgetsEndBuild();
        UIRaiseTopMost();

        // Create deferred tooltip at root level (renders on top of everything)
        if(s_UIState->ShowTooltip && s_UIState->TooltipText.size > 0)
        {
            // Push directly to root
            s_UIState->parents.PushBack(&s_UIState->root_parent);

            const f32 dp = s_UIState->DPIScale;
            // a size below the label it explains, and never the huge one of a game HUD
            const f32 fontSize = Maths::Clamp(s_UIState->TooltipFontSize * 0.92f, 11.0f * dp, 15.0f * dp);
            const f32 maxWidth = 300.0f * dp;
            String8 text       = s_UIState->TooltipText;
            if(GetStringSize(text, fontSize).x > maxWidth)
                text = UIWrapText(text, fontSize, maxWidth);

            // one id: the size from last frame keeps it on screen
            UI_Widget* tooltip = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_DrawText | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                            text,
                                            UIHashCombine(s_UIState->root_parent.hash, 0x544F4F4C544950ull),
                                            { SizeKind_TextContent, 1.0f },
                                            { SizeKind_TextContent, 1.0f });

            // below the pointer, flipped above / pulled left at the screen edges
            const Vec2 screen = s_UIState->root_parent.size;
            const Vec2 size   = tooltip->size.x > 0.0f ? tooltip->size : GetStringSize(text, fontSize) + Vec2(18.0f * dp, 12.0f * dp);
            Vec2 pos          = s_UIState->TooltipPos + Vec2(12.0f * dp, 20.0f * dp);
            if(pos.y + size.y > screen.y - 4.0f * dp)
                pos.y = s_UIState->TooltipPos.y - size.y - 8.0f * dp;
            pos.x = Maths::Clamp(pos.x, 4.0f * dp, Maths::Max(4.0f * dp, screen.x - size.x - 4.0f * dp));
            pos.y = Maths::Max(pos.y, 4.0f * dp);

            tooltip->relative_position                = Vec2(floorf(pos.x), floorf(pos.y));
            tooltip->style_vars[StyleVar_FontSize]     = Vec4(fontSize, 0.0f, 0.0f, 0.0f);
            tooltip->style_vars[StyleVar_Padding]      = Vec4(9.0f * dp, 6.0f * dp, 0.0f, 0.0f);
            tooltip->style_vars[StyleVar_CornerRadius] = Vec4(6.0f * dp, 0.0f, 0.0f, 0.0f);
            tooltip->style_vars[StyleVar_Border]       = Vec4(Maths::Max(1.0f, floorf(dp * 0.5f)), Maths::Max(1.0f, floorf(dp * 0.5f)), 0.0f, 0.0f);

            // dark in both themes; opaque, since the border is a quad behind the fill
            tooltip->style_vars[StyleVar_BackgroundColor] = Vec4(0.085f, 0.085f, 0.10f, 1.0f);
            tooltip->style_vars[StyleVar_BorderColor]     = Vec4(0.26f, 0.26f, 0.30f, 1.0f);
            tooltip->style_vars[StyleVar_TextColor]       = Vec4(0.90f, 0.90f, 0.92f, 1.0f);

            s_UIState->parents.PopBack();
        }
    }

    static void UIShiftSubtreePosition(UI_Widget* Widget, Vec2 delta)
    {
        if(!Widget) return;
        Widget->position = Widget->position + delta;
        for(UI_Widget* Child = Widget->first; Child; Child = Child->next)
            UIShiftSubtreePosition(Child, delta);
    }

    static void UIApplyAnchors(UI_Widget* Root)
    {
        const Vec2 rootSize = Root->size;
        for(UI_Widget* Window = Root->first; Window; Window = Window->next)
        {
            if(Window->Anchor == UIAnchor_None) continue;

            const Vec2 sz     = Window->size;
            const Vec2 margin = Window->AnchorMargin;

            float relX = 0.0f, relY = 0.0f;
            switch(Window->Anchor)
            {
            case UIAnchor_TopLeft:      relX = margin.x;                          relY = margin.y; break;
            case UIAnchor_TopCenter:    relX = (rootSize.x - sz.x) * 0.5f;        relY = margin.y; break;
            case UIAnchor_TopRight:     relX = rootSize.x - sz.x - margin.x;      relY = margin.y; break;
            case UIAnchor_MiddleLeft:   relX = margin.x;                          relY = (rootSize.y - sz.y) * 0.5f + margin.y; break;
            case UIAnchor_MiddleCenter: relX = (rootSize.x - sz.x) * 0.5f + margin.x; relY = (rootSize.y - sz.y) * 0.5f + margin.y; break;
            case UIAnchor_MiddleRight:  relX = rootSize.x - sz.x - margin.x;      relY = (rootSize.y - sz.y) * 0.5f + margin.y; break;
            case UIAnchor_BottomLeft:   relX = margin.x;                          relY = rootSize.y - sz.y - margin.y; break;
            case UIAnchor_BottomCenter: relX = (rootSize.x - sz.x) * 0.5f;        relY = rootSize.y - sz.y - margin.y; break;
            case UIAnchor_BottomRight:  relX = rootSize.x - sz.x - margin.x;      relY = rootSize.y - sz.y - margin.y; break;
            default: continue;
            }

            const Vec2 desiredAbs = Root->position + Vec2(relX, relY);
            const Vec2 delta      = desiredAbs - Window->position;
            Window->relative_position = Window->relative_position + delta;
            UIShiftSubtreePosition(Window, delta);
        }
    }

    static void UIPropagateClipRects(UI_Widget* Root)
    {
        Root->ClipRect = Vec4(-1.0e9f, -1.0e9f, 1.0e9f, 1.0e9f);
        for(UI_Widget* Widget = Root; Widget; Widget = UIWidgetRecurseDepthFirstPreOrder(Widget))
        {
            if(Widget == Root)
                continue;
            UI_Widget* P = Widget->parent;
            if(!P)
            {
                Widget->ClipRect = Vec4(-1.0e9f, -1.0e9f, 1.0e9f, 1.0e9f);
                continue;
            }
            Vec4 clip = P->ClipRect;
            if(P->Clip)
            {
                Vec2 pad = P->style_vars[StyleVar_Padding].ToVector2();
                clip.x = Maths::Max(clip.x, P->position.x + pad.x);
                clip.y = Maths::Max(clip.y, P->position.y + pad.y);
                clip.z = Maths::Min(clip.z, P->position.x + P->size.x - pad.x);
                clip.w = Maths::Min(clip.w, P->position.y + P->size.y - pad.y);
            }
            Widget->ClipRect = clip;
        }
    }

    void UILayoutRoot(UI_Widget* Root)
    {
        for(UIAxis Axis = (UIAxis)0; Axis < UIAxis_Count; Axis = (UIAxis)(Axis + 1))
        {
            UILayoutSolveStandaloneSizes(Root, Axis);
            UILayoutSolveDownwardsSizes(Root, Axis);
            UILayoutSolveUpwardsSizes(Root, Axis);
        }

        for(UIAxis Axis = (UIAxis)0; Axis < UIAxis_Count; Axis = (UIAxis)(Axis + 1))
        {
            UILayoutFinalisePositions(Root, Axis);
        }

        UIApplyAnchors(Root);
        UIPropagateClipRects(Root);
    }

    void UILayout()
    {
        LUMOS_PROFILE_FUNCTION();
        UI_Widget* Root = &s_UIState->root_parent;
        UILayoutRoot(Root);
    }

    void UIAnimate()
    {
        LUMOS_PROFILE_FUNCTION();
        // Animation transitions are handled in UIEndFrame()
        // This function is kept for future animation features or as a hook point
    }

    // ---- Overlay / command-palette building blocks -------------------------

    UI_Interaction UILabelHighlighted(const char* str, const String8& text,
                                      const UIHighlightMask& mask, const Vec4& highlightColour)
    {
        UI_Interaction interaction = UILabel(str, text);
        if(interaction.widget)
        {
            interaction.widget->HighlightBits[0] = mask.bits[0];
            interaction.widget->HighlightBits[1] = mask.bits[1];
            interaction.widget->HighlightColour  = highlightColour;
        }
        return interaction;
    }

    UI_Interaction UIBeginSelectableRow(const char* str, bool selected, float height)
    {
        const float dpi = s_UIState->DPIScale;
        u64 hash;
        HandleUIString(str, &hash);

        UI_Size sizeY = height > 0.0f ? UI_Size{ SizeKind_Pixels, height * dpi }
                                      : UI_Size{ SizeKind_MaxChild, 1.0f };

        UI_Widget* row = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground | WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY,
                                    Str8Lit(""),
                                    hash,
                                    { SizeKind_PercentOfParent, 1.0f },
                                    sizeY);

        row->style_vars[StyleVar_Padding]      = Vec4(8.0f * dpi, 4.0f * dpi, 0.0f, 0.0f);
        row->style_vars[StyleVar_ItemSpacing]  = Vec4(6.0f * dpi, 0.0f, 0.0f, 0.0f);
        row->style_vars[StyleVar_CornerRadius] = Vec4(5.0f * dpi, 0.0f, 0.0f, 0.0f);
        row->style_vars[StyleVar_Border]       = Vec4(0.0f);
        // A clickable is always hot under the cursor - all three border colours
        // have to be cleared or the renderer tints it with the theme blue.
        row->style_vars[StyleVar_BorderColor]       = Vec4(0.0f);
        row->style_vars[StyleVar_HotBorderColor]    = Vec4(0.0f);
        row->style_vars[StyleVar_ActiveBorderColor] = Vec4(0.0f);

        const bool dark = UIThemeIsDark();
        Vec4 idle      = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        Vec4 hot       = dark ? Vec4(1.0f, 1.0f, 1.0f, 0.07f) : Vec4(0.0f, 0.0f, 0.0f, 0.06f);
        Vec4 selectedC = dark ? Vec4(0.24f, 0.44f, 0.85f, 0.75f) : Vec4(0.30f, 0.52f, 0.95f, 0.60f);
        if(s_UIState->AccentColour.w > 0.0f)
            selectedC = Vec4(s_UIState->AccentColour.x, s_UIState->AccentColour.y, s_UIState->AccentColour.z, dark ? 0.35f : 0.25f);

        row->style_vars[StyleVar_BackgroundColor]       = selected ? selectedC : idle;
        row->style_vars[StyleVar_HotBackgroundColor]    = selected ? selectedC : hot;
        row->style_vars[StyleVar_ActiveBackgroundColor] = selectedC;

        UI_Interaction interaction = HandleWidgetInteraction(row);
        PushParent(row);
        return interaction;
    }

    void UIEndSelectableRow()
    {
        PopParent(GetCurrentParent());
    }

    UI_Interaction UIBadge(const char* str, const String8& text, const Vec4& bg, const Vec4& fg)
    {
        const float dpi = s_UIState->DPIScale;
        u64 hash;
        HandleUIString(str, &hash);

        UI_Widget* badge = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawText | WidgetFlags_CentreY,
                                      text,
                                      hash,
                                      { SizeKind_TextContent, 1.0f },
                                      { SizeKind_TextContent, 1.0f });

        badge->style_vars[StyleVar_Padding]         = Vec4(6.0f * dpi, 2.0f * dpi, 0.0f, 0.0f);
        badge->style_vars[StyleVar_Border]          = Vec4(0.0f);
        badge->style_vars[StyleVar_BorderColor]     = Vec4(0.0f);
        badge->style_vars[StyleVar_BackgroundColor] = bg;
        badge->style_vars[StyleVar_TextColor]       = fg;
        badge->style_vars[StyleVar_CornerRadius]    = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);
        badge->style_vars[StyleVar_ShadowColor]     = Vec4(0.0f);

        return HandleWidgetInteraction(badge);
    }

    void UIFlexSpacer(float weight)
    {
        String8 id = PushStr8F(s_UIState->UIFrameArena, "flex###flex%llu", s_UIState->WidgetIdCounter++);
        u64 hash;
        String8 text = HandleUIString((char*)id.str, &hash);

        UI_Widget* parent = GetCurrentParent();
        const bool horizontal = parent && (parent->flags & WidgetFlags_StackHorizontally);

        if(horizontal)
            PushWidget(0, text, hash, { SizeKind_Grow, weight }, { SizeKind_Pixels, 1.0f });
        else
            PushWidget(0, text, hash, { SizeKind_Pixels, 1.0f }, { SizeKind_Grow, weight });
    }

    UI_Interaction UISearchField(const char* str, char* buffer, u32 buffer_size,
                                 const char* placeholder, float width)
    {
        const float dpi = s_UIState->DPIScale;
        u64 hash;
        HandleUIString(str, &hash);

        const u64 fieldHash = UIHashCombine(GetCurrentParent()->hash, hash);
        const bool isFocused = (s_UIState->FocusedTextInput == fieldHash);
        const u32 len        = (u32)strlen(buffer);

#ifdef LUMOS_PLATFORM_IOS
        // only when the keyboard changed it: the field can also be edited from here (or cleared by its owner)
        if(isFocused && s_NativeKeyboardHash == fieldHash && s_NativeKeyboardText != s_NativeKeyboardSeen)
        {
            s_NativeKeyboardSeen = s_NativeKeyboardText;
            strncpy(buffer, s_NativeKeyboardText.c_str(), buffer_size - 1);
            buffer[buffer_size - 1]    = 0;
            s_UIState->TextInputCursor = (u32)strlen(buffer);
        }
#endif

        String8 displayStr;
        const bool showPlaceholder = (len == 0) && placeholder && placeholder[0];
        if(showPlaceholder)
            displayStr = Str8C((char*)placeholder);
        else
            displayStr = Str8C(buffer);

        UI_Size sizeX = width > 0.0f ? UI_Size{ SizeKind_Pixels, width * dpi }
                                     : UI_Size{ SizeKind_Grow, 1.0f };

        // callers restyle the field after this returns: caret, selection and focus line go by what it ended up with last frame
        Vec2 pad    = Vec2(10.0f * dpi, 8.0f * dpi);
        f32 radius  = 6.0f * dpi;
        bool filled = true;
        {
            UI_Widget* last = nullptr;
            HashMapFind(&s_UIState->widgets, fieldHash, &last);
            if(last && last->LastFrameIndexActive + 1 == s_UIState->FrameIndex)
            {
                pad    = last->style_vars[StyleVar_Padding].ToVector2();
                radius = last->style_vars[StyleVar_CornerRadius].x;
                filled = last->style_vars[StyleVar_BackgroundColor].w > 0.0f;
            }
        }

        UI_Widget* field = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground | WidgetFlags_DrawText | WidgetFlags_CentreY,
                                      displayStr,
                                      hash,
                                      sizeX,
                                      { SizeKind_TextContent, 1.0f });

        const bool dark = UIThemeIsDark();
        field->style_vars[StyleVar_Padding]           = Vec4(10.0f * dpi, 8.0f * dpi, 0.0f, 0.0f);
        field->style_vars[StyleVar_Border]            = Vec4(0.0f);
        field->style_vars[StyleVar_BorderColor]       = Vec4(0.0f);
        field->style_vars[StyleVar_HotBorderColor]    = Vec4(0.0f);
        field->style_vars[StyleVar_ActiveBorderColor] = Vec4(0.0f);
        field->style_vars[StyleVar_CornerRadius]      = Vec4(6.0f * dpi, 0.0f, 0.0f, 0.0f);

        Vec4 bg    = dark ? Vec4(1.0f, 1.0f, 1.0f, 0.06f) : Vec4(0.0f, 0.0f, 0.0f, 0.05f);
        Vec4 bgHot = dark ? Vec4(1.0f, 1.0f, 1.0f, 0.10f) : Vec4(0.0f, 0.0f, 0.0f, 0.08f);
        Vec4 text  = dark ? Vec4(0.95f, 0.95f, 0.97f, 1.0f) : Vec4(0.08f, 0.08f, 0.10f, 1.0f);
        // dimmed in rgb: text alpha is not a reliable fade
        if(showPlaceholder)
            text = dark ? Vec4(0.52f, 0.52f, 0.55f, 1.0f) : Vec4(0.55f, 0.55f, 0.58f, 1.0f);

        field->style_vars[StyleVar_BackgroundColor]       = bg;
        field->style_vars[StyleVar_HotBackgroundColor]    = isFocused ? bg : bgHot;
        field->style_vars[StyleVar_ActiveBackgroundColor] = isFocused ? bg : bgHot;
        field->style_vars[StyleVar_TextColor]             = text;
        field->style_vars[StyleVar_HotTextColor]          = text;
        field->style_vars[StyleVar_ActiveTextColor]       = text;

        s_UIState->FocusableWidgets.PushBack(fieldHash);
        UIScribbleRegister(field, fieldHash, false);

        UI_Interaction interaction = HandleWidgetInteraction(field);
        if(interaction.hovering)
            UISetCursor(UICursor_Text);

        auto takeFocus = [&](bool selectAll)
        {
            s_UIState->FocusedTextInput    = fieldHash;
            s_UIState->TextInputBuffer     = buffer;
            s_UIState->TextInputBufferSize = buffer_size;
            s_UIState->TextInputCursor     = (u32)strlen(buffer);
            s_UIState->TextInputSelStart   = 0;
            s_UIState->TextInputSelEnd     = selectAll ? (u32)strlen(buffer) : 0;
#ifdef LUMOS_PLATFORM_IOS
            s_NativeKeyboardText = buffer;
            s_NativeKeyboardSeen = buffer;
            s_NativeKeyboardHash = fieldHash;
            OpeniOSKeyboard(&s_NativeKeyboardText);
#endif
        };

        if(interaction.clicked)
            takeFocus(false);

        if(s_ClaimNextTextInputFocus)
        {
            s_ClaimNextTextInputFocus     = false;
            s_UIState->PendingFocusWidget = fieldHash;
        }

        if(s_UIState->PendingFocusWidget == fieldHash)
        {
            s_UIState->PendingFocusWidget = 0;
            takeFocus(true);
        }

        // focus mark: an accent line along the bottom (a border would show through a see-through fill)
        // not on a field with no box of its own
        if(isFocused && filled && field->size.x > 0.0f)
        {
            const f32 inset = Maths::Min(radius, field->size.y * 0.5f);
            const f32 lineH = Maths::Max(1.0f, floorf(1.5f * dpi));
            Vec4 accent     = s_UIState->AccentColour.w > 0.0f ? s_UIState->AccentColour : Vec4(0.35f, 0.58f, 1.0f, 1.0f);
            accent.w        = 1.0f;
            PushParent(field);
            UI_Widget* mark = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y, Str8Lit(""),
                                         UIHashCombine(fieldHash, 0x464F43ull), { SizeKind_Pixels, Maths::Max(0.0f, field->size.x - inset * 2.0f) }, { SizeKind_Pixels, lineH });
            mark->relative_position                    = Vec2(inset, field->size.y - lineH);
            mark->style_vars[StyleVar_Padding]         = Vec4(0.0f);
            mark->style_vars[StyleVar_Border]          = Vec4(0.0f);
            mark->style_vars[StyleVar_CornerRadius]    = Vec4(lineH * 0.5f, 0.0f, 0.0f, 0.0f);
            mark->style_vars[StyleVar_BackgroundColor] = accent;
            PopParent(field);
        }

        // text wider than the field: show the part round the cursor
        u32 viewStart = 0, viewEnd = len;
        if(!showPlaceholder)
        {
            TextFieldView(field, buffer, fieldHash, &viewStart, &viewEnd, &pad);
            if(viewStart != 0 || viewEnd != len)
                field->text = PushStr8Copy(s_UIState->UIFrameArena, Str8((u8*)buffer + viewStart, viewEnd - viewStart));
        }

        UITextFieldExtras(field, buffer, fieldHash, showPlaceholder, interaction,
                          CaretColour(dark ? Vec4(0.95f, 0.95f, 0.97f, 1.0f) : Vec4(0.08f, 0.08f, 0.10f, 1.0f)), viewStart, viewEnd, &pad);

        return interaction;
    }

    void UIPlotHistogram(const char* str, const float* values, u32 count,
                         float minValue, float maxValue, float width, float height,
                         const Vec4& colour, const Vec4& bgColour, float markerValue)
    {
        const float dpi = s_UIState->DPIScale;
        width  *= dpi;
        height *= dpi;

        u64 hash;
        String8 text = HandleUIString(str, &hash);

        UI_Widget* plot = PushWidget(WidgetFlags_DrawBackground,
                                     Str8Lit(""),
                                     hash,
                                     { SizeKind_Pixels, width },
                                     { SizeKind_Pixels, height });

        plot->style_vars[StyleVar_Padding]         = Vec4(0.0f);
        plot->style_vars[StyleVar_Border]          = Vec4(0.0f);
        plot->style_vars[StyleVar_BorderColor]     = Vec4(0.0f);
        plot->style_vars[StyleVar_BackgroundColor] = bgColour;
        plot->style_vars[StyleVar_CornerRadius]    = Vec4(3.0f * dpi, 0.0f, 0.0f, 0.0f);
        plot->style_vars[StyleVar_ShadowColor]     = Vec4(0.0f);

        if(!values || count == 0)
            return;

        PushParent(plot);

        const float range    = Maths::Max(0.0001f, maxValue - minValue);
        const float barWidth = Maths::Max(1.0f, width / (float)count);

        for(u32 i = 0; i < count; i++)
        {
            float t = Maths::Clamp((values[i] - minValue) / range, 0.0f, 1.0f);
            float h = Maths::Max(1.0f, t * height);

            String8 barId = PushStr8F(s_UIState->UIFrameArena, "bar###%s_bar%u", (char*)text.str, i);
            u64 barHash;
            HandleUIString((char*)barId.str, &barHash);

            UI_Widget* bar = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                        Str8Lit(""),
                                        barHash,
                                        { SizeKind_Pixels, Maths::Max(1.0f, barWidth - 1.0f * dpi) },
                                        { SizeKind_Pixels, h });

            bar->relative_position                 = Vec2((float)i * barWidth, height - h);
            bar->style_vars[StyleVar_Padding]      = Vec4(0.0f);
            bar->style_vars[StyleVar_Border]       = Vec4(0.0f);
            bar->style_vars[StyleVar_BorderColor]  = Vec4(0.0f);
            bar->style_vars[StyleVar_ShadowColor]  = Vec4(0.0f);
            // Bars near the top of the range go warm - spikes read at a glance.
            Vec4 c = colour;
            if(t > 0.75f)
                c = c.Lerp(Vec4(0.95f, 0.35f, 0.25f, colour.w), (t - 0.75f) * 4.0f);
            bar->style_vars[StyleVar_BackgroundColor] = c;
            bar->style_vars[StyleVar_CornerRadius]    = Vec4(0.0f);
        }

        if(markerValue > minValue && markerValue < maxValue)
        {
            float t = (markerValue - minValue) / range;
            String8 markId = PushStr8F(s_UIState->UIFrameArena, "mark###%s_mark", (char*)text.str);
            u64 markHash;
            HandleUIString((char*)markId.str, &markHash);

            UI_Widget* mark = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                         Str8Lit(""),
                                         markHash,
                                         { SizeKind_Pixels, width },
                                         { SizeKind_Pixels, Maths::Max(1.0f, dpi) });

            mark->relative_position                 = Vec2(0.0f, height - t * height);
            mark->style_vars[StyleVar_Padding]      = Vec4(0.0f);
            mark->style_vars[StyleVar_Border]       = Vec4(0.0f);
            mark->style_vars[StyleVar_BorderColor]  = Vec4(0.0f);
            mark->style_vars[StyleVar_ShadowColor]  = Vec4(0.0f);
            mark->style_vars[StyleVar_BackgroundColor] = Vec4(1.0f, 1.0f, 1.0f, 0.25f);
            mark->style_vars[StyleVar_CornerRadius]    = Vec4(0.0f);
        }

        PopParent(plot);
    }

    UI_Interaction UIDimBackdrop(const char* str, float alpha)
    {
        u64 hash;
        HandleUIString(str, &hash);

        UI_Widget* dim = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Clickable | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                    Str8Lit(""),
                                    hash,
                                    { SizeKind_PercentOfViewport, 1.0f },
                                    { SizeKind_PercentOfViewport, 1.0f });

        dim->relative_position                    = Vec2(0.0f, 0.0f);
        dim->style_vars[StyleVar_Padding]         = Vec4(0.0f);
        dim->style_vars[StyleVar_Border]          = Vec4(0.0f);
        dim->style_vars[StyleVar_BorderColor]     = Vec4(0.0f);
        dim->style_vars[StyleVar_HotBorderColor]  = Vec4(0.0f);
        dim->style_vars[StyleVar_ActiveBorderColor] = Vec4(0.0f);
        dim->style_vars[StyleVar_CornerRadius]    = Vec4(0.0f);
        dim->style_vars[StyleVar_ShadowColor]     = Vec4(0.0f);

        Vec4 c = Vec4(0.0f, 0.0f, 0.0f, alpha);
        dim->style_vars[StyleVar_BackgroundColor]       = c;
        dim->style_vars[StyleVar_HotBackgroundColor]    = c;
        dim->style_vars[StyleVar_ActiveBackgroundColor] = c;

        return HandleWidgetInteraction(dim);
    }

    // Draggable number field. Horizontal drag scrubs the value; minValue >= maxValue
    // means unbounded. StateValue holds the value at press so the drag is absolute
    // rather than accumulating rounding error frame to frame.
    UI_Interaction UIDragFloat(const char* str, float* value, float speed,
                               float minValue, float maxValue, const char* fmt, float width)
    {
        const float dpi = s_UIState->DPIScale;
        u64 hash;
        String8 text = HandleUIString(str, &hash);
        (void)text;

        char buffer[48];
        snprintf(buffer, sizeof(buffer), fmt ? fmt : "%.3f", *value);

        UI_Widget* field = PushWidget(WidgetFlags_Clickable | WidgetFlags_Draggable | WidgetFlags_DrawBackground | WidgetFlags_DrawText | WidgetFlags_CentreY,
                                      PushStr8Copy(s_UIState->UIFrameArena, buffer),
                                      hash,
                                      { SizeKind_Pixels, width * dpi },
                                      { SizeKind_TextContent, 1.0f });

        field->TextAlignment                   = UI_Text_Alignment_Center_X;
        field->drag_constraint_y               = true;
        field->style_vars[StyleVar_Padding]    = Vec4(4.0f * dpi, 3.0f * dpi, 0.0f, 0.0f);
        field->style_vars[StyleVar_Border]     = Vec4(0.0f);
        field->style_vars[StyleVar_BorderColor]       = Vec4(0.0f);
        field->style_vars[StyleVar_HotBorderColor]    = Vec4(0.0f);
        field->style_vars[StyleVar_ActiveBorderColor] = Vec4(0.0f);
        field->style_vars[StyleVar_CornerRadius] = Vec4(3.0f * dpi, 0.0f, 0.0f, 0.0f);
        field->style_vars[StyleVar_ShadowColor]  = Vec4(0.0f);

        const bool dark = UIThemeIsDark();
        Vec4 bg  = dark ? Vec4(1.0f, 1.0f, 1.0f, 0.08f) : Vec4(0.0f, 0.0f, 0.0f, 0.06f);
        Vec4 hot = dark ? Vec4(1.0f, 1.0f, 1.0f, 0.16f) : Vec4(0.0f, 0.0f, 0.0f, 0.12f);
        field->style_vars[StyleVar_BackgroundColor]       = bg;
        field->style_vars[StyleVar_HotBackgroundColor]    = hot;
        field->style_vars[StyleVar_ActiveBackgroundColor] = hot;

        UI_Interaction interaction = HandleWidgetInteraction(field);

        if(interaction.pressed)
            memcpy(&field->StateValue, value, sizeof(float));

        if(field->dragging)
        {
            float start = 0.0f;
            memcpy(&start, &field->StateValue, sizeof(float));
            float v = start + (interaction.drag_delta.x / dpi) * speed;
            if(maxValue > minValue)
                v = Maths::Clamp(v, minValue, maxValue);
            *value = v;
        }

        return interaction;
    }

    static bool UIDragFieldTinted(const char* id, float* value, float speed,
                                  float minValue, float maxValue, const char* fmt,
                                  float width, const Vec4& tint)
    {
        UIPushStyle(StyleVar_TextColor, tint);
        UIPushStyle(StyleVar_HotTextColor, tint);
        UIPushStyle(StyleVar_ActiveTextColor, tint);
        const float before         = *value;
        UI_Interaction interaction = UIDragFloat(id, value, speed, minValue, maxValue, fmt, width);
        UIPopStyle(StyleVar_ActiveTextColor);
        UIPopStyle(StyleVar_HotTextColor);
        UIPopStyle(StyleVar_TextColor);
        return interaction.dragging && *value != before;
    }

    bool UIDragFloatRow(const char* str, const String8& label, float* value, float speed,
                        float minValue, float maxValue, const char* fmt)
    {
        const float dpi = s_UIState->DPIScale;
        UIBeginRowFullWidth();
        GetCurrentParent()->style_vars[StyleVar_ItemSpacing] = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);

        String8 labelId = PushStr8F(s_UIState->UIFrameArena, "l###%slabel", str);
        UILabel((const char*)labelId.str, label);
        UIFlexSpacer();

        String8 fieldId = PushStr8F(s_UIState->UIFrameArena, "f###%sfield", str);
        const bool changed = UIDragFieldTinted((const char*)fieldId.str, value, speed, minValue, maxValue,
                                               fmt ? fmt : "%.3f", 84.0f,
                                               UIThemeIsDark() ? Vec4(0.92f, 0.93f, 0.96f, 1.0f) : Vec4(0.1f, 0.1f, 0.12f, 1.0f));
        UIEndRow();
        return changed;
    }

    bool UIDragVec3Row(const char* str, const String8& label, float* xyz, float speed)
    {
        const float dpi = s_UIState->DPIScale;
        UIBeginRowFullWidth();
        GetCurrentParent()->style_vars[StyleVar_ItemSpacing] = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);

        String8 labelId = PushStr8F(s_UIState->UIFrameArena, "l###%slabel", str);
        UILabel((const char*)labelId.str, label);
        UIFlexSpacer();

        static const Vec4 kAxisTint[3] = {
            Vec4(1.00f, 0.45f, 0.42f, 1.0f),
            Vec4(0.55f, 0.90f, 0.50f, 1.0f),
            Vec4(0.45f, 0.70f, 1.00f, 1.0f),
        };
        static const char* kAxisName[3] = { "x", "y", "z" };

        bool changed = false;
        for(int i = 0; i < 3; i++)
        {
            String8 fieldId = PushStr8F(s_UIState->UIFrameArena, "f###%s%s", str, kAxisName[i]);
            changed |= UIDragFieldTinted((const char*)fieldId.str, &xyz[i], speed, 0.0f, 0.0f, "%.2f", 60.0f, kAxisTint[i]);
        }

        UIEndRow();
        return changed;
    }

    void RefreshUI()
    {
        ForHashMapEach(u64, UI_Widget*, &s_UIState->widgets, it)
        {
            u64 key          = *it.key;
            UI_Widget* value = *it.value;

            s_UIState->WidgetAllocator->Deallocate(value);
        }

        HashMapClear(&s_UIState->widgets);

        // Everything above was freed — drop every cached pointer/hash into it.
        s_UIState->active_widget       = 0;
        s_UIState->active_widget_state = nullptr;
        s_UIState->hot_widget          = 0;
        s_UIState->next_hot_widget     = 0;
        s_UIState->FocusedTextInput    = 0;
        s_UIState->TextInputBuffer     = nullptr;
        s_UIState->TextInputBufferSize = 0;

        s_UIState->root_parent.first = NULL;
        s_UIState->root_parent.last  = NULL;
        s_UIState->root_parent.next  = NULL;
        s_UIState->root_parent.prev  = NULL;
    }

    void drawHierarchy(UI_Widget* widget)
    {
        UI_Widget* Next = NULL;
        ImGui::PushID((int)(intptr_t)widget);
        if(ImGui::TreeNode(widget->text.size ? (char*)widget->text.str : "Widget"))
        {
            ImGui::Separator();

            if(widget->clicked)
                ImGui::TextUnformatted("clicked");

            if(widget->dragging)
                ImGui::TextUnformatted("Dragging");

            ImGui::Text("Size : %.2f , %.2f", widget->size.x, widget->size.y);
            ImGui::Text("Position : %.2f , %.2f", widget->position.x, widget->position.y);

            if(widget->parent)
                ImGui::Text("Parent : %s", (char*)widget->parent->text.str);

            if(widget->next)
                ImGui::Text("Next : %s", (char*)widget->next->text.str);

            if(widget->prev)
                ImGui::Text("Prev : %s", (char*)widget->prev->text.str);

            ImGui::Separator();
            if(widget->first)
            {
                Next = widget->first;
                drawHierarchy(Next);
            }

            ImGui::TreePop();
        }

        if(widget->next)
        {
            Next = widget->next;
            drawHierarchy(Next);
        }
        ImGui::PopID();
    };

    void DearIMGUIDebugPanel()
    {

        if(ImGui::Begin("UI Debug"))
        {
            drawHierarchy(&GetUIState()->root_parent);

            if(ImGui::TreeNode("Default Style"))
            {
                ImGui::DragFloat("Padding", &s_UIState->style_variable_lists[StyleVar_Padding].first->value.x);
                ImGui::DragFloat("BorderX", &s_UIState->style_variable_lists[StyleVar_Border].first->value.x);
                ImGui::DragFloat("BorderY", &s_UIState->style_variable_lists[StyleVar_Border].first->value.y);
                ImGui::DragFloat("Font Size", &s_UIState->style_variable_lists[StyleVar_FontSize].first->value.y);

                ImGui::ColorEdit4("Border Colour", &s_UIState->style_variable_lists[StyleVar_BorderColor].first->value.x);
                ImGui::ColorEdit4("Hot Border Colour", &s_UIState->style_variable_lists[StyleVar_HotBorderColor].first->value.x);
                ImGui::ColorEdit4("Active Border Colour", &s_UIState->style_variable_lists[StyleVar_ActiveBorderColor].first->value.x);

                ImGui::ColorEdit4("Background Colour", &s_UIState->style_variable_lists[StyleVar_BackgroundColor].first->value.x);
                ImGui::ColorEdit4("Hot Background Colour", &s_UIState->style_variable_lists[StyleVar_HotBackgroundColor].first->value.x);
                ImGui::ColorEdit4("Active Background Colour", &s_UIState->style_variable_lists[StyleVar_ActiveBackgroundColor].first->value.x);

                ImGui::ColorEdit4("Text Colour", &s_UIState->style_variable_lists[StyleVar_TextColor].first->value.x);
                ImGui::ColorEdit4("Hot Text Colour", &s_UIState->style_variable_lists[StyleVar_HotTextColor].first->value.x);
                ImGui::ColorEdit4("Active Text Colour", &s_UIState->style_variable_lists[StyleVar_ActiveTextColor].first->value.x);

                // style_variable_lists[StyleVar_Border].first->value                = { 1.0f, 1.0f, 0.0f, 0.0f };
                // style_variable_lists[StyleVar_BorderColor].first->value           = { 1.0f, 0.0f, 0.0f, 1.0f };
                // style_variable_lists[StyleVar_BackgroundColor].first->value       = { 1.0f, 1.0f, 1.0f, 1.0f };
                // style_variable_lists[StyleVar_TextColor].first->value             = { 0.0f, 0.0f, 0.0f, 1.0f };
                // style_variable_lists[StyleVar_HotBorderColor].first->value        = { 0.9f, 0.0f, 0.0f, 0.8f };
                // style_variable_lists[StyleVar_HotBackgroundColor].first->value    = { 0.9f, 0.9f, 0.9f, 1.0f };
                // style_variable_lists[StyleVar_HotTextColor].first->value          = { 0.1f, 0.1f, 0.1f, 1.0f };
                // style_variable_lists[StyleVar_ActiveBorderColor].first->value     = { 0.5f, 0.0f, 0.0f, 0.8f };
                // style_variable_lists[StyleVar_ActiveBackgroundColor].first->value = { 0.7f, 0.7f, 0.7f, 1.0f };
                // style_variable_lists[StyleVar_ActiveTextColor].first->value       = { 0.5f, 0.0f, 0.0f, 1.0f };
                // style_variable_lists[StyleVar_FontSize].first->value              = { 28.0f, 0.0f, 0.0f, 1.0f };

                ImGui::TreePop();
            }

            if(ImGui::Button("Refresh"))
                RefreshUI();
        }

        ImGui::End();
    }

    float UIGetTextHeight(Graphics::Font* font, UI_Widget* widget)
    {
        float fontSize      = widget->style_vars[StyleVar_FontSize].x;
        float paddingHeight = widget->style_vars[StyleVar_Padding].y;

        return font->CalculateTextSize(Str8Lit("A"), fontSize).y + paddingHeight;
    }
}

Lumos::UI_State* Lumos::GetUIState()
{
    return Lumos::s_UIState;
}

namespace Lumos
{
    void UIApplyLightTheme()
    {
        Style_Variable_List* style_variable_lists = s_UIState->style_variable_lists;

        style_variable_lists[StyleVar_Padding].first->value               = { 6.0f, 3.0f, 0.0f, 0.0f };
        style_variable_lists[StyleVar_Border].first->value                = { 1.0f, 1.0f, 0.0f, 0.0f };
        style_variable_lists[StyleVar_BorderColor].first->value           = { 0.7f, 0.7f, 0.7f, 1.0f };
        style_variable_lists[StyleVar_BackgroundColor].first->value       = { 0.95f, 0.95f, 0.95f, 1.0f };
        style_variable_lists[StyleVar_TextColor].first->value             = { 0.1f, 0.1f, 0.1f, 1.0f };
        style_variable_lists[StyleVar_HotBorderColor].first->value        = { 0.4f, 0.6f, 1.0f, 1.0f };
        style_variable_lists[StyleVar_HotBackgroundColor].first->value    = { 0.92f, 0.94f, 0.98f, 1.0f };
        style_variable_lists[StyleVar_HotTextColor].first->value          = { 0.05f, 0.05f, 0.05f, 1.0f };
        style_variable_lists[StyleVar_ActiveBorderColor].first->value     = { 0.3f, 0.5f, 0.95f, 1.0f };
        style_variable_lists[StyleVar_ActiveBackgroundColor].first->value = { 0.85f, 0.9f, 0.98f, 1.0f };
        style_variable_lists[StyleVar_ActiveTextColor].first->value       = { 0.0f, 0.0f, 0.0f, 1.0f };

        style_variable_lists[StyleVar_ItemSpacing].first->value          = { 2.0f, 2.0f, 0.0f, 0.0f };

        s_UIState->CurrentTheme = UITheme_Light;
    }

    void UIApplyDarkTheme()
    {
        Style_Variable_List* style_variable_lists = s_UIState->style_variable_lists;

        style_variable_lists[StyleVar_Padding].first->value               = { 6.0f, 3.0f, 0.0f, 0.0f };
        style_variable_lists[StyleVar_Border].first->value                = { 1.0f, 1.0f, 0.0f, 0.0f };
        style_variable_lists[StyleVar_BorderColor].first->value           = { 0.35f, 0.35f, 0.35f, 1.0f };
        style_variable_lists[StyleVar_BackgroundColor].first->value       = { 0.15f, 0.15f, 0.15f, 1.0f };
        style_variable_lists[StyleVar_TextColor].first->value             = { 0.95f, 0.95f, 0.95f, 1.0f };
        style_variable_lists[StyleVar_HotBorderColor].first->value        = { 0.45f, 0.65f, 1.0f, 1.0f };
        style_variable_lists[StyleVar_HotBackgroundColor].first->value    = { 0.22f, 0.22f, 0.22f, 1.0f };
        style_variable_lists[StyleVar_HotTextColor].first->value          = { 1.0f, 1.0f, 1.0f, 1.0f };
        style_variable_lists[StyleVar_ActiveBorderColor].first->value     = { 0.35f, 0.55f, 0.95f, 1.0f };
        style_variable_lists[StyleVar_ActiveBackgroundColor].first->value = { 0.25f, 0.25f, 0.25f, 1.0f };
        style_variable_lists[StyleVar_ActiveTextColor].first->value       = { 1.0f, 1.0f, 1.0f, 1.0f };

        style_variable_lists[StyleVar_ItemSpacing].first->value          = { 2.0f, 2.0f, 0.0f, 0.0f };

        s_UIState->CurrentTheme = UITheme_Dark;
    }

    void UIApplyBlueTheme()
    {
        Style_Variable_List* style_variable_lists = s_UIState->style_variable_lists;

        style_variable_lists[StyleVar_Padding].first->value               = { 6.0f, 3.0f, 0.0f, 0.0f };
        style_variable_lists[StyleVar_Border].first->value                = { 1.0f, 1.0f, 0.0f, 0.0f };
        style_variable_lists[StyleVar_BorderColor].first->value           = { 0.3f, 0.4f, 0.6f, 1.0f };
        style_variable_lists[StyleVar_BackgroundColor].first->value       = { 0.15f, 0.2f, 0.3f, 1.0f };
        style_variable_lists[StyleVar_TextColor].first->value             = { 0.9f, 0.95f, 1.0f, 1.0f };
        style_variable_lists[StyleVar_HotBorderColor].first->value        = { 0.4f, 0.6f, 0.9f, 1.0f };
        style_variable_lists[StyleVar_HotBackgroundColor].first->value    = { 0.2f, 0.28f, 0.4f, 1.0f };
        style_variable_lists[StyleVar_HotTextColor].first->value          = { 1.0f, 1.0f, 1.0f, 1.0f };
        style_variable_lists[StyleVar_ActiveBorderColor].first->value     = { 0.5f, 0.7f, 1.0f, 1.0f };
        style_variable_lists[StyleVar_ActiveBackgroundColor].first->value = { 0.25f, 0.35f, 0.5f, 1.0f };
        style_variable_lists[StyleVar_ActiveTextColor].first->value       = { 1.0f, 1.0f, 1.0f, 1.0f };

        style_variable_lists[StyleVar_ItemSpacing].first->value          = { 2.0f, 2.0f, 0.0f, 0.0f };

        s_UIState->CurrentTheme = UITheme_Blue;
    }

    void UIApplyGreenTheme()
    {
        Style_Variable_List* style_variable_lists = s_UIState->style_variable_lists;

        style_variable_lists[StyleVar_Padding].first->value               = { 6.0f, 3.0f, 0.0f, 0.0f };
        style_variable_lists[StyleVar_Border].first->value                = { 1.0f, 1.0f, 0.0f, 0.0f };
        style_variable_lists[StyleVar_BorderColor].first->value           = { 0.3f, 0.5f, 0.35f, 1.0f };
        style_variable_lists[StyleVar_BackgroundColor].first->value       = { 0.12f, 0.22f, 0.15f, 1.0f };
        style_variable_lists[StyleVar_TextColor].first->value             = { 0.9f, 1.0f, 0.92f, 1.0f };
        style_variable_lists[StyleVar_HotBorderColor].first->value        = { 0.4f, 0.75f, 0.5f, 1.0f };
        style_variable_lists[StyleVar_HotBackgroundColor].first->value    = { 0.18f, 0.32f, 0.22f, 1.0f };
        style_variable_lists[StyleVar_HotTextColor].first->value          = { 1.0f, 1.0f, 1.0f, 1.0f };
        style_variable_lists[StyleVar_ActiveBorderColor].first->value     = { 0.5f, 0.9f, 0.6f, 1.0f };
        style_variable_lists[StyleVar_ActiveBackgroundColor].first->value = { 0.22f, 0.4f, 0.28f, 1.0f };
        style_variable_lists[StyleVar_ActiveTextColor].first->value       = { 1.0f, 1.0f, 1.0f, 1.0f };

        style_variable_lists[StyleVar_ItemSpacing].first->value          = { 2.0f, 2.0f, 0.0f, 0.0f };

        s_UIState->CurrentTheme = UITheme_Green;
    }

    void UIApplyPurpleTheme()
    {
        Style_Variable_List* style_variable_lists = s_UIState->style_variable_lists;

        style_variable_lists[StyleVar_Padding].first->value               = { 6.0f, 3.0f, 0.0f, 0.0f };
        style_variable_lists[StyleVar_Border].first->value                = { 1.0f, 1.0f, 0.0f, 0.0f };
        style_variable_lists[StyleVar_BorderColor].first->value           = { 0.45f, 0.35f, 0.55f, 1.0f };
        style_variable_lists[StyleVar_BackgroundColor].first->value       = { 0.18f, 0.14f, 0.22f, 1.0f };
        style_variable_lists[StyleVar_TextColor].first->value             = { 0.95f, 0.9f, 1.0f, 1.0f };
        style_variable_lists[StyleVar_HotBorderColor].first->value        = { 0.65f, 0.5f, 0.85f, 1.0f };
        style_variable_lists[StyleVar_HotBackgroundColor].first->value    = { 0.25f, 0.2f, 0.32f, 1.0f };
        style_variable_lists[StyleVar_HotTextColor].first->value          = { 1.0f, 1.0f, 1.0f, 1.0f };
        style_variable_lists[StyleVar_ActiveBorderColor].first->value     = { 0.75f, 0.55f, 0.95f, 1.0f };
        style_variable_lists[StyleVar_ActiveBackgroundColor].first->value = { 0.32f, 0.25f, 0.42f, 1.0f };
        style_variable_lists[StyleVar_ActiveTextColor].first->value       = { 1.0f, 1.0f, 1.0f, 1.0f };

        style_variable_lists[StyleVar_ItemSpacing].first->value          = { 2.0f, 2.0f, 0.0f, 0.0f };

        s_UIState->CurrentTheme = UITheme_Purple;
    }

    void UIApplyHighContrastTheme()
    {
        Style_Variable_List* style_variable_lists = s_UIState->style_variable_lists;

        style_variable_lists[StyleVar_Padding].first->value               = { 6.0f, 6.0f, 0.0f, 0.0f };
        style_variable_lists[StyleVar_Border].first->value                = { 1.0f, 1.0f, 0.0f, 0.0f };
        style_variable_lists[StyleVar_BorderColor].first->value           = { 1.0f, 1.0f, 1.0f, 1.0f };
        style_variable_lists[StyleVar_BackgroundColor].first->value       = { 0.0f, 0.0f, 0.0f, 1.0f };
        style_variable_lists[StyleVar_TextColor].first->value             = { 1.0f, 1.0f, 1.0f, 1.0f };
        style_variable_lists[StyleVar_HotBorderColor].first->value        = { 1.0f, 1.0f, 0.0f, 1.0f };
        style_variable_lists[StyleVar_HotBackgroundColor].first->value    = { 0.15f, 0.15f, 0.0f, 1.0f };
        style_variable_lists[StyleVar_HotTextColor].first->value          = { 1.0f, 1.0f, 0.0f, 1.0f };
        style_variable_lists[StyleVar_ActiveBorderColor].first->value     = { 0.0f, 1.0f, 1.0f, 1.0f };
        style_variable_lists[StyleVar_ActiveBackgroundColor].first->value = { 0.0f, 0.2f, 0.2f, 1.0f };
        style_variable_lists[StyleVar_ActiveTextColor].first->value       = { 0.0f, 1.0f, 1.0f, 1.0f };

        style_variable_lists[StyleVar_ItemSpacing].first->value          = { 2.0f, 2.0f, 0.0f, 0.0f };

        s_UIState->CurrentTheme = UITheme_HighContrast;
    }

    const char* UIGetThemeName(UITheme theme)
    {
        switch(theme)
        {
            case UITheme_Light:        return "Light";
            case UITheme_Dark:         return "Dark";
            case UITheme_Blue:         return "Blue";
            case UITheme_Green:        return "Green";
            case UITheme_Purple:       return "Purple";
            case UITheme_HighContrast: return "High Contrast";
            default:                   return "Unknown";
        }
    }

    // Labels get upper-cased and letter-spaced on their way to the screen, so a
    // test asking for "Back to Menu" must still find "B A C K  T O  M E N U".
    // Folding case and dropping spaces makes lookups survive that styling.
    static u32 UINormaliseLabel(const String8& in, char* out, u32 outSize)
    {
        u32 n = 0;
        for(u64 i = 0; i < in.size && n + 1 < outSize; i++)
        {
            const char c = (char)in.str[i];
            if(c == ' ' || c == '\t' || c == '\n')
                continue;
            out[n++] = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
        }
        out[n] = '\0';
        return n;
    }

    u32 UIFindOverlappingText(UIOverlapPair* out, u32 maxOut, float minOverlapPx)
    {
        if(!s_UIState)
            return 0;

        // Collect the visible text widgets first: the hash map is unordered and
        // we need pairwise tests, so one gather then an O(n^2) sweep over what
        // is almost always a few dozen labels.
        struct Entry
        {
            UI_Widget* w;
            Vec4 rect;
        };
        TDArray<Entry> entries(s_UIState->UIFrameArena);

        ForHashMapEach(u64, UI_Widget*, &s_UIState->widgets, it)
        {
            UI_Widget* w = *it.value;
            if(!w || !(w->flags & WidgetFlags_DrawText) || w->text.size == 0)
                continue;
            if(w->size.x <= 0.0f || w->size.y <= 0.0f)
                continue;
            if(w->ExitTransition > 0.0f) // mid fade-out, on its way off screen
                continue;
            if(w->LastFrameIndexActive != s_UIState->FrameIndex) // not built this frame
                continue;
            // Clipped away (scrolled out of its area) - not visible, not a bug.
            const Vec4& c = w->ClipRect;
            if(w->position.x >= c.z || w->position.y >= c.w
               || w->position.x + w->size.x <= c.x || w->position.y + w->size.y <= c.y)
                continue;
            Entry e;
            e.w    = w;
            e.rect = Vec4(w->position.x, w->position.y, w->size.x, w->size.y);
            entries.PushBack(e);
        }

        u32 found = 0;
        for(u32 i = 0; i < entries.Size(); i++)
        {
            for(u32 j = i + 1; j < entries.Size(); j++)
            {
                const Vec4& a = entries[i].rect;
                const Vec4& b = entries[j].rect;

                // One is nested inside the other (a label inside its own button,
                // a row inside a panel): containment is layout, not collision.
                const float ox = Maths::Min(a.x + a.z, b.x + b.z) - Maths::Max(a.x, b.x);
                const float oy = Maths::Min(a.y + a.w, b.y + b.w) - Maths::Max(a.y, b.y);
                if(ox <= minOverlapPx || oy <= minOverlapPx)
                    continue;
                if(entries[i].w->parent == entries[j].w || entries[j].w->parent == entries[i].w)
                    continue;

                if(found < maxOut && out)
                {
                    UINormaliseLabel(entries[i].w->text, out[found].A, sizeof(out[found].A));
                    UINormaliseLabel(entries[j].w->text, out[found].B, sizeof(out[found].B));
                }
                found++;
            }
        }
        return found;
    }

    u32 UIFindOffscreenText(UIOffscreenText* out, u32 maxOut, float tolPx)
    {
        if(!s_UIState)
            return 0;

        const Vec2 fb = s_UIState->root_parent.size;
        const Vec2 origin = s_UIState->root_parent.position;
        u32 found = 0;

        ForHashMapEach(u64, UI_Widget*, &s_UIState->widgets, it)
        {
            UI_Widget* w = *it.value;
            if(!w || !(w->flags & WidgetFlags_DrawText) || w->text.size == 0)
                continue;
            if(w->size.x <= 0.0f || w->size.y <= 0.0f)
                continue;
            if(w->ExitTransition > 0.0f) // mid fade-out, on its way off screen
                continue;
            if(w->LastFrameIndexActive != s_UIState->FrameIndex)
                continue;
            // Scrolled out of its own scroll area: clipped on purpose, and the
            // player can bring it back - not the same as gone off the screen.
            const Vec4& c = w->ClipRect;
            if(w->position.x >= c.z || w->position.y >= c.w
               || w->position.x + w->size.x <= c.x || w->position.y + w->size.y <= c.y)
                continue;

            const float outLeft   = origin.x - w->position.x;
            const float outTop    = origin.y - w->position.y;
            const float outRight  = (w->position.x + w->size.x) - (origin.x + fb.x);
            const float outBottom = (w->position.y + w->size.y) - (origin.y + fb.y);
            float worst = Maths::Max(Maths::Max(outLeft, outRight), Maths::Max(outTop, outBottom));
            if(worst <= tolPx)
                continue;

            if(found < maxOut && out)
            {
                UINormaliseLabel(w->text, out[found].Text, sizeof(out[found].Text));
                out[found].OutPx = worst;
            }
            found++;
        }
        return found;
    }

    u32 UIQueryWidgets(UIWidgetInfo* out, u32 maxOut, const char* textFilter)
    {
        if(!s_UIState)
            return 0;

        const Vec2 fb     = s_UIState->root_parent.size;
        const Vec2 origin = s_UIState->root_parent.position;
        u32 found         = 0;

        ForHashMapEach(u64, UI_Widget*, &s_UIState->widgets, it)
        {
            UI_Widget* w = *it.value;
            if(!w || w->LastFrameIndexActive != s_UIState->FrameIndex)
                continue;

            char label[64];
            UINormaliseLabel(w->text, label, sizeof(label));
            if(textFilter && textFilter[0] && !strstr(label, textFilter))
                continue;

            if(found < maxOut && out)
            {
                UIWidgetInfo& o = out[found];
                memcpy(o.Text, label, sizeof(o.Text));

                o.X = w->position.x;
                o.Y = w->position.y;
                o.W = w->size.x;
                o.H = w->size.y;

                o.Alpha = Maths::Clamp(w->style_vars[StyleVar_Alpha].x, 0.0f, 1.0f);

                auto copy4 = [](float* dst, const Vec4& v)
                { dst[0] = v.x; dst[1] = v.y; dst[2] = v.z; dst[3] = v.w; };
                copy4(o.BG, w->style_vars[StyleVar_BackgroundColor]);
                copy4(o.Border, w->style_vars[StyleVar_BorderColor]);
                copy4(o.TextCol, w->style_vars[StyleVar_TextColor]);
                o.CornerRadius = w->style_vars[StyleVar_CornerRadius].x;

                const Vec4& c = w->ClipRect;
                copy4(o.Clip, c);

                o.Flags = w->flags;
                o.Hot   = (s_UIState->hot_widget == w->hash);

                o.Clipped = (w->position.x >= c.z || w->position.y >= c.w
                             || w->position.x + w->size.x <= c.x
                             || w->position.y + w->size.y <= c.y);

                o.Offscreen = (w->position.x < origin.x - 1.0f
                               || w->position.y < origin.y - 1.0f
                               || w->position.x + w->size.x > origin.x + fb.x + 1.0f
                               || w->position.y + w->size.y > origin.y + fb.y + 1.0f);
            }
            found++;
        }
        return found;
    }

    u32 UIGetClickableRects(Vec4* out, u32 max)
    {
        u32 count       = 0;
        const Vec2 root = s_UIState->root_parent.size;

        // a full-screen clickable is a scrim: what it covers can't be reached
        u64 scrim = 0;
        ForHashMapEach(u64, UI_Widget*, &s_UIState->widgets, find)
        {
            UI_Widget* w = *find.value;
            if(w && (w->flags & WidgetFlags_Clickable) && w->LastFrameIndexActive + 1 >= s_UIState->FrameIndex
               && w->ExitTransition <= 0.0f && w->size.x >= root.x * 0.9f && w->size.y >= root.y * 0.9f)
                scrim = Maths::Max(scrim, w->BuildIndex);
        }

        ForHashMapEach(u64, UI_Widget*, &s_UIState->widgets, it)
        {
            UI_Widget* w = *it.value;
            if(count >= max)
                break;
            if(!w || w->BuildIndex < scrim)
                continue;
            if(!(w->flags & WidgetFlags_Clickable) || w->Clip)
                continue;
            // callers are usually mid-build, before this frame's widgets exist: last frame's count
            if(w->LastFrameIndexActive + 1 < s_UIState->FrameIndex || w->ExitTransition > 0.0f)
                continue;
            if(w->size.x <= 0.0f || w->size.y <= 0.0f)
                continue;
            if(w->size.x >= root.x * 0.9f && w->size.y >= root.y * 0.9f) // a scrim, not a control
                continue;
            const Vec4& c = w->ClipRect;
            const float x0 = Maths::Max(w->position.x, c.x), y0 = Maths::Max(w->position.y, c.y);
            const float x1 = Maths::Min(w->position.x + w->size.x, c.z), y1 = Maths::Min(w->position.y + w->size.y, c.w);
            if(x1 <= x0 || y1 <= y0)
                continue;
            const Vec2 p = Vec2(x0, y0) + s_UIState->InputOffset;
            out[count++] = Vec4(p.x, p.y, x1 - x0, y1 - y0);
        }
        return count;
    }

    bool UIFindWidgetByText(const char* text, Vec2* outMousePos, Vec4* outRect, bool substring)
    {
        if(!s_UIState || !text)
            return false;

        char needle[256];
        if(UINormaliseLabel(Str8C((char*)text), needle, sizeof(needle)) == 0)
            return false;

        UI_Widget* match = nullptr;

        ForHashMapEach(u64, UI_Widget*, &s_UIState->widgets, it)
        {
            UI_Widget* w = *it.value;
            if(!w || w->size.x <= 0.0f || w->size.y <= 0.0f)
                continue;
            if(w->ExitTransition > 0.0f) // mid fade-out, not clickable
                continue;
            // widgets stay in the map after they stop being built; only what's on screen counts
            if(w->LastFrameIndexActive + 1 < s_UIState->FrameIndex)
                continue;

            char candidate[256];
            if(UINormaliseLabel(w->text, candidate, sizeof(candidate)) == 0)
                continue;

            if(substring)
            {
                if(!strstr(candidate, needle))
                    continue;
            }
            else if(strcmp(candidate, needle) != 0)
                continue;

            // Prefer something actually clickable - a plain label often carries
            // the same text as the button that owns it.
            if(w->flags & WidgetFlags_Clickable)
            {
                match = w;
                break;
            }
            if(!match)
                match = w;
        }

        if(!match)
            return false;

        if(outRect)
            *outRect = Vec4(match->position.x, match->position.y, match->size.x, match->size.y);

        if(outMousePos)
        {
            // Undo the layout-space transform so the result can go straight into Input.
            const Vec2 centre = match->position + match->size * 0.5f;
            const Vec2 scaled = centre + s_UIState->InputOffset;
            const f32 inv     = (s_UIState->InputScale != 0.0f) ? 1.0f / s_UIState->InputScale : 1.0f;
            *outMousePos      = scaled * inv;
        }

        return true;
    }

    bool UIIsMouseOverUI()
    {
        if(!s_UIState)
            return false;
        if(s_UIState->hot_widget != 0)
            return true;
        Vec2 m = Input::Get().GetMousePosition() * s_UIState->InputScale - s_UIState->InputOffset;
        for(UI_Widget* w = s_UIState->root_parent.first; w; w = w->next)
        {
            if(w->size.x <= 0.0f || w->size.y <= 0.0f)
                continue;
            if(m.x >= w->position.x && m.x <= w->position.x + w->size.x
               && m.y >= w->position.y && m.y <= w->position.y + w->size.y)
                return true;
        }
        return false;
    }

    void UISetSafeAreaConfig(const UISafeAreaConfig& cfg)
    {
        s_UIState->SafeArea = cfg;
    }

    UISafeAreaConfig UIGetSafeAreaConfig()
    {
        return s_UIState->SafeArea;
    }

    void UISetLayeredBatching(bool enable)
    {
        s_UIState->LayeredBatching = enable;
    }

    void UIPushMonoText()
    {
        if(s_UIState)
            s_UIState->MonoTextDepth++;
    }

    void UIPopMonoText()
    {
        if(s_UIState && s_UIState->MonoTextDepth > 0)
            s_UIState->MonoTextDepth--;
    }

    void UISetClickOnRelease(bool enable)
    {
        if(s_UIState)
            s_UIState->ClickOnRelease = enable;
    }

    void UISetAccentColour(const Vec4& colour)
    {
        s_UIState->AccentColour = colour;
    }

    void UISetTheme(UITheme theme)
    {
        switch(theme)
        {
            case UITheme_Light:        UIApplyLightTheme(); break;
            case UITheme_Dark:         UIApplyDarkTheme(); break;
            case UITheme_Blue:         UIApplyBlueTheme(); break;
            case UITheme_Green:        UIApplyGreenTheme(); break;
            case UITheme_Purple:       UIApplyPurpleTheme(); break;
            case UITheme_HighContrast: UIApplyHighContrastTheme(); break;
            default: break;
        }

        // Reset non-theme style vars to base values before DPI scaling
        auto* styles = s_UIState->style_variable_lists;
#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS)
        styles[StyleVar_FontSize].first->value      = { 22.0f, 0.0f, 0.0f, 1.0f };
#else
        styles[StyleVar_FontSize].first->value      = { 20.0f, 0.0f, 0.0f, 1.0f };
#endif
        styles[StyleVar_CornerRadius].first->value   = { 6.0f, 0.0f, 0.0f, 0.0f };
        styles[StyleVar_ShadowColor].first->value    = { 0.0f, 0.0f, 0.0f, 0.15f };
        styles[StyleVar_ShadowOffset].first->value   = { 0.0f, 1.0f, 0.0f, 0.0f };
        styles[StyleVar_ShadowBlur].first->value     = { 4.0f, 0.0f, 0.0f, 0.0f };
        styles[StyleVar_ItemSpacing].first->value    = { 2.0f, 2.0f, 0.0f, 0.0f };

        // Apply DPI scaling
        float dpi = s_UIState->DPIScale;
        if(dpi > 1.0f)
        {
            styles[StyleVar_FontSize].first->value.x *= dpi;
            styles[StyleVar_Padding].first->value.x *= dpi;
            styles[StyleVar_Padding].first->value.y *= dpi;
            styles[StyleVar_Border].first->value.x *= dpi;
            styles[StyleVar_Border].first->value.y *= dpi;
            styles[StyleVar_CornerRadius].first->value.x *= dpi;
            styles[StyleVar_ShadowOffset].first->value.y *= dpi;
            styles[StyleVar_ShadowBlur].first->value.x *= dpi;
            styles[StyleVar_ItemSpacing].first->value.x *= dpi;
            styles[StyleVar_ItemSpacing].first->value.y *= dpi;
        }
    }
}
