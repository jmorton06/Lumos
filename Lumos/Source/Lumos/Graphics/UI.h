#pragma once
#include "Core/Core.h"
#include "Core/String.h"
#include "Core/DataStructures/Map.h"
#include "Core/OS/Allocators/PoolAllocator.h"
#include "Core/DataStructures/TDArray.h"

#include "Maths/Vector3.h"
#include "Maths/Vector4.h"

namespace Lumos
{
    namespace Graphics
    {
        class Font;
        class Texture2D;
    }
    struct Arena;

    enum WidgetFlags : u32
    {
        WidgetFlags_Clickable         = (1 << 0),
        WidgetFlags_DrawText          = (1 << 1),
        WidgetFlags_DrawBorder        = (1 << 2),
        WidgetFlags_DrawBackground    = (1 << 3),
        WidgetFlags_Draggable         = (1 << 4),
        WidgetFlags_StackVertically   = (1 << 5),
        WidgetFlags_StackHorizontally = (1 << 6),
        WidgetFlags_Floating_X        = (1 << 7),
        WidgetFlags_Floating_Y        = (1 << 8),
        WidgetFlags_CentreX           = (1 << 9),
		WidgetFlags_CentreY           = (1 << 10),
        WidgetFlags_DragParent       = (1 << 11)
    };

    enum UITextAlignment : u32
    {
        UI_Text_Alignment_None     = (1 << 0),
        UI_Text_Alignment_Center_X = (1 << 1),
        UI_Text_Alignment_Center_Y = (1 << 2),
    };

    enum SizeKind
    {
        SizeKind_Pixels,
        SizeKind_TextContent,
        SizeKind_PercentOfParent,
        SizeKind_ChildSum,
        SizeKind_MaxChild
    };

    enum UIAxis
    {
        UIAxis_X,
        UIAxis_Y,
        UIAxis_Count
    };

    struct UI_Size
    {
        SizeKind kind;
        f32 value;
    };

    enum StyleVar : u8
    {
        StyleVar_Padding,
        StyleVar_Border,
        StyleVar_BorderColor,
        StyleVar_BackgroundColor,
        StyleVar_TextColor,
        StyleVar_HotBorderColor,
        StyleVar_HotBackgroundColor,
        StyleVar_HotTextColor,
        StyleVar_ActiveBorderColor,
        StyleVar_ActiveBackgroundColor,
        StyleVar_ActiveTextColor,
        StyleVar_FontSize,
        StyleVar_Count
    };

    struct UI_Widget
    {
        UI_Widget* parent;
        UI_Widget* first;
        UI_Widget* last;
        UI_Widget* next;
        UI_Widget* prev;

        Vec4 style_vars[StyleVar_Count];

        u64 hash;
        u32 flags;

        String8 text;
        Graphics::Texture2D* texture;
        UI_Size semantic_size[UIAxis_Count];
        UIAxis LayoutingAxis = UIAxis_Y;
        UITextAlignment TextAlignment;

        Vec2 cursor;
        Vec2 position;
        Vec2 relative_position;
        Vec2 size;

        // Temp
        bool clicked;

        bool is_initial_dragging_position_set;
        bool dragging;
        bool drag_constraint_x;
        bool drag_constraint_y;
        Vec2 drag_offset;
        Vec2 drag_mouse_p;
        
        f32 HotTransition;
        f32 ActiveTransition;

        u64 LastFrameIndexActive;
    };

    struct UI_Interaction
    {
        UI_Widget* widget;
        bool hovering;
        bool clicked;
        bool dragging;
    };

    struct Style_Variable
    {
        Vec4 value;
        Style_Variable* next;
        Style_Variable* prev;
    };

    struct Style_Variable_List
    {
        u32 count;

        Style_Variable* first;
        Style_Variable* last;

        Style_Variable* first_free;
    };

    struct UI_State
    {
        Arena* UIArena;
        Arena* UIFrameArena;

        PoolAllocator<UI_Widget>* WidgetAllocator;

        UI_Widget root_parent;
        TDArray<UI_Widget*> parents;
        HashMap(u64, UI_Widget*) widgets;

        Style_Variable_List style_variable_lists[StyleVar_Count];

        UI_Widget* active_widget_state;
        u64 next_hot_widget;
        u64 hot_widget;
        u64 active_widget;

        Vec2 InputOffset;
        f32 DPIScale = 1.0f;
        u64 FrameIndex;
        f32 AnimationRate = 10.0f;
        f32 AnimationRateDT = 10.0f;
    };

    UI_State* GetUIState();

    Vec2 GetStringSize(String8 text, float size = 32.0f);

    bool InitialiseUI(Arena* arena);
    void ShutDownUI();

    void UIBeginFrame(const Vec2& frame_buffer_size, f32 dt, const Vec2& inputOffset);

    void UIEndFrame(Graphics::Font* font);

    UI_Interaction UIBeginPanel(const char* str);
    UI_Interaction UIBeginPanel(const char* str, u32 extraFlags = 0);
    UI_Interaction UIBeginPanel(const char* str, SizeKind sizeKindX, float xValue, SizeKind sizeKindY, float yValue, u32 extraFlags = 0);

    void UIEndPanel();

    void UIPushStyle(StyleVar style_variable, float value);
    void UIPushStyle(StyleVar style_variable, const Vec2& value);
    void UIPushStyle(StyleVar style_variable, const Vec3& value);
    void UIPushStyle(StyleVar style_variable, const Vec4& value);
    void UIPopStyle(StyleVar style_variable);

    UI_Interaction UILabelCStr(const char* str, const char* text);
    UI_Interaction UILabel(const char* str, const String8& text);
    UI_Interaction UIButton(const char* str);
    UI_Interaction UIImage(const char* str,
                            Graphics::Texture2D* texture,
                            Vec2 scale = { 1.0f, 1.0f });

    UI_Interaction UISlider(const char* str,
                             float* value,
                             float min_value = 0.0f,
                             float max_value = 1.0f);

    UI_Interaction UIToggle(const char* str,
                             bool* value);

    void UIBeginBuild();
    void UIEndBuild();
    void UILayoutRoot(UI_Widget* Root);
    void UILayout();
    void UIAnimate();

    // Layout
    UI_Widget* UIWidgetRecurseDepthFirstPreOrder(UI_Widget* Node);
    UI_Widget* UIWidgetRecurseDepthFirstPostOrder(UI_Widget* Node);

    void UILayoutSolveStandaloneSizes(UI_Widget* Root, UIAxis Axis);
    void UILayoutSolveUpwardsSizes(UI_Widget* Root, UIAxis Axis);
    void UILayoutSolveDownwardsSizes(UI_Widget* Root, UIAxis Axis);
    void UILayoutFinalisePositions(UI_Widget* Root, UIAxis Axis);
    float UIGetTextHeight(Graphics::Font* font, UI_Widget* widget);

    void RefreshUI();
    void DearIMGUIDebugPanel();
}
