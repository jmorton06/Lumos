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

    bool InitialiseUI(Arena* arena)
    {
        s_UIState                  = PushArray(arena, UI_State, 1);
        s_UIState->active_widget   = 0;
        s_UIState->hot_widget      = 0;
        s_UIState->next_hot_widget = 0;
        s_UIState->FrameIndex      = 0;
        s_UIState->AnimationRate   = 10.0f;

        s_UIState->UIArena      = arena;
        // Every widget's label and id string is formatted in here each frame, so
        // a text-heavy panel over a big scene needs real headroom.
        s_UIState->UIFrameArena = ArenaAlloc(Megabytes(4));
        s_UIState->UIExitArena  = ArenaAlloc(Megabytes(1));

        s_UIState->WidgetAllocator = new PoolAllocator<UI_Widget>();
        s_UIState->parents         = TDArray<UI_Widget*>(arena);

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
            ArenaRelease(s_UIState->UIExitArena);
            delete s_UIState->WidgetAllocator;
            s_UIState->WidgetAllocator = nullptr;
        }
    }

    static bool UIThemeIsDark()
    {
        const Vec4& bg = s_UIState->style_variable_lists[StyleVar_BackgroundColor].first->value;
        return (bg.x * 0.299f + bg.y * 0.587f + bg.z * 0.114f) < 0.5f;
    }

    void UISetNextFlags(u32 flags)
    {
        s_UIState->NextWidgetFlags |= flags;
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

    static UI_Widget*
    GetCurrentParent()
    {
        return s_UIState->parents.Back();
    }

    static void PushParent(UI_Widget* widget)
    {
        s_UIState->parents.PushBack(widget);
    }

    static void PopParent(UI_Widget* widget)
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

    static u64 UIHashCombine(u64 a, u64 b)
    {
        return a ^ (b + 0x9E3779B97F4A7C15ULL + (a << 6) + (a >> 2));
    }

    static UI_Widget* PushWidget(u32 flags,
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
        // Rebuilt while mid-exit-fade = revived.
        widget->ExitTransition = 0.0f;

        widget->parent               = parent;
        widget->flags                = flags | s_UIState->NextWidgetFlags;
        s_UIState->NextWidgetFlags   = 0;
        widget->text                 = text;
        widget->hash                 = hash;
        widget->texture              = nullptr;
        widget->LastFrameIndexActive = s_UIState->FrameIndex;
        widget->ActiveTransition     = 0.0f;
        widget->Anchor               = UIAnchor_None;
        widget->AnchorMargin         = Vec2(0.0f, 0.0f);
        widget->Rotation             = 0.0f;
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

    static UI_Interaction HandleWidgetInteraction(UI_Widget* widget)
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

    SafeAreaInsets UIResolveSafeAreaInsets(const SafeAreaInsets& raw, bool isEditor)
    {
        SafeAreaInsets sa = raw;

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

    void UIBeginFrame(const Vec2& frame_buffer_size, f32 dt, const Vec2& inputOffset)
    {
        LUMOS_PROFILE_FUNCTION();
        ArenaClear(s_UIState->UIFrameArena);

        s_UIState->InputOffset         = inputOffset;
        s_UIState->FrameBufferSize     = frame_buffer_size;
        s_UIState->next_hot_widget     = 0;
        s_UIState->OverlayBlocksInput  = false;
        s_UIState->CurrentTime += dt;
        s_UIState->parents.Clear();
        s_UIState->FrameIndex++;
        s_UIState->AnimationRateDT = s_UIState->AnimationRate * dt;
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
                    if(d.x * d.x + d.y * d.y > slop * slop)
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
        ForHashMapEach(u64, UI_Widget*, &s_UIState->widgets, it)
        {
            u64 key          = *it.key;
            UI_Widget* value = *it.value;

            if(key == s_UIState->hot_widget || key == s_UIState->active_widget)
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

    UI_Interaction UIBeginPanel(const char* str, u32 extraFlags)
    {
        return UIBeginPanel(str, SizeKind_MaxChild, 1.0f, SizeKind_ChildSum, 1.0f, extraFlags);
    }

    UI_Interaction UIBeginPanel(const char* str, SizeKind sizeKindX, float xValue, SizeKind sizeKindY, float yValue, u32 extraFlags)
    {
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        String8 WindowText = PushStr8F(s_UIState->UIFrameArena, "Window###window%s", (char*)text.str);
        u64 hashWindow;
        String8 WindowText2 = HandleUIString((char*)WindowText.str, &hashWindow);
        UI_Widget* window   = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | extraFlags,
                                         WindowText2,
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

        String8 HeaderText = PushStr8F(s_UIState->UIFrameArena, "Header###header%s", (char*)text.str);
        u64 hashHeader;
        String8 HeaderText2 = HandleUIString((char*)HeaderText.str, &hashHeader);
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

        String8 windowText = PushStr8F(s_UIState->UIFrameArena, "Overlay###overlay%s", (char*)text.str);
        u64 hashWindow;
        String8 windowText2 = HandleUIString((char*)windowText.str, &hashWindow);
        UI_Widget* window   = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | extraFlags,
                                         windowText2,
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

    UI_Interaction UILabel(const char* str, const String8& text)
    {
        u64 hash;
        HandleUIString(str, &hash);

        UI_Widget* widget = PushWidget(WidgetFlags_DrawText,
                                       text,
                                       hash,
                                       { SizeKind_TextContent, 1.0f },
                                       { SizeKind_TextContent, 1.0f });

        widget->style_vars[StyleVar_Padding] = Vec4(2.0f * s_UIState->DPIScale, 2.0f * s_UIState->DPIScale, 0.0f, 0.0f);

        return HandleWidgetInteraction(widget);
    }

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

    UI_Interaction UILabelEllipsis(const char* str, const String8& text, float maxWidth)
    {
        maxWidth *= s_UIState->DPIScale;
        const f32 fontSize = s_UIState->style_variable_lists[StyleVar_FontSize].last->value.x;

        if(GetStringSize(text, fontSize).x <= maxWidth)
            return UILabel(str, text);

        const f32 budget = maxWidth - GetStringSize(Str8Lit("..."), fontSize).x;

        // Longest codepoint-aligned prefix that fits the budget.
        u64 fit = 0;
        u64 ci  = 0;
        while(ci < text.size)
        {
            UnicodeDecode dec = Utf8Decode(text.str + ci, text.size - ci);
            u64 cl            = dec.inc > 0 ? (u64)dec.inc : 1;
            String8 prefix;
            prefix.str  = text.str;
            prefix.size = ci + cl;
            if(GetStringSize(prefix, fontSize).x > budget)
                break;
            ci += cl;
            fit = ci;
        }

        String8 truncated = PushStr8F(s_UIState->UIFrameArena, "%.*s...", (int)fit, (const char*)text.str);
        return UILabel(str, truncated);
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
            (void)widget;
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
            float knobSize   = textHeight - 4.0f * dpi;

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
                (void)val;
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
            PushWidget(WidgetFlags_DrawText | WidgetFlags_CentreY,
                       slider_label,
                       labelHash,
                       { SizeKind_TextContent, 1.0f },
                       { SizeKind_TextContent, 1.0f });
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

        s_ScrollAreaWidget = area;
        s_ScrollAreaOffset = scroll_offset;
        s_ScrollAreaHeight = height;

        PushParent(area);
    }

    void UIEndScrollArea()
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
            contentHeight += child->size.y;
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
        if(hovering)
        {
            float scrollDelta = Input::Get().GetScrollOffset() * 30.0f * dpi;
            *s_ScrollAreaOffset -= scrollDelta;
        }

        if(s_UIState->ClickOnRelease && maxScroll > 0.0f)
        {
            const u64 areaHash = s_ScrollAreaWidget->hash;
            const bool held    = Input::Get().GetMouseHeld(Lumos::InputCode::MouseKey::ButtonLeft);
            if(held && (hovering || s_ScrollDragHash == areaHash))
            {
                if(s_ScrollDragHash == areaHash)
                    *s_ScrollAreaOffset -= (mousePos.y - s_ScrollDragLastY);
                s_ScrollDragHash  = areaHash;
                s_ScrollDragLastY = mousePos.y;
            }
            else if(s_ScrollDragHash == areaHash)
            {
                s_ScrollDragHash = 0;
            }
        }

        *s_ScrollAreaOffset = Maths::Clamp(*s_ScrollAreaOffset, 0.0f, maxScroll);

        // Only show scrollbar if content exceeds visible area
        if(contentHeight > visibleHeight)
        {
            float scrollbarWidth = 8.0f * dpi;
            float trackHeight = visibleHeight;
            float thumbRatio = visibleHeight / contentHeight;
            float thumbHeight = Maths::Max(20.0f * dpi, trackHeight * thumbRatio);
            float scrollRatio = (maxScroll > 0.0f) ? (*s_ScrollAreaOffset / maxScroll) : 0.0f;
            float thumbOffset = scrollRatio * (trackHeight - thumbHeight);

            // Scrollbar track
            u64 scrollId = s_UIState->WidgetIdCounter++;
            String8 trackId = PushStr8F(s_UIState->UIFrameArena, "scrolltrack###st%llu", scrollId);
            u64 trackHash;
            HandleUIString((char*)trackId.str, &trackHash);

            UI_Widget* track = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                          Str8Lit(""),
                                          trackHash,
                                          { SizeKind_Pixels, scrollbarWidth },
                                          { SizeKind_Pixels, trackHeight });

            track->relative_position = Vec2(s_ScrollAreaWidget->size.x - scrollbarWidth - 2.0f * dpi, padding);
            track->style_vars[StyleVar_Padding] = Vec4(0.0f);

            if(UIThemeIsDark())
                track->style_vars[StyleVar_BackgroundColor] = Vec4(0.2f, 0.2f, 0.2f, 0.5f);
            else
                track->style_vars[StyleVar_BackgroundColor] = Vec4(0.85f, 0.85f, 0.85f, 0.5f);

            track->style_vars[StyleVar_CornerRadius] = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);

            String8 thumbId = PushStr8F(s_UIState->UIFrameArena, "scrollthumb###sth%llu", scrollId);
            u64 thumbHash;
            HandleUIString((char*)thumbId.str, &thumbHash);

            UI_Widget* thumb = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | WidgetFlags_Clickable,
                                          Str8Lit(""),
                                          thumbHash,
                                          { SizeKind_Pixels, scrollbarWidth - 2.0f * dpi },
                                          { SizeKind_Pixels, thumbHeight });

            thumb->relative_position = Vec2(s_ScrollAreaWidget->size.x - scrollbarWidth - 1.0f * dpi, padding + thumbOffset);
            thumb->style_vars[StyleVar_Padding] = Vec4(0.0f);

            if(UIThemeIsDark())
                thumb->style_vars[StyleVar_BackgroundColor] = Vec4(0.5f, 0.5f, 0.5f, 0.8f);
            else
                thumb->style_vars[StyleVar_BackgroundColor] = Vec4(0.6f, 0.6f, 0.6f, 0.8f);

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
    static std::string s_NativeKeyboardText;
    static u64 s_NativeKeyboardHash = 0;
#endif

    // One-shot: the next UITextInput built claims keyboard focus.
    static bool s_ClaimNextTextInputFocus = false;

    void UIFocusNextTextInput()
    {
        s_ClaimNextTextInputFocus = true;
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
        if(isFocused && s_NativeKeyboardHash == fieldHash)
        {
            strncpy(buffer, s_NativeKeyboardText.c_str(), buffer_size - 1);
            buffer[buffer_size - 1]    = 0;
            s_UIState->TextInputCursor = (u32)strlen(buffer);
        }
#endif

        String8 displayStr = Str8C(buffer);

        if(isFocused)
        {
            // Show cursor as | character
            u32 cursorIdx       = s_UIState->TextInputCursor;
            u32 len             = (u32)strlen(buffer);
            cursorIdx           = Maths::Min(cursorIdx, len);
            String8 displayText = PushStr8F(s_UIState->UIFrameArena, "%.*s|%s",
                                            cursorIdx, buffer,
                                            buffer + cursorIdx);
            displayStr          = displayText;
        }

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
            s_NativeKeyboardHash = fieldHash;
            OpeniOSKeyboard(&s_NativeKeyboardText);
#endif
        }

        // Update external cursor if provided
        if(cursor_pos && isFocused)
        {
            *cursor_pos = s_UIState->TextInputCursor;
        }

        return interaction;
    }

    void UIProcessKeyTyped(char character)
    {
        if(s_UIState->FocusedTextInput == 0 || !s_UIState->TextInputBuffer)
            return;

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
        // Tab focus navigation (works even without focused text input)
        if(key == InputCode::Key::Tab)
        {
            if(Input::Get().GetKeyHeldRaw(InputCode::Key::LeftShift) || Input::Get().GetKeyHeldRaw(InputCode::Key::RightShift))
                s_UIState->ShiftTabPressed = true;
            else
                s_UIState->TabPressed = true;
            return;
        }

        if(s_UIState->FocusedTextInput == 0 || !s_UIState->TextInputBuffer)
            return;

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

        switch(key)
        {
        case InputCode::Key::Backspace:
            if(!deleteSelection() && s_UIState->TextInputCursor > 0)
            {
                u32 cursor = s_UIState->TextInputCursor;
                for(u32 i = cursor - 1; i < len; i++)
                    s_UIState->TextInputBuffer[i] = s_UIState->TextInputBuffer[i + 1];
                s_UIState->TextInputCursor--;
            }
            break;

        case InputCode::Key::Delete:
            if(!deleteSelection() && s_UIState->TextInputCursor < len)
            {
                u32 cursor = s_UIState->TextInputCursor;
                for(u32 i = cursor; i < len; i++)
                    s_UIState->TextInputBuffer[i] = s_UIState->TextInputBuffer[i + 1];
            }
            break;

        case InputCode::Key::Left:
            if(s_UIState->TextInputCursor > 0)
                updateSelection(s_UIState->TextInputCursor - 1);
            break;

        case InputCode::Key::Right:
            if(s_UIState->TextInputCursor < len)
                updateSelection(s_UIState->TextInputCursor + 1);
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

            UI_Widget* list = PushWidget(WidgetFlags_StackVertically | WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
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
                s_UIState->TooltipText = Str8C((char*)text);
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
            s_UIState->TooltipPos = Input::Get().GetMousePosition() * s_UIState->InputScale - s_UIState->InputOffset;
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

        UI_Widget* menu = PushWidget(WidgetFlags_StackVertically | WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
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

    Vec2 GetStringSize(String8 text, float size)
    {
        Vec2 sizeVec = { size * text.size, size };
        return Graphics::Font::GetDefaultFont()->CalculateTextSize(text, size); // sizeVec;
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
                Vec2 text_size = GetStringSize(Widget->text, fontSize);
                if(Axis == UIAxis_Y)
                    text_size.y = Maths::Max(text_size.y, GetStringSize(Str8Lit("Ay"), fontSize).y);
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
    static u64 s_DownwardsSizePass = 0;

    static void UILayoutSolveDownwardsSizesInner(UI_Widget* Root, UIAxis Axis)
    {
        for(UI_Widget* Widget = Root; Widget; Widget = UIWidgetRecurseDepthFirstPostOrder(Widget, Root))
        {
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

                    if(Child->semantic_size[Axis].kind == SizeKind_ChildSum || Child->semantic_size[Axis].kind == SizeKind_MaxChild)
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

                    if(Child->semantic_size[Axis].kind == SizeKind_ChildSum || Child->semantic_size[Axis].kind == SizeKind_MaxChild)
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
        // Mark the beginning of UI building phase
        // This can be used for validation or setup if needed
    }

    void UIEndBuild()
    {
        LUMOS_PROFILE_FUNCTION();

        // Create deferred tooltip at root level (renders on top of everything)
        if(s_UIState->ShowTooltip && s_UIState->TooltipText.size > 0)
        {
            // Push directly to root
            s_UIState->parents.PushBack(&s_UIState->root_parent);

            String8 tipId             = PushStr8F(s_UIState->UIFrameArena, "tooltip###tip%llu", s_UIState->WidgetIdCounter++);
            u64 hash;
            HandleUIString((char*)tipId.str, &hash);

            UI_Widget* tooltip = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_DrawText | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                            s_UIState->TooltipText,
                                            hash,
                                            { SizeKind_TextContent, 1.0f },
                                            { SizeKind_TextContent, 1.0f });

            {float dp = s_UIState->DPIScale;
            tooltip->relative_position               = s_UIState->TooltipPos + Vec2(10.0f * dp, 10.0f * dp);
            tooltip->style_vars[StyleVar_Padding]      = Vec4(4.0f * dp, 3.0f * dp, 0.0f, 0.0f);
            tooltip->style_vars[StyleVar_CornerRadius] = Vec4(3.0f * dp, 0.0f, 0.0f, 0.0f);}

            if(UIThemeIsDark())
            {
                tooltip->style_vars[StyleVar_BackgroundColor] = Vec4(0.1f, 0.1f, 0.1f, 0.95f);
                tooltip->style_vars[StyleVar_BorderColor]     = Vec4(0.4f, 0.4f, 0.4f, 1.0f);
                tooltip->style_vars[StyleVar_TextColor]       = Vec4(0.9f, 0.9f, 0.9f, 1.0f);
            }
            else
            {
                tooltip->style_vars[StyleVar_BackgroundColor] = Vec4(1.0f, 1.0f, 0.9f, 0.98f);
                tooltip->style_vars[StyleVar_BorderColor]     = Vec4(0.5f, 0.5f, 0.4f, 1.0f);
                tooltip->style_vars[StyleVar_TextColor]       = Vec4(0.1f, 0.1f, 0.1f, 1.0f);
            }

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
        if(isFocused && s_NativeKeyboardHash == fieldHash)
        {
            strncpy(buffer, s_NativeKeyboardText.c_str(), buffer_size - 1);
            buffer[buffer_size - 1]    = 0;
            s_UIState->TextInputCursor = (u32)strlen(buffer);
        }
#endif

        String8 displayStr;
        const bool showPlaceholder = (len == 0) && placeholder && placeholder[0];
        if(showPlaceholder)
        {
            displayStr = isFocused ? PushStr8F(s_UIState->UIFrameArena, "|%s", placeholder)
                                   : Str8C((char*)placeholder);
        }
        else if(isFocused)
        {
            u32 cursorIdx = Maths::Min(s_UIState->TextInputCursor, (u32)strlen(buffer));
            displayStr    = PushStr8F(s_UIState->UIFrameArena, "%.*s|%s", cursorIdx, buffer, buffer + cursorIdx);
        }
        else
        {
            displayStr = Str8C(buffer);
        }

        UI_Size sizeX = width > 0.0f ? UI_Size{ SizeKind_Pixels, width * dpi }
                                     : UI_Size{ SizeKind_Grow, 1.0f };

        UI_Widget* field = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground | WidgetFlags_DrawText | WidgetFlags_CentreY,
                                      displayStr,
                                      hash,
                                      sizeX,
                                      { SizeKind_TextContent, 1.0f });

        field->style_vars[StyleVar_Padding]      = Vec4(10.0f * dpi, 8.0f * dpi, 0.0f, 0.0f);
        field->style_vars[StyleVar_Border]       = Vec4(0.0f);
        field->style_vars[StyleVar_BorderColor]       = Vec4(0.0f);
        field->style_vars[StyleVar_HotBorderColor]    = Vec4(0.0f);
        field->style_vars[StyleVar_ActiveBorderColor] = Vec4(0.0f);
        field->style_vars[StyleVar_CornerRadius] = Vec4(6.0f * dpi, 0.0f, 0.0f, 0.0f);

        const bool dark = UIThemeIsDark();
        Vec4 bg   = dark ? Vec4(1.0f, 1.0f, 1.0f, 0.06f) : Vec4(0.0f, 0.0f, 0.0f, 0.05f);
        Vec4 text = dark ? Vec4(0.95f, 0.95f, 0.97f, 1.0f) : Vec4(0.08f, 0.08f, 0.10f, 1.0f);
        if(showPlaceholder)
            text.w = 0.45f;

        field->style_vars[StyleVar_BackgroundColor]       = bg;
        field->style_vars[StyleVar_HotBackgroundColor]    = bg;
        field->style_vars[StyleVar_ActiveBackgroundColor] = bg;
        field->style_vars[StyleVar_TextColor]             = text;
        field->style_vars[StyleVar_HotTextColor]          = text;
        field->style_vars[StyleVar_ActiveTextColor]       = text;

        s_UIState->FocusableWidgets.PushBack(fieldHash);

        UI_Interaction interaction = HandleWidgetInteraction(field);

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

    void UISetClickOnRelease(bool enable)
    {
        if(s_UIState)
            s_UIState->ClickOnRelease = enable;
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
