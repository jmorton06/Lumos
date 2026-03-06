#include "Precompiled.h"
#include "UI.h"
#include "Core/OS/Input.h"
#include "Font.h"
#include "Maths/MathsUtilities.h"
#include "Utilities/StringUtilities.h"
#include "Graphics/RHI/Texture.h"
#include "Core/Application.h"
#include "Core/OS/Window.h"

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
        s_UIState->UIFrameArena = ArenaAlloc(Megabytes(1));

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

        style_variable_lists[StyleVar_Padding].first->value               = { 10.0f, 7.0f, 0.0f, 0.0f };
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
        }
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

    static UI_Widget* PushWidget(u32 flags,
                                 const String8& text,
                                 u64 hash,
                                 UI_Size semantic_size_x,
                                 UI_Size semantic_size_y)
    {
        UI_Widget* parent = GetCurrentParent();

        UI_Widget* widget = nullptr;
        HashMapFind(&s_UIState->widgets, hash, &widget);
        if(!widget)
        {
            void* mem                = s_UIState->WidgetAllocator->Allocate();
            widget                   = new(mem) UI_Widget();
            widget->HotTransition    = 0.0f;
            widget->ActiveTransition = 0.0f;
            widget->ToggleTransition = 0.0f;
            widget->ScaleAnimation   = 1.0f;
            HashMapInsert(&s_UIState->widgets, hash, widget);
        }

        widget->parent               = parent;
        widget->flags                = flags;
        widget->text                 = text;
        widget->hash                 = hash;
        widget->texture              = nullptr;
        widget->LastFrameIndexActive = s_UIState->FrameIndex;
        widget->ActiveTransition     = 0.0f;

        widget->first = NULL;
        widget->last  = NULL;
        widget->next  = NULL;
        widget->prev  = NULL;

        widget->semantic_size[UIAxis_X] = semantic_size_x;
        widget->semantic_size[UIAxis_Y] = semantic_size_y;

        if(!parent->first)
        {
            parent->first = widget;
            parent->last  = widget;
        }
        else
        {
            if(!widget->parent->last)
                widget->prev = parent->first;
            else
                widget->prev = widget->parent->last;

            if(widget->prev)
            {
                widget->prev->next = widget;
            }
            parent->last = widget;
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
        const Vec2& mouse    = Input::Get().GetMousePosition() * s_UIState->DPIScale - s_UIState->InputOffset; // Needs to take Quality setting render scale into accoutn too

        bool hovering = mouse.x >= position.x && mouse.x <= position.x + size.x && mouse.y >= position.y && mouse.y <= position.y + size.y;
        if(hovering)
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

        interaction.clicked        = widget->clicked;
        interaction.right_clicked  = widget->right_clicked;
        interaction.double_clicked = widget->double_clicked;
        interaction.dragging       = widget->dragging;

        return interaction;
    }

    void UIBeginFrame(const Vec2& frame_buffer_size, f32 dt, const Vec2& inputOffset)
    {
        LUMOS_PROFILE_FUNCTION();
        ArenaClear(s_UIState->UIFrameArena);

        s_UIState->InputOffset     = inputOffset;
        s_UIState->next_hot_widget = 0;
        s_UIState->CurrentTime += dt;
        s_UIState->parents.Clear();
        s_UIState->FrameIndex++;
        s_UIState->AnimationRateDT = s_UIState->AnimationRate * dt;
        s_UIState->TreeIndentLevel = 0;
        s_UIState->FocusableWidgets.Clear();

        // Handle dragging BEFORE building widgets so position updates are immediate
        if(s_UIState->active_widget && s_UIState->active_widget_state)
        {
            Vec2 mouse_p = Input::Get().GetMousePosition() * s_UIState->DPIScale - inputOffset;

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

                        Vec2 min_p = parent ? parent->position : Vec2(0.0f, 0.0f);
                        Vec2 max_p = parent ? (parent->size - (widget->size - s_UIState->active_widget_state->drag_offset)) : frame_buffer_size;

                        if(s_UIState->active_widget_state->drag_constraint_x)
                            mouse_p.x = s_UIState->active_widget_state->drag_mouse_p.x;

                        if(s_UIState->active_widget_state->drag_constraint_y)
                            mouse_p.y = s_UIState->active_widget_state->drag_mouse_p.y;

                        mouse_p = Maths::Clamp(mouse_p, min_p, max_p);
                        s_UIState->active_widget_state->relative_position = (mouse_p - s_UIState->active_widget_state->drag_offset);
                    }
                }
            }
        }

        UI_Widget* root_parent                    = &s_UIState->root_parent;
        root_parent->semantic_size[UIAxis_X]      = { SizeKind_Pixels, frame_buffer_size.x };
        root_parent->semantic_size[UIAxis_Y]      = { SizeKind_Pixels, frame_buffer_size.y };
        root_parent->hash                         = HashUIStr8Name(Str8Lit("root"));
        root_parent->flags                        = WidgetFlags_StackVertically;
        root_parent->text                         = Str8Lit("root");
        root_parent->style_vars[StyleVar_Padding] = { 0.0f, 0.0f, 0.0f, 0.0f };
        root_parent->style_vars[StyleVar_Border]  = { 0.0f, 0.0f, 0.0f, 0.0f };
        root_parent->cursor                       = { 0.0f, 0.0f };
        root_parent->position                     = { 0.0f, 0.0f };
        root_parent->size                         = frame_buffer_size;
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
        Vec2 mouse_p = input->GetMousePosition() * s_UIState->DPIScale - s_UIState->InputOffset;

        if(!s_UIState->root_parent.first)
        {
            return;
        }

        // Reset right_clicked for all widgets at start of frame processing
        ForHashMapEach(u64, UI_Widget*, &s_UIState->widgets, it)
        {
            (*it.value)->right_clicked = false;
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
                s_UIState->active_widget_state->clicked  = false;
                s_UIState->active_widget_state->dragging = false;
                s_UIState->active_widget_state->drag_mouse_p = { 0.0f, 0.0f };
                s_UIState->active_widget_state->drag_offset  = { 0.0f, 0.0f };
                s_UIState->active_widget = 0;
            }
        }

        if(!s_UIState->active_widget)
        {
            s_UIState->hot_widget = s_UIState->next_hot_widget;
            if(s_UIState->hot_widget)
            {
                if(Input::Get().GetMouseClicked(Lumos::InputCode::MouseKey::ButtonLeft))
                {
                    s_UIState->active_widget = s_UIState->hot_widget;
                    s_UIState->hot_widget    = 0;

                    HashMapFind(&s_UIState->widgets, s_UIState->active_widget, &s_UIState->active_widget_state);

                    if(s_UIState->active_widget_state)
                    {
                        s_UIState->active_widget_state->clicked = true;

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

                    // Close context menu on left click elsewhere
                    s_UIState->ContextMenuOpen = false;
                }

                // Right-click detection
                if(Input::Get().GetMouseClicked(Lumos::InputCode::MouseKey::ButtonRight))
                {
                    UI_Widget* hotWidget = nullptr;
                    HashMapFind(&s_UIState->widgets, s_UIState->hot_widget, &hotWidget);
                    if(hotWidget)
                    {
                        hotWidget->right_clicked = true;
                        s_UIState->ContextMenuPos = Input::Get().GetMousePosition() * s_UIState->DPIScale - s_UIState->InputOffset;
                        s_UIState->ContextMenuTrigger = s_UIState->hot_widget;
                    }
                }
            }
        }

        TDArray<UI_Widget*> lWidgetsToDelete(s_UIState->UIFrameArena);
        ForHashMapEach(u64, UI_Widget*, &s_UIState->widgets, it)
        {
            u64 key          = *it.key;
            UI_Widget* value = *it.value;

            if(key == s_UIState->hot_widget || key == s_UIState->active_widget)
            {
                if(key == s_UIState->hot_widget)
                    value->HotTransition += s_UIState->AnimationRateDT;
                else
                    value->ActiveTransition += s_UIState->AnimationRateDT;
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

            if(value->LastFrameIndexActive < s_UIState->FrameIndex)
            {
                lWidgetsToDelete.PushBack(value);
            }
        }

        for(auto widget : lWidgetsToDelete)
        {
            HashMapRemove(&s_UIState->widgets, widget->hash);
            s_UIState->WidgetAllocator->Deallocate(widget);
        }

        // Reset tab navigation state at end of frame
        s_UIState->TabPressed = false;
        s_UIState->ShiftTabPressed = false;
    }

    String8 HandleUIString(const char* str, u64* out_hash)
    {
        String8 text = Str8C((char*)str);                          //{ (char *)str, strlen(str) };
        *out_hash    = StringUtilities::BasicHashFromString(text); // hash(text);

        u64 last_hash = FindSubstr8(text, Str8Lit("#"), 0, MatchFlags::FindLast);

        // u32 last_hash = find_last_any_char(&text, "#");
        if(last_hash != text.size)
        {
            text = Substr8(text, { 0, last_hash /*- 1*/ });
        }
        return text;
    }

    UI_Interaction UIBeginPanel(const char* str)
    {
        return UIBeginPanel(str, SizeKind_MaxChild, 1.0f, SizeKind_ChildSum, 1.0f, 0);
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
        window->style_vars[StyleVar_CornerRadius] = Vec4(6.0f * dpi, 0.0f, 0.0f, 0.0f);
        window->style_vars[StyleVar_Border]       = Vec4(1.0f * dpi, 1.0f * dpi, 0.0f, 0.0f);
        window->style_vars[StyleVar_ShadowColor]  = Vec4(0.0f, 0.0f, 0.0f, 0.3f);
        window->style_vars[StyleVar_ShadowOffset] = Vec4(2.0f * dpi, 2.0f * dpi, 0.0f, 0.0f);
        window->style_vars[StyleVar_ShadowBlur]   = Vec4(5.0f * dpi, 0.0f, 0.0f, 0.0f);
        
        if(s_UIState->CurrentTheme == UITheme_Dark)
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
        if(s_UIState->CurrentTheme == UITheme_Dark)
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

    UI_Interaction UIButton(const char* str)
    {
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        UI_Widget* widget = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawText | WidgetFlags_DrawBorder | WidgetFlags_DrawBackground | WidgetFlags_AnimateScale,
                                       text,
                                       hash,
                                       { SizeKind_TextContent, 1.0f },
                                       { SizeKind_TextContent, 1.0f });

        // Enhanced button appearance - theme aware
        float d = s_UIState->DPIScale;
        widget->style_vars[StyleVar_Padding]      = Vec4(8.0f * d, 5.0f * d, 0.0f, 0.0f);
        widget->style_vars[StyleVar_CornerRadius] = Vec4(5.0f * d, 0.0f, 0.0f, 0.0f);
        widget->TextAlignment = UI_Text_Alignment_Center_X | UI_Text_Alignment_Center_Y;

        if(s_UIState->CurrentTheme == UITheme_Dark)
        {
            widget->style_vars[StyleVar_BackgroundColor]       = Vec4(0.25f, 0.25f, 0.25f, 1.0f);
            widget->style_vars[StyleVar_HotBackgroundColor]    = Vec4(0.30f, 0.30f, 0.30f, 1.0f);
            widget->style_vars[StyleVar_ActiveBackgroundColor] = Vec4(0.35f, 0.35f, 0.35f, 1.0f);
            widget->style_vars[StyleVar_BorderColor]           = Vec4(0.45f, 0.45f, 0.45f, 1.0f);
            widget->style_vars[StyleVar_HotBorderColor]        = Vec4(0.5f, 0.65f, 0.9f, 1.0f);
            widget->style_vars[StyleVar_ActiveBorderColor]     = Vec4(0.4f, 0.55f, 0.85f, 1.0f);
        }
        else
        {
            widget->style_vars[StyleVar_BackgroundColor]       = Vec4(0.85f, 0.85f, 0.85f, 1.0f);
            widget->style_vars[StyleVar_HotBackgroundColor]    = Vec4(0.75f, 0.75f, 0.75f, 1.0f);
            widget->style_vars[StyleVar_ActiveBackgroundColor] = Vec4(0.65f, 0.65f, 0.65f, 1.0f);
            widget->style_vars[StyleVar_BorderColor]           = Vec4(0.6f, 0.6f, 0.6f, 1.0f);
            widget->style_vars[StyleVar_HotBorderColor]        = Vec4(0.5f, 0.65f, 0.9f, 1.0f);
            widget->style_vars[StyleVar_ActiveBorderColor]     = Vec4(0.4f, 0.55f, 0.85f, 1.0f);
        }

        return HandleWidgetInteraction(widget);
    }

    UI_Interaction UIImage(const char* str,
                           Graphics::Texture2D* texture,
                           Vec2 scale)
    {
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        UI_Widget* widget = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBorder | WidgetFlags_DrawBackground,
                                       text,
                                       hash,
                                       { SizeKind_Pixels, texture->GetWidth() * scale.x },
                                       { SizeKind_Pixels, texture->GetHeight() * scale.y });
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
            parent->style_vars[StyleVar_CornerRadius] = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);
            
            if(s_UIState->CurrentTheme == UITheme_Dark)
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
 
            UI_Widget* slider                    = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBorder | WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Draggable | WidgetFlags_AnimateScale,
                                                              text,
                                                              hash,
                                                              { SizeKind_PercentOfParent, handleSizeFraction },
                                                              { SizeKind_PercentOfParent, 1.0f });
            
            // Enhanced slider handle appearance - theme aware
            slider->style_vars[StyleVar_Border]       = Vec4(1.0f * dpi, 1.0f * dpi, 0.0f, 0.0f);
            slider->style_vars[StyleVar_Padding]      = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
            slider->style_vars[StyleVar_CornerRadius] = Vec4(5.0f * dpi, 0.0f, 0.0f, 0.0f);
            
            if(s_UIState->CurrentTheme == UITheme_Dark)
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
            Vec2 mouse = Input::Get().GetMousePosition() * s_UIState->DPIScale - s_UIState->InputOffset;

            // If clicked on the parent track, set value based on mouse position
            if(parent_interaction.clicked)
            {
                f32 t  = (mouse.x - parent_x - slider_size_x * 0.5f) / (parent_size_x - slider_size_x);
                t      = Maths::Clamp(t, 0.0f, 1.0f);
                *value = min_value + (max_value - min_value) * t;
            }

            // Compute t from value, unless dragging, in which case use mouse pos
            f32 t = (*value - min_value) / (max_value - min_value);
            if(slider->dragging)
            {
                Vec2 dragMouse = Input::Get().GetMousePosition() * s_UIState->DPIScale - s_UIState->InputOffset;
                t              = (dragMouse.x - parent_x - slider_size_x * 0.5f) / (parent_size_x - slider_size_x);
            }

            // Final clamp and apply
            t      = Maths::Clamp(t, 0.0f, 1.0f);
            *value = Maths::Clamp(min_value + (max_value - min_value) * t, min_value, max_value);

            // Place the slider so it stays within the parent (relative position)
            slider->relative_position[UIAxis_X] = t * (parent_size_x - slider_size_x);

            // Render text showing the value
            String8 slider_text  = PushStr8F(s_UIState->UIFrameArena, "  %s: %.2f", (const char*)text.str, *value);
            String8 slider_label = HandleUIString((char*)slider_text.str, &hash);
            UI_Widget* widget    = PushWidget(WidgetFlags_DrawText | WidgetFlags_CentreY,
                                              slider_label,
                                              hash,
                                              { SizeKind_TextContent, 1.0f },
                                              { SizeKind_TextContent, 1.0f });
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

        UI_Widget* spacer = PushWidget(WidgetFlags_StackHorizontally,
                                       SpacerText2,
                                       hashSpacer,
                                       { SizeKind_ChildSum, 1.0f },
                                       { SizeKind_ChildSum, 1.0f });
        PushParent(spacer);
        {
            float fontSize   = spacer->style_vars[StyleVar_FontSize].x;
            float textHeight = GetStringSize(Str8Lit("A"), fontSize).y;
            float trackWidth = textHeight * 1.8f;
            float trackHeight = textHeight;
            float knobSize   = textHeight - 4.0f;

            // Toggle track (background pill)
            UI_Widget* toggle_track = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Clickable | WidgetFlags_CentreY | WidgetFlags_IsToggle,
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
            float speed = s_UIState->AnimationRateDT * 1.5f;
            if(toggle_track->ToggleTransition < target)
                toggle_track->ToggleTransition = Maths::Min(toggle_track->ToggleTransition + speed, target);
            else if(toggle_track->ToggleTransition > target)
                toggle_track->ToggleTransition = Maths::Max(toggle_track->ToggleTransition - speed, target);

            float t = toggle_track->ToggleTransition;

            // Smooth cubic easing for color
            float easedT = t * t * (3.0f - 2.0f * t);

            // Interpolate track colors based on animation
            Vec4 offColor, onColor;
            if(s_UIState->CurrentTheme == UITheme_Dark)
            {
                offColor = Vec4(0.3f, 0.3f, 0.3f, 1.0f);
                onColor  = Vec4(0.2f, 0.5f, 0.9f, 1.0f);
            }
            else
            {
                offColor = Vec4(0.75f, 0.75f, 0.75f, 1.0f);
                onColor  = Vec4(0.3f, 0.6f, 0.95f, 1.0f);
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

            // Knob position: animate from left to right
            float knobPadding = 2.0f;
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

            // Label
            String8 labelText = PushStr8F(s_UIState->UIFrameArena, "label###label%s", (char*)text.str);
            UI_Widget* label  = PushWidget(WidgetFlags_DrawText | WidgetFlags_CentreY,
                                           text,
                                           HashUIStr8Name(labelText),
                                           { SizeKind_TextContent, 1.0f },
                                           { SizeKind_TextContent, 1.0f });
        }

        PopParent(spacer);

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
        row->style_vars[StyleVar_Padding] = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        PushParent(row);

        // Label
        String8 labelText = PushStr8F(s_UIState->UIFrameArena, "label###label%s", (char*)text.str);
        u64 hashLabel;
        HandleUIString((char*)labelText.str, &hashLabel);
        UI_Widget* label = PushWidget(WidgetFlags_DrawText | WidgetFlags_CentreY,
                                      text,
                                      hashLabel,
                                      { SizeKind_TextContent, 1.0f },
                                      { SizeKind_TextContent, 1.0f });

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

        if(s_UIState->CurrentTheme == UITheme_Dark)
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
        fill->style_vars[StyleVar_BackgroundColor] = Vec4(0.3f, 0.6f, 0.95f, 1.0f);

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
            parent->style_vars[StyleVar_CornerRadius] = Vec4(4.0f * dp, 0.0f, 0.0f, 0.0f);}

            if(s_UIState->CurrentTheme == UITheme_Dark)
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

            if(s_UIState->CurrentTheme == UITheme_Dark)
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

            Vec2 mouse = Input::Get().GetMousePosition() * s_UIState->DPIScale - s_UIState->InputOffset;

            if(parent_interaction.clicked)
            {
                f32 t  = (mouse.x - parent_x - slider_size_x * 0.5f) / (parent_size_x - slider_size_x);
                t      = Maths::Clamp(t, 0.0f, 1.0f);
                *value = min_value + (int)Maths::Round((float)(max_value - min_value) * t);
            }

            f32 t = (float)(*value - min_value) / (float)(max_value - min_value);
            if(slider->dragging)
            {
                Vec2 dragMouse = Input::Get().GetMousePosition() * s_UIState->DPIScale - s_UIState->InputOffset;
                t              = (dragMouse.x - parent_x - slider_size_x * 0.5f) / (parent_size_x - slider_size_x);
            }

            t      = Maths::Clamp(t, 0.0f, 1.0f);
            *value = Maths::Clamp(min_value + (int)Maths::Round((float)(max_value - min_value) * t), min_value, max_value);

            slider->relative_position[UIAxis_X] = t * (parent_size_x - slider_size_x);

            // Integer display label
            String8 slider_text  = PushStr8F(s_UIState->UIFrameArena, "  %s: %d", (const char*)text.str, *value);
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
        static u64 separatorCounter = 0;
        String8 sepText = PushStr8F(s_UIState->UIFrameArena, "separator###sep%llu", separatorCounter++);
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

        if(s_UIState->CurrentTheme == UITheme_Dark)
        {
            separator->style_vars[StyleVar_BackgroundColor] = Vec4(0.4f, 0.4f, 0.4f, 1.0f);
        }
        else
        {
            separator->style_vars[StyleVar_BackgroundColor] = Vec4(0.7f, 0.7f, 0.7f, 1.0f);
        }
    }

    void UISpacer(float size)
    {
        size *= s_UIState->DPIScale;
        static u64 spacerCounter = 0;
        String8 spacerText = PushStr8F(s_UIState->UIFrameArena, "spacer###spacer%llu", spacerCounter++);
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
        static u64 rowCounter = 0;
        String8 rowText = PushStr8F(s_UIState->UIFrameArena, "row###row%llu", rowCounter++);
        u64 hash;
        String8 text = HandleUIString((char*)rowText.str, &hash);

        UI_Widget* row = PushWidget(WidgetFlags_StackHorizontally,
                                    text,
                                    hash,
                                    { SizeKind_ChildSum, 1.0f },
                                    { SizeKind_MaxChild, 1.0f });

        row->style_vars[StyleVar_Padding] = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        PushParent(row);
    }

    void UIEndRow()
    {
        PopParent(GetCurrentParent());
    }

    void UIBeginColumn()
    {
        static u64 colCounter = 0;
        String8 colText = PushStr8F(s_UIState->UIFrameArena, "col###col%llu", colCounter++);
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
        row->style_vars[StyleVar_Padding] = Vec4(3.0f * s_UIState->DPIScale, 3.0f * s_UIState->DPIScale, 0.0f, 0.0f);

        UI_Interaction interaction = HandleWidgetInteraction(row);
        if(interaction.clicked)
        {
            *expanded = !(*expanded);
        }

        PushParent(row);

        // Arrow indicator
        String8 arrowText = *expanded ? Str8Lit("v") : Str8Lit(">");
        String8 arrowId   = PushStr8F(s_UIState->UIFrameArena, "arrow###arrow%s", (char*)text.str);
        u64 hashArrow;
        HandleUIString((char*)arrowId.str, &hashArrow);

        UI_Widget* arrow = PushWidget(WidgetFlags_DrawText | WidgetFlags_CentreY,
                                      arrowText,
                                      hashArrow,
                                      { SizeKind_TextContent, 1.0f },
                                      { SizeKind_TextContent, 1.0f });

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

    static UI_Widget* s_ScrollAreaWidget = nullptr;
    static float* s_ScrollAreaOffset     = nullptr;
    static float s_ScrollAreaHeight      = 0.0f;
    static float s_ScrollAreaContentHeight = 0.0f;

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

        if(s_UIState->CurrentTheme == UITheme_Dark)
        {
            area->style_vars[StyleVar_BackgroundColor] = Vec4(0.12f, 0.12f, 0.12f, 1.0f);
            area->style_vars[StyleVar_BorderColor]     = Vec4(0.3f, 0.3f, 0.3f, 1.0f);
        }
        else
        {
            area->style_vars[StyleVar_BackgroundColor] = Vec4(0.97f, 0.97f, 0.97f, 1.0f);
            area->style_vars[StyleVar_BorderColor]     = Vec4(0.7f, 0.7f, 0.7f, 1.0f);
        }

        s_ScrollAreaWidget = area;
        s_ScrollAreaOffset = scroll_offset;
        s_ScrollAreaHeight = height;

        PushParent(area);
    }

    void UIEndScrollArea()
    {
        if(!s_ScrollAreaWidget || !s_ScrollAreaOffset)
        {
            PopParent(s_ScrollAreaWidget);
            s_ScrollAreaWidget = nullptr;
            s_ScrollAreaOffset = nullptr;
            return;
        }

        // Calculate content height from children
        float contentHeight = 0.0f;
        for(UI_Widget* child = s_ScrollAreaWidget->first; child; child = child->next)
        {
            contentHeight += child->size.y;
        }
        s_ScrollAreaContentHeight = contentHeight;

        float visibleHeight = s_ScrollAreaHeight;
        float padding = s_ScrollAreaWidget->style_vars[StyleVar_Padding].x;
        visibleHeight -= padding * 2.0f;

        // Clamp scroll offset
        float maxScroll = Maths::Max(0.0f, contentHeight - visibleHeight);

        // Handle mouse wheel scrolling when hovering
        Vec2 mousePos  = Input::Get().GetMousePosition() - s_UIState->InputOffset;
        Vec2 areaMin   = s_ScrollAreaWidget->position;
        Vec2 areaMax   = areaMin + s_ScrollAreaWidget->size;
        bool hovering  = mousePos.x >= areaMin.x && mousePos.x <= areaMax.x &&
                         mousePos.y >= areaMin.y && mousePos.y <= areaMax.y;
        if(hovering)
        {
            float scrollDelta = Input::Get().GetScrollOffset() * 30.0f;
            *s_ScrollAreaOffset -= scrollDelta;
        }

        *s_ScrollAreaOffset = Maths::Clamp(*s_ScrollAreaOffset, 0.0f, maxScroll);

        // Only show scrollbar if content exceeds visible area
        if(contentHeight > visibleHeight)
        {
            float scrollbarWidth = 8.0f;
            float trackHeight = visibleHeight;
            float thumbRatio = visibleHeight / contentHeight;
            float thumbHeight = Maths::Max(20.0f, trackHeight * thumbRatio);
            float scrollRatio = (maxScroll > 0.0f) ? (*s_ScrollAreaOffset / maxScroll) : 0.0f;
            float thumbOffset = scrollRatio * (trackHeight - thumbHeight);

            // Scrollbar track
            static u64 trackCounter = 0;
            String8 trackId = PushStr8F(s_UIState->UIFrameArena, "scrolltrack###st%llu", trackCounter++);
            u64 trackHash;
            HandleUIString((char*)trackId.str, &trackHash);

            UI_Widget* track = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                          Str8Lit(""),
                                          trackHash,
                                          { SizeKind_Pixels, scrollbarWidth },
                                          { SizeKind_Pixels, trackHeight });

            track->relative_position = Vec2(s_ScrollAreaWidget->size.x - scrollbarWidth - 2.0f, padding);

            if(s_UIState->CurrentTheme == UITheme_Dark)
                track->style_vars[StyleVar_BackgroundColor] = Vec4(0.2f, 0.2f, 0.2f, 0.5f);
            else
                track->style_vars[StyleVar_BackgroundColor] = Vec4(0.85f, 0.85f, 0.85f, 0.5f);

            track->style_vars[StyleVar_CornerRadius] = Vec4(4.0f * s_UIState->DPIScale, 0.0f, 0.0f, 0.0f);

            // Scrollbar thumb
            String8 thumbId = PushStr8F(s_UIState->UIFrameArena, "scrollthumb###sth%llu", trackCounter);
            u64 thumbHash;
            HandleUIString((char*)thumbId.str, &thumbHash);

            UI_Widget* thumb = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | WidgetFlags_Clickable | WidgetFlags_Draggable,
                                          Str8Lit(""),
                                          thumbHash,
                                          { SizeKind_Pixels, scrollbarWidth - 2.0f },
                                          { SizeKind_Pixels, thumbHeight });

            thumb->relative_position = Vec2(s_ScrollAreaWidget->size.x - scrollbarWidth - 1.0f, padding + thumbOffset);
            thumb->drag_constraint_x = true; // Only allow vertical dragging

            if(s_UIState->CurrentTheme == UITheme_Dark)
                thumb->style_vars[StyleVar_BackgroundColor] = Vec4(0.5f, 0.5f, 0.5f, 0.8f);
            else
                thumb->style_vars[StyleVar_BackgroundColor] = Vec4(0.6f, 0.6f, 0.6f, 0.8f);

            thumb->style_vars[StyleVar_CornerRadius] = Vec4(3.0f * s_UIState->DPIScale, 0.0f, 0.0f, 0.0f);

            // Handle thumb dragging
            UI_Interaction thumbInteraction = HandleWidgetInteraction(thumb);
            if(thumbInteraction.dragging)
            {
                float dragRatio = thumbInteraction.drag_delta.y / (trackHeight - thumbHeight);
                *s_ScrollAreaOffset += dragRatio * maxScroll;
                *s_ScrollAreaOffset = Maths::Clamp(*s_ScrollAreaOffset, 0.0f, maxScroll);
            }
        }

        PopParent(s_ScrollAreaWidget);
        s_ScrollAreaWidget = nullptr;
        s_ScrollAreaOffset = nullptr;
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
        row->style_vars[StyleVar_Padding] = Vec4(0.0f, 0.0f, 0.0f, 0.0f);

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

        // Build display text with cursor if focused
        bool isFocused     = (s_UIState->FocusedTextInput == hash);
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
                                      { SizeKind_Pixels, 150.0f },
                                      { SizeKind_TextContent, 1.0f });

        {float dp = s_UIState->DPIScale;
        field->style_vars[StyleVar_CornerRadius] = Vec4(3.0f * dp, 0.0f, 0.0f, 0.0f);
        field->style_vars[StyleVar_Padding]      = Vec4(5.0f * dp, 3.0f * dp, 0.0f, 0.0f);}

        if(s_UIState->CurrentTheme == UITheme_Dark)
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
        s_UIState->FocusableWidgets.PushBack(hash);

        UI_Interaction interaction = HandleWidgetInteraction(field);

        // Handle click focus
        if(interaction.clicked)
        {
            s_UIState->FocusedTextInput    = hash;
            s_UIState->TextInputBuffer     = buffer;
            s_UIState->TextInputBufferSize = buffer_size;
            s_UIState->TextInputCursor     = (u32)strlen(buffer);
            s_UIState->TextInputSelStart   = 0;
            s_UIState->TextInputSelEnd     = 0;
            s_UIState->FocusIndex = (i32)(s_UIState->FocusableWidgets.Size() - 1);
        }

        // Handle Tab focus navigation
        if(s_UIState->TabPressed || s_UIState->ShiftTabPressed)
        {
            i32 numFocusable = (i32)s_UIState->FocusableWidgets.Size();
            if(numFocusable > 0)
            {
                if(s_UIState->TabPressed)
                {
                    s_UIState->FocusIndex = (s_UIState->FocusIndex + 1) % numFocusable;
                }
                else
                {
                    s_UIState->FocusIndex = (s_UIState->FocusIndex - 1 + numFocusable) % numFocusable;
                }

                // Focus this widget if it's the one selected
                u64 focusedHash = s_UIState->FocusableWidgets[s_UIState->FocusIndex];
                if(focusedHash == hash)
                {
                    s_UIState->FocusedTextInput    = hash;
                    s_UIState->TextInputBuffer     = buffer;
                    s_UIState->TextInputBufferSize = buffer_size;
                    s_UIState->TextInputCursor     = (u32)strlen(buffer);
                    s_UIState->TextInputSelStart   = 0;
                    s_UIState->TextInputSelEnd     = (u32)strlen(buffer); // Select all on focus
                }
            }
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
            if(Input::Get().GetKeyHeld(InputCode::Key::LeftShift) || Input::Get().GetKeyHeld(InputCode::Key::RightShift))
                s_UIState->ShiftTabPressed = true;
            else
                s_UIState->TabPressed = true;
            return;
        }

        if(s_UIState->FocusedTextInput == 0 || !s_UIState->TextInputBuffer)
            return;

        u32 len = (u32)strlen(s_UIState->TextInputBuffer);
        bool shift = Input::Get().GetKeyHeld(InputCode::Key::LeftShift) || Input::Get().GetKeyHeld(InputCode::Key::RightShift);
        bool ctrl = Input::Get().GetKeyHeld(InputCode::Key::LeftControl) || Input::Get().GetKeyHeld(InputCode::Key::RightControl);
#ifdef LUMOS_PLATFORM_MACOS
        ctrl = ctrl || Input::Get().GetKeyHeld(InputCode::Key::LeftSuper) || Input::Get().GetKeyHeld(InputCode::Key::RightSuper);
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
        row->style_vars[StyleVar_Padding] = Vec4(0.0f, 0.0f, 0.0f, 0.0f);

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
                                       { SizeKind_Pixels, 150.0f },
                                       { SizeKind_TextContent, 1.0f });

        {float dp = s_UIState->DPIScale;
        button->style_vars[StyleVar_CornerRadius] = Vec4(3.0f * dp, 0.0f, 0.0f, 0.0f);
        button->style_vars[StyleVar_Padding]      = Vec4(5.0f * dp, 3.0f * dp, 0.0f, 0.0f);}

        bool isOpen = (s_UIState->OpenDropdown == hash);

        if(s_UIState->CurrentTheme == UITheme_Dark)
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
                s_UIState->OpenDropdown = hash;
            
            interaction.clicked = false;
        }

        // Draw dropdown list if open
        if(isOpen)
        {
            String8 listId = PushStr8F(s_UIState->UIFrameArena, "list###droplist%s", (char*)text.str);
            u64 hashList;
            HandleUIString((char*)listId.str, &hashList);

            UI_Widget* list = PushWidget(WidgetFlags_StackVertically | WidgetFlags_DrawBackground | WidgetFlags_DrawBorder,
                                         Str8Lit(""),
                                         hashList,
                                         { SizeKind_Pixels, 150.0f },
                                         { SizeKind_ChildSum, 1.0f });

            {float dp = s_UIState->DPIScale;
            list->style_vars[StyleVar_CornerRadius] = Vec4(3.0f * dp, 0.0f, 0.0f, 0.0f);
            list->style_vars[StyleVar_Padding]      = Vec4(1.0f * dp, 1.0f * dp, 0.0f, 0.0f);}

            if(s_UIState->CurrentTheme == UITheme_Dark)
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
                                               { SizeKind_PercentOfParent, 1.0f },
                                               { SizeKind_TextContent, 1.0f });

                option->style_vars[StyleVar_Padding] = Vec4(4.0f * s_UIState->DPIScale, 2.0f * s_UIState->DPIScale, 0.0f, 0.0f);

                if(s_UIState->CurrentTheme == UITheme_Dark)
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
            s_UIState->TooltipPos = Input::Get().GetMousePosition() * s_UIState->DPIScale - s_UIState->InputOffset;
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

        UI_Widget* menu = PushWidget(WidgetFlags_StackVertically | WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                     text,
                                     hash,
                                     { SizeKind_MaxChild, 1.0f },
                                     { SizeKind_ChildSum, 1.0f });

        menu->relative_position = s_UIState->ContextMenuPos;
        {float dp = s_UIState->DPIScale;
        menu->style_vars[StyleVar_Padding] = Vec4(2.0f * dp, 2.0f * dp, 0.0f, 0.0f);
        menu->style_vars[StyleVar_CornerRadius] = Vec4(3.0f * dp, 0.0f, 0.0f, 0.0f);}

        if(s_UIState->CurrentTheme == UITheme_Dark)
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

        if(s_UIState->CurrentTheme == UITheme_Dark)
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
    static int s_ActiveTab = 0;
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

        tabBar->style_vars[StyleVar_Padding] = Vec4(0.0f, 0.0f, 0.0f, 0.0f);

        if(s_UIState->CurrentTheme == UITheme_Dark)
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
        bool isActive = (thisTabIndex == s_ActiveTab);

        UI_Widget* tab = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawText | WidgetFlags_DrawBackground | WidgetFlags_DrawBorder,
                                    text,
                                    hash,
                                    { SizeKind_TextContent, 1.0f },
                                    { SizeKind_TextContent, 1.0f });

        tab->style_vars[StyleVar_Padding] = Vec4(10.0f * s_UIState->DPIScale, 5.0f * s_UIState->DPIScale, 0.0f, 0.0f);
        tab->TextAlignment = UI_Text_Alignment_Center_X | UI_Text_Alignment_Center_Y;

        if(s_UIState->CurrentTheme == UITheme_Dark)
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
        if(interaction.clicked)
        {
            s_ActiveTab = thisTabIndex;
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
        s_ModalOverlay->style_vars[StyleVar_BackgroundColor] = Vec4(0.0f, 0.0f, 0.0f, 0.5f);

        // Close modal when clicking overlay
        UI_Interaction overlayInteraction = HandleWidgetInteraction(s_ModalOverlay);
        if(overlayInteraction.clicked)
        {
            *open = false;
            return false;
        }

        // Modal dialog box
        s_ModalWidget = PushWidget(WidgetFlags_StackVertically | WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | WidgetFlags_CentreX | WidgetFlags_CentreY,
                                   text,
                                   hash,
                                   { SizeKind_ChildSum, 1.0f },
                                   { SizeKind_ChildSum, 1.0f });

        // Center the modal
        Vec2 screenCenter = s_UIState->root_parent.size * 0.5f;
        s_ModalWidget->relative_position = screenCenter;

        {float dp = s_UIState->DPIScale;
        s_ModalWidget->style_vars[StyleVar_Padding] = Vec4(14.0f * dp, 14.0f * dp, 0.0f, 0.0f);
        s_ModalWidget->style_vars[StyleVar_CornerRadius] = Vec4(6.0f * dp, 0.0f, 0.0f, 0.0f);}

        if(s_UIState->CurrentTheme == UITheme_Dark)
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
            s_ModalWidget = nullptr;
            s_ModalOverlay = nullptr;
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
                                      { SizeKind_Pixels, 20.0f },
                                      { SizeKind_TextContent, 1.0f });

        arrow->style_vars[StyleVar_Padding] = Vec4(2.0f * dp, 2.0f * dp, 0.0f, 0.0f);
        arrow->TextAlignment = UI_Text_Alignment_Center_Y;

        if(s_UIState->CurrentTheme == UITheme_Dark)
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

        if(s_UIState->CurrentTheme == UITheme_Dark)
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

        // Row container
        String8 rowId = PushStr8F(s_UIState->UIFrameArena, "colorow###cr%llu", hash);
        u64 rowHash;
        HandleUIString((char*)rowId.str, &rowHash);

        UI_Widget* row = PushWidget(WidgetFlags_StackHorizontally,
                                    Str8Lit(""),
                                    rowHash,
                                    { SizeKind_PercentOfParent, 1.0f },
                                    { SizeKind_ChildSum, 1.0f });

        row->style_vars[StyleVar_Padding] = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        PushParent(row);

        // Label
        UI_Widget* label = PushWidget(WidgetFlags_DrawText | WidgetFlags_CentreY,
                                      text,
                                      hash,
                                      { SizeKind_TextContent, 1.0f },
                                      { SizeKind_TextContent, 1.0f });

        {float dp = s_UIState->DPIScale;
        label->style_vars[StyleVar_Padding] = Vec4(4.0f * dp, 2.0f * dp, 0.0f, 0.0f);}
        if(s_UIState->CurrentTheme == UITheme_Dark)
            label->style_vars[StyleVar_TextColor] = Vec4(0.9f, 0.9f, 0.9f, 1.0f);
        else
            label->style_vars[StyleVar_TextColor] = Vec4(0.1f, 0.1f, 0.1f, 1.0f);

        // Color preview box
        String8 previewId = PushStr8F(s_UIState->UIFrameArena, "colorpreview###cp%llu", hash);
        u64 previewHash;
        HandleUIString((char*)previewId.str, &previewHash);

        UI_Widget* preview = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_CentreY,
                                        Str8Lit(""),
                                        previewHash,
                                        { SizeKind_Pixels, 30.0f * s_UIState->DPIScale },
                                        { SizeKind_Pixels, 20.0f * s_UIState->DPIScale });

        preview->style_vars[StyleVar_BackgroundColor] = Vec4(rgb[0], rgb[1], rgb[2], 1.0f);
        preview->style_vars[StyleVar_BorderColor] = Vec4(0.5f, 0.5f, 0.5f, 1.0f);
        preview->style_vars[StyleVar_CornerRadius] = Vec4(3.0f * s_UIState->DPIScale, 0.0f, 0.0f, 0.0f);

        // R slider
        float rVal = rgb[0];
        String8 rId = PushStr8F(s_UIState->UIFrameArena, "R###r%llu", hash);
        PopParent(row);

        // Use individual sliders for R, G, B
        UISlider((char*)rId.str, &rgb[0], 0.0f, 1.0f, 80.0f, 16.0f, 0.05f);
        if(rgb[0] != rVal) changed = true;

        String8 gId = PushStr8F(s_UIState->UIFrameArena, "G###g%llu", hash);
        float gVal = rgb[1];
        UISlider((char*)gId.str, &rgb[1], 0.0f, 1.0f, 80.0f, 16.0f, 0.05f);
        if(rgb[1] != gVal) changed = true;

        String8 bId = PushStr8F(s_UIState->UIFrameArena, "B###b%llu", hash);
        float bVal = rgb[2];
        UISlider((char*)bId.str, &rgb[2], 0.0f, 1.0f, 80.0f, 16.0f, 0.05f);
        if(rgb[2] != bVal) changed = true;

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

    // Easing functions for smooth animations
    static float EaseOutCubic(float t)
    {
        return 1.0f - powf(1.0f - t, 3.0f);
    }

    static float EaseInOutCubic(float t)
    {
        return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) / 2.0f;
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

    UI_Widget* UIWidgetRecurseDepthFirstPostOrder(UI_Widget* Node)
    {
        UI_Widget* Next = 0;

        if(Node->last != 0)
        {
            Next = Node->last;
        }
        else
        {
            for(UI_Widget* P = Node; P != 0; P = P->parent)
            {
                if(P->prev != 0)
                {
                    Next = P->prev;
                    break;
                }
            }
        }

        return Next;
    }

    void UILayoutSolveStandaloneSizes(UI_Widget* Root, UIAxis Axis)
    {
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
                f32 value = Widget->semantic_size[Axis].value;
                // Add padding for both sides (left+right or top+bottom)
                Widget->size[Axis] = (text_size[Axis] + padding[Axis] * 2.0f) * value;
            }
            break;
            }
        }
    }

    void UILayoutSolveUpwardsSizes(UI_Widget* Root, UIAxis Axis)
    {
        for(UI_Widget* Widget = Root; Widget; Widget = UIWidgetRecurseDepthFirstPreOrder(Widget))
        {
            UI_Size* Size = &Widget->semantic_size[Axis];

            switch(Size->kind)
            {
            case SizeKind_PercentOfParent:
            {
                Widget->size[Axis] = Widget->parent->size[Axis] * Size->value;
            }
            break;
            }
        }
    }

    void UILayoutSolveDownwardsSizes(UI_Widget* Root, UIAxis Axis)
    {
        for(UI_Widget* Widget = Root; Widget; Widget = UIWidgetRecurseDepthFirstPostOrder(Widget))
        {
            UI_Size* Size = &Widget->semantic_size[Axis];

            switch(Size->kind)
            {
            case SizeKind_ChildSum:
            {
                f32 Sum = 0;
                for(UI_Widget* Child = Widget->first; Child; Child = Child->next)
                {
                    if(Child->semantic_size[Axis].kind == SizeKind_ChildSum)
                    {
                        UILayoutSolveDownwardsSizes(Child, Axis);
                    }

                    Vec2 padding = Child->style_vars[StyleVar_Padding].ToVector2();

                    WidgetFlags flag = (Axis == 0 ? WidgetFlags_StackHorizontally : WidgetFlags_StackVertically);
                    if(Widget->flags & flag) // Axis == Widget->LayoutingAxis)
                    {
                        Sum += Child->size[Axis] + padding[Axis] * 2.0f;
                    }
                    else
                    {
                        Sum = Maths::Max(Sum, Child->size[Axis] + padding[Axis] * 2.0f);
                    }
                }

                Widget->size[Axis] = Sum;
            }
            break;
            case SizeKind_MaxChild:
            {
                f32 Sum = 0;
                for(UI_Widget* Child = Widget->first; Child; Child = Child->next)
                {
                    if(Child->semantic_size[Axis].kind == SizeKind_ChildSum || Child->semantic_size[Axis].kind == SizeKind_MaxChild)
                    {
                        UILayoutSolveDownwardsSizes(Child, Axis);
                    }

                    {
                        Vec2 padding = Child->style_vars[StyleVar_Padding].ToVector2();
                        Sum          = Maths::Max(Sum, Child->size[Axis] + padding[Axis] * 2.0f);
                    }
                }

                Widget->size[Axis] = Sum;
            }
            break;
            }
        }
    }

    void UILayoutFinalisePositions(UI_Widget* Root, UIAxis Axis)
    {
        for(UI_Widget* Parent = Root; Parent != 0; Parent = UIWidgetRecurseDepthFirstPreOrder(Parent))
        {
            f32 LayoutPosition = 0;
            for(UI_Widget* Child = Parent->first; Child != 0; Child = Child->next)
            {
                if(Axis == UIAxis_X && !(Child->flags & WidgetFlags_Floating_X))
                {
                    float xOffset = 0;
                    if(Child->flags & WidgetFlags_CentreX)
                    {
                        xOffset = Parent->size[Axis] * 0.5f - Child->size[Axis] * 0.5f;
                    }

                    Child->relative_position[Axis] = LayoutPosition + xOffset;
                    LayoutPosition += (Parent->flags & WidgetFlags_StackHorizontally ? 1 : 0) * Child->size[Axis];
                }
                if(Axis == UIAxis_Y && !(Child->flags & WidgetFlags_Floating_Y))
                {
                    float yOffset = 0;
                    if(Child->flags & WidgetFlags_CentreY)
                    {
                        yOffset = Parent->size[Axis] * 0.5f - Child->size[Axis] * 0.5f;
                    }

                    Child->relative_position[Axis] = LayoutPosition + yOffset;
                    LayoutPosition += (Parent->flags & WidgetFlags_StackVertically ? 1 : 0) * Child->size[Axis];
                }

                Vec2 padding = Child->style_vars[StyleVar_Padding].ToVector2();

                if(Axis == UIAxis_X)
                {
                    f32 X = 0.0f;
                    Child->position.x = Parent->position.x + Child->relative_position[Axis] - X + padding.x;
                }
                else if(Axis == UIAxis_Y)
                {
                    f32 Y = 0.0f;
                    Child->position.y = Parent->position.y + Child->relative_position[Axis] - Y + padding.y;
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

            static u64 tooltipCounter = 0;
            String8 tipId             = PushStr8F(s_UIState->UIFrameArena, "tooltip###tip%llu", tooltipCounter++);
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

            if(s_UIState->CurrentTheme == UITheme_Dark)
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

    void RefreshUI()
    {
        ForHashMapEach(u64, UI_Widget*, &s_UIState->widgets, it)
        {
            u64 key          = *it.key;
            UI_Widget* value = *it.value;

            s_UIState->WidgetAllocator->Deallocate(value);
        }

        HashMapClear(&s_UIState->widgets);

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

        style_variable_lists[StyleVar_Padding].first->value               = { 10.0f, 7.0f, 0.0f, 0.0f };
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

        s_UIState->CurrentTheme = UITheme_Light;
    }
    
    void UIApplyDarkTheme()
    {
        Style_Variable_List* style_variable_lists = s_UIState->style_variable_lists;

        style_variable_lists[StyleVar_Padding].first->value               = { 10.0f, 7.0f, 0.0f, 0.0f };
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

        s_UIState->CurrentTheme = UITheme_Dark;
    }
    
    void UIApplyBlueTheme()
    {
        Style_Variable_List* style_variable_lists = s_UIState->style_variable_lists;

        style_variable_lists[StyleVar_Padding].first->value               = { 10.0f, 7.0f, 0.0f, 0.0f };
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

        s_UIState->CurrentTheme = UITheme_Blue;
    }

    void UIApplyGreenTheme()
    {
        Style_Variable_List* style_variable_lists = s_UIState->style_variable_lists;

        style_variable_lists[StyleVar_Padding].first->value               = { 10.0f, 7.0f, 0.0f, 0.0f };
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

        s_UIState->CurrentTheme = UITheme_Green;
    }

    void UIApplyPurpleTheme()
    {
        Style_Variable_List* style_variable_lists = s_UIState->style_variable_lists;

        style_variable_lists[StyleVar_Padding].first->value               = { 10.0f, 7.0f, 0.0f, 0.0f };
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

        // Re-apply DPI scaling after theme reset
        float dpi = s_UIState->DPIScale;
        if(dpi > 1.0f)
        {
            auto* styles = s_UIState->style_variable_lists;
            styles[StyleVar_FontSize].first->value.x *= dpi;
            styles[StyleVar_Padding].first->value.x *= dpi;
            styles[StyleVar_Padding].first->value.y *= dpi;
            styles[StyleVar_Border].first->value.x *= dpi;
            styles[StyleVar_Border].first->value.y *= dpi;
            styles[StyleVar_CornerRadius].first->value.x *= dpi;
            styles[StyleVar_ShadowOffset].first->value.y *= dpi;
            styles[StyleVar_ShadowBlur].first->value.x *= dpi;
        }
    }
}
