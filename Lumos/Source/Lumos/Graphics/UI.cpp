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

        style_variable_lists[StyleVar_Padding].first->value               = { 10.0f, 10.0f, 0.0f, 0.0f };
        style_variable_lists[StyleVar_Border].first->value                = { 1.0f, 1.0f, 0.0f, 0.0f };
        style_variable_lists[StyleVar_BorderColor].first->value           = { 1.0f, 0.0f, 0.0f, 1.0f };
        style_variable_lists[StyleVar_BackgroundColor].first->value       = { 1.0f, 1.0f, 1.0f, 1.0f };
        style_variable_lists[StyleVar_TextColor].first->value             = { 0.0f, 0.0f, 0.0f, 1.0f };
        style_variable_lists[StyleVar_HotBorderColor].first->value        = { 0.9f, 0.0f, 0.0f, 0.8f };
        style_variable_lists[StyleVar_HotBackgroundColor].first->value    = { 0.9f, 0.9f, 0.9f, 1.0f };
        style_variable_lists[StyleVar_HotTextColor].first->value          = { 0.1f, 0.1f, 0.1f, 1.0f };
        style_variable_lists[StyleVar_ActiveBorderColor].first->value     = { 0.5f, 0.0f, 0.0f, 0.8f };
        style_variable_lists[StyleVar_ActiveBackgroundColor].first->value = { 0.7f, 0.7f, 0.7f, 1.0f };
        style_variable_lists[StyleVar_ActiveTextColor].first->value       = { 0.5f, 0.0f, 0.0f, 1.0f };
        style_variable_lists[StyleVar_FontSize].first->value              = { 28.0f, 0.0f, 0.0f, 1.0f };

#ifdef LUMOS_PLATFORM_MACOS
        style_variable_lists[StyleVar_FontSize].first->value = { 48.0f, 0.0f, 0.0f, 1.0f };
#endif

        return true;
    }

    void ShutDownUI()
    {
        ArenaRelease(s_UIState->UIFrameArena);
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
        ASSERT(style_variable < StyleVar_Count);

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
        ASSERT(style_variable < StyleVar_Count);
        Style_Variable_List* list = &s_UIState->style_variable_lists[style_variable];
        ASSERT(list->count);
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
        ASSERT(!s_UIState->parents.Empty());
        ASSERT(s_UIState->parents.Back() == widget);
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
            void* mem = s_UIState->WidgetAllocator->Allocate();
            widget    = new(mem) UI_Widget();
            HashMapInsert(&s_UIState->widgets, hash, widget);
            LINFO("Creating Widget");
        }

        widget->parent               = parent;
        widget->flags                = flags;
        widget->text                 = text;
        widget->hash                 = hash;
        widget->texture              = nullptr;
        widget->LastFrameIndexActive = s_UIState->FrameIndex;

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
        ASSERT(widget);

        UI_Interaction interaction = {};
        interaction.widget         = widget;

        const Vec2& position = widget->position;
        const Vec2& size     = widget->size;
        const Vec2& mouse    = Input::Get().GetMousePosition() * s_UIState->DPIScale - s_UIState->InputOffset;

        bool hovering = mouse.x >= position.x && mouse.x <= position.x + size.x && mouse.y >= position.y && mouse.y <= position.y + size.y;
        if(hovering)
        {
            s_UIState->next_hot_widget = widget->hash;
            interaction.hovering       = true;
        }

        interaction.clicked  = widget->clicked;
        interaction.dragging = widget->dragging;

        return interaction;
    }

    void UIBeginFrame(const Vec2& frame_buffer_size, const Vec2& inputOffset)
    {
        LUMOS_PROFILE_FUNCTION();
        ArenaClear(s_UIState->UIFrameArena);

        s_UIState->InputOffset     = inputOffset;
        s_UIState->next_hot_widget = 0;
        s_UIState->parents.Clear();
        s_UIState->FrameIndex++;

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
            ASSERT(list->count == 0);
        }

        PopParent(&s_UIState->root_parent);
        ASSERT(s_UIState->parents.Empty());

        Input* input = &Input::Get();
        Vec2 mouse_p = input->GetMousePosition() * s_UIState->DPIScale - s_UIState->InputOffset;

        if(!s_UIState->root_parent.first)
        {
            return;
        }

        if(s_UIState->active_widget)
        {
            if(Input::Get().GetMouseHeld(Lumos::InputCode::MouseKey::ButtonLeft)) // is_button_held(input, MC_MOUSE_BUTTON_LEFT))
            {
                s_UIState->active_widget_state->clicked = false;

                if((s_UIState->active_widget_state->flags & WidgetFlags_Draggable))
                {
                    if(!s_UIState->active_widget_state->dragging)
                    {
                        s_UIState->active_widget_state->dragging     = true;
                        s_UIState->active_widget_state->drag_mouse_p = mouse_p;
                        s_UIState->active_widget_state->drag_offset  = mouse_p - s_UIState->active_widget_state->relative_position;
                    }
                    else
                    {
                        UI_Widget* widget = s_UIState->active_widget_state;
                        UI_Widget* parent = widget->parent;

                        Vec2 min_p = s_UIState->active_widget_state->drag_offset;
                        Vec2 max_p = parent->size - (widget->size - s_UIState->active_widget_state->drag_offset);

                        if(s_UIState->active_widget_state->drag_constraint_x)
                        {
                            mouse_p.x = s_UIState->active_widget_state->drag_mouse_p.x;
                        }

                        if(s_UIState->active_widget_state->drag_constraint_y)
                        {
                            mouse_p.y = s_UIState->active_widget_state->drag_mouse_p.y;
                        }

                        mouse_p = Maths::Clamp(mouse_p, min_p, max_p);
                        Application::Get().GetWindow()->SetMousePosition(mouse_p);
                        s_UIState->active_widget_state->relative_position = (mouse_p - s_UIState->active_widget_state->drag_offset);
                    }
                }
            }

            if(!Input::Get().GetMouseClicked(Lumos::InputCode::MouseKey::ButtonLeft)) // is_button_released(input, 0)) //Mouse_LEFT
            {
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
                if(Input::Get().GetMouseClicked(Lumos::InputCode::MouseKey::ButtonLeft)) // is_button_pressed(input, MC_MOUSE_BUTTON_LEFT))
                {
                    s_UIState->active_widget = s_UIState->hot_widget;
                    s_UIState->hot_widget    = 0;

                    HashMapFind(&s_UIState->widgets, s_UIState->active_widget, &s_UIState->active_widget_state);

                    if(s_UIState->active_widget_state)
                        s_UIState->active_widget_state->clicked = true;
                }
            }
        }

        TDArray<UI_Widget*> lWidgetsToDelete(s_UIState->UIFrameArena);
        ForHashMapEach(u64, UI_Widget*, &s_UIState->widgets, it)
        {
            u64 key          = *it.key;
            UI_Widget* value = *it.value;

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

    UI_Interaction UIBeginPanel(const char* str, WidgetFlags extraFlags)
    {
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        String8 WindowText = PushStr8F(s_UIState->UIFrameArena, "Window###window%s", (char*)text.str);
        u64 hashWindow;
        String8 WindowText2 = HandleUIString((char*)WindowText.str, &hashWindow);
        UI_Widget* window   = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_StackVertically | extraFlags,
                                         WindowText2,
                                         hashWindow,
                                         { SizeKind_MaxChild, 1.0f },
                                         { SizeKind_ChildSum, 1.0f });
        PushParent(window);

        String8 HeaderText = PushStr8F(s_UIState->UIFrameArena, "Header###header%s", (char*)text.str);
        u64 hashHeader;
        String8 HeaderText2 = HandleUIString((char*)HeaderText.str, &hashHeader);
        UIPushStyle(StyleVar_Padding, { 0.0f, 0.0f, 0.0f, 0.0f });
        UI_Widget* header = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_StackVertically,
                                       HeaderText2,
                                       hashHeader,
                                       { SizeKind_PercentOfParent, 1.0f },
                                       { SizeKind_ChildSum, 1.0f });
        UIPopStyle(StyleVar_Padding);
        PushParent(header);

        UI_Widget* title = PushWidget(WidgetFlags_DrawText,
                                      text,
                                      hash,
                                      { SizeKind_TextContent, 1.0f },
                                      { SizeKind_TextContent, 1.0f });

        PopParent(header);
        return HandleWidgetInteraction(header);
    }

    void UIEndPanel()
    {
        PopParent(GetCurrentParent());
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

        return HandleWidgetInteraction(widget);
    }

    UI_Interaction UIButton(const char* str)
    {
        u64 hash;
        String8 text = HandleUIString(str, &hash);

        UI_Widget* widget = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawText | WidgetFlags_DrawBorder | WidgetFlags_DrawBackground,
                                       text,
                                       hash,
                                       { SizeKind_TextContent, 1.0f },
                                       { SizeKind_TextContent, 1.0f });

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
                            float max_value)
    {
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
            String8 parentText = PushStr8F(s_UIState->UIFrameArena, "parent###parent%s", (char*)text.str);
            UI_Widget* parent  = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBorder | WidgetFlags_DrawBackground,
                                            text,
                                            HashUIStr8Name(parentText),
                                            { SizeKind_Pixels, 250.0f },
                                            { SizeKind_Pixels, 20.0f });

            UI_Interaction parent_interaction = HandleWidgetInteraction(parent);
            PushParent(parent);

            UI_Widget* slider = PushWidget(WidgetFlags_Clickable | WidgetFlags_Draggable | WidgetFlags_DrawBorder | WidgetFlags_DrawBackground,
                                           text,
                                           hash,
                                           { SizeKind_PercentOfParent, 0.1f },
                                           { SizeKind_PercentOfParent, 1.0f });

            slider->style_vars[StyleVar_Border]  = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
            slider->style_vars[StyleVar_Padding] = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
            slider_interaction                   = HandleWidgetInteraction(slider);

            PopParent(parent);

            slider->drag_constraint_y = true;

            *value         = Maths::Clamp(*value, min_value, max_value);
            Vec2& slider_p = slider->relative_position;

            f32 slider_x      = slider->position.x;
            f32 parent_x      = parent->position.x;
            f32 slider_size_x = slider->size.x;
            f32 parent_size_x = parent->size.x;

            if(parent_interaction.clicked)
            {
                const Vec2& mouse = Input::Get().GetMousePosition() * s_UIState->DPIScale - s_UIState->InputOffset; // s_UIState->input->mouse_position;
                f32 t             = (mouse.x - parent_x) / (parent_size_x - slider_size_x);
                *value            = min_value + (max_value - min_value) * t;
            }

            if(slider->dragging)
            {
                f32 t  = (slider_x - parent_x) / (parent_size_x - slider_size_x);
                *value = min_value + (max_value - min_value) * t;
            }
            else
            {
                f32 t      = (*value - min_value) / (max_value - min_value);
                slider_p.x = t * (parent_size_x - slider_size_x);
            }

            String8 slider_text = PushStr8F(s_UIState->UIFrameArena, "%.2f - %s", *value, (const char*)text.str);
            String8 text        = HandleUIString((char*)slider_text.str, &hash);

            UI_Widget* widget = PushWidget(WidgetFlags_DrawText,
                                           text,
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
            float fontSize  = spacer->style_vars[StyleVar_FontSize].x;
            float text_size = GetStringSize(Str8Lit("A"), fontSize).y;

            UI_Widget* toggle_box = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_Clickable,
                                               text,
                                               hash,
                                               { SizeKind_Pixels, text_size },
                                               { SizeKind_Pixels, text_size });

            interaction = HandleWidgetInteraction(toggle_box);
            if(interaction.clicked)
            {
                *value = !(*value);
            }

            if(*value)
            {
                toggle_box->style_vars[StyleVar_BackgroundColor]       = { 0.0f, 0.0f, 0.0f, 1.0f };
                toggle_box->style_vars[StyleVar_HotBackgroundColor]    = { 0.0f, 0.0f, 0.0f, 1.0f };
                toggle_box->style_vars[StyleVar_ActiveBackgroundColor] = { 0.0f, 0.0f, 0.0f, 1.0f };
            }
            else
            {
                toggle_box->style_vars[StyleVar_BackgroundColor]       = { 1.0f, 1.0f, 1.0f, 1.0f };
                toggle_box->style_vars[StyleVar_HotBackgroundColor]    = { 1.0f, 1.0f, 1.0f, 1.0f };
                toggle_box->style_vars[StyleVar_ActiveBackgroundColor] = { 1.0f, 1.0f, 1.0f, 1.0f };
            }

            String8 labelText = PushStr8F(s_UIState->UIFrameArena, "label###label%s", (char*)text.str);
            UI_Widget* label  = PushWidget(WidgetFlags_DrawText,
                                           text,
                                           HashUIStr8Name(labelText),
                                           { SizeKind_TextContent, 1.0f },
                                           { SizeKind_TextContent, 1.0f });
        }

        PopParent(spacer);

        return interaction;
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
                if(Axis == UIAxis_X)
                {
                    // f32 FontWidth = FontGetTextWidth(&Maraton->FontProvider, Widget->Font, Widget->String);
                    // Widget->size[Axis] = FontWidth * Widget->FontScale + padding.x;

                    f32 value          = Widget->semantic_size[Axis].value;
                    Vec2 text_size     = GetStringSize(Widget->text, fontSize) + padding.x;
                    Widget->size[Axis] = text_size[Axis] * value;
                }
                else
                {
                    // f32 FontHeight = FontGetTextHeight(Widget->Font);
                    // Widget->size[Axis] = FontHeight + (padding.y / 2.0f);

                    f32 value          = Widget->semantic_size[Axis].value;
                    Vec2 text_size     = GetStringSize(Widget->text, fontSize) + (padding.y / 2.0f);
                    Widget->size[Axis] = text_size[Axis] * value;
                }
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
                    if(Child->flags & WidgetFlags_CentreY)
                    {
                        xOffset = Parent->size[Axis] * 0.5f - Child->size[Axis] * 0.5f;
                    }

                    Child->relative_position[Axis] = LayoutPosition + xOffset;

                    LayoutPosition += (Parent->flags & WidgetFlags_StackHorizontally ? 1 : 0) * (Child->size[Axis] + xOffset);
                }
                if(Axis == UIAxis_Y && !(Child->flags & WidgetFlags_Floating_Y))
                {
                    Child->relative_position[Axis] = LayoutPosition;
                    LayoutPosition += /*(Axis == Parent->LayoutingAxis)*/ (Parent->flags & WidgetFlags_StackVertically ? 1 : 0) * Child->size[Axis];
                }

                Vec2 padding = Child->style_vars[StyleVar_Padding].ToVector2();

                if(Axis == UIAxis_X)
                {
                    f32 X = 0.0f;
                    // if (Parent->flags & UI_WidgetFlag_ViewScroll)
                    // {
                    //     X = Parent->ViewOffset[UIAxis_X];
                    // }

                    Child->position.x = Parent->position.x + Child->relative_position[Axis] - X + padding.x;
                    // Child->position.Width = Child->size[Axis];
                }
                else if(Axis == UIAxis_Y)
                {
                    f32 Y = 0.0f;
                    // if (Parent->flags & UI_WidgetFlag_ViewScroll)
                    // {
                    //     Y = Parent->ViewOffset[UIAxis_Y];
                    // }

                    Child->position.y = Parent->position.y + Child->relative_position[Axis] - Y + padding.y;
                    // Child->position.Height = Child->size[Axis];
                }
            }
        }
    }

    void UIBeginBuild()
    {
    }

    void UIEndBuild()
    {
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
