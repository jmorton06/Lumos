#pragma once
#include "Core/Core.h"
#include "Core/String.h"
#include "Core/DataStructures/Map.h"
#include "Core/OS/Allocators/PoolAllocator.h"
#include "Core/DataStructures/TDArray.h"
#include "Core/OS/KeyCodes.h"

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
        WidgetFlags_DragParent        = (1 << 11),
        WidgetFlags_AnimateScale      = (1 << 12), // Scale down slightly when pressed
        WidgetFlags_IsToggle          = (1 << 13)  // For animated toggle switches
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
        StyleVar_CornerRadius,
        StyleVar_ShadowColor,
        StyleVar_ShadowOffset,
        StyleVar_ShadowBlur,
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
        u32 TextAlignment = 0;

        Vec2 cursor;
        Vec2 position;
        Vec2 relative_position;
        Vec2 size;

        // Temp
        bool clicked;
        bool right_clicked;
        bool double_clicked;

        bool is_initial_dragging_position_set;
        bool dragging;
        bool drag_constraint_x;
        bool drag_constraint_y;
        Vec2 drag_offset;
        Vec2 drag_mouse_p;
        
        f32 HotTransition;
        f32 ActiveTransition;
        f32 ToggleTransition; // 0 = off, 1 = on (for animated toggles)
        f32 ScaleAnimation;   // For press scale effect

        u64 LastFrameIndexActive;
    };

    struct UI_Interaction
    {
        UI_Widget* widget;
        bool hovering;
        bool clicked;
        bool right_clicked;
        bool double_clicked;
        bool pressed;
        bool released;
        bool dragging;
        Vec2 drag_delta;
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

    enum UITheme
    {
        UITheme_Light,
        UITheme_Dark,
        UITheme_Blue,
        UITheme_Green,
        UITheme_Purple,
        UITheme_HighContrast,
        UITheme_Count
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
        
        UITheme CurrentTheme = UITheme_Light;

        // Double-click tracking
        f32 LastClickTime     = 0.0f;
        u64 LastClickedWidget = 0;
        f32 DoubleClickTime   = 0.3f; // seconds
        f32 CurrentTime       = 0.0f;

        // Text input state
        u64 FocusedTextInput    = 0;
        char* TextInputBuffer   = nullptr;
        u32 TextInputBufferSize = 0;
        u32 TextInputCursor     = 0;
        u32 TextInputSelStart   = 0; // Selection start
        u32 TextInputSelEnd     = 0; // Selection end
        bool TextInputShiftHeld = false;

        // Focus navigation
        TDArray<u64> FocusableWidgets;
        i32 FocusIndex = -1;
        bool TabPressed = false;
        bool ShiftTabPressed = false;

        // Tree view indent level
        i32 TreeIndentLevel = 0;

        // Tooltip state
        u64 HoveredWidget       = 0;
        f32 HoverStartTime      = 0.0f;
        f32 TooltipDelay        = 0.5f; // seconds before showing
        String8 TooltipText     = {};
        bool ShowTooltip        = false;
        Vec2 TooltipPos         = {};

        // Dropdown state
        u64 OpenDropdown = 0;

        // Context menu state
        bool ContextMenuOpen    = false;
        Vec2 ContextMenuPos     = {};
        u64 ContextMenuTrigger  = 0; // Widget that triggered the menu
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
                             float max_value = 1.0f,
                             float width = 250.0f,
                             float height = 20.0f,
                             float handleSizeFraction = 0.1f);

    UI_Interaction UIToggle(const char* str,
                             bool* value);

    // Progress bar (0.0 to 1.0)
    UI_Interaction UIProgressBar(const char* str,
                                  float progress,
                                  float width = 200.0f,
                                  float height = 20.0f);

    // Integer slider
    UI_Interaction UISliderInt(const char* str,
                                int* value,
                                int min_value = 0,
                                int max_value = 100,
                                float width = 250.0f,
                                float height = 20.0f);

    // Horizontal separator line
    void UISeparator(float width = 0.0f);

    // Add vertical spacing
    void UISpacer(float height = 10.0f);

    // Horizontal layout container
    void UIBeginRow();
    void UIEndRow();

    // Vertical layout container
    void UIBeginColumn();
    void UIEndColumn();

    // Expander (collapsible section header)
    UI_Interaction UIExpander(const char* str, bool* expanded);

    // Scroll area
    void UIBeginScrollArea(const char* str, float height, float* scroll_offset);
    void UIEndScrollArea();

    // Text input
    UI_Interaction UITextInput(const char* str, char* buffer, u32 buffer_size, u32* cursor_pos = nullptr);

    // Dropdown/ComboBox
    UI_Interaction UIDropdown(const char* str, int* selected_index, const char** options, int option_count);

    // Tooltip - call after the widget you want to add tooltip to
    void UITooltip(const char* text);

    // Context menu - right-click popup
    bool UIBeginContextMenu(const char* str);
    void UIEndContextMenu();
    UI_Interaction UIContextMenuItem(const char* str);

    // Tab container
    bool UIBeginTabBar(const char* str);
    void UIEndTabBar();
    bool UITabItem(const char* str, bool* open = nullptr);

    // Modal dialog
    bool UIBeginModal(const char* str, bool* open);
    void UIEndModal();

    // Tree view
    bool UITreeNode(const char* str, bool* expanded = nullptr);
    void UITreePop();

    // Color picker
    bool UIColorEdit3(const char* str, float* rgb); // RGB 0-1
    bool UIColorEdit4(const char* str, float* rgba); // RGBA 0-1

    // Process text input (call from app event handler)
    void UIProcessKeyTyped(char character);
    void UIProcessKeyPressed(InputCode::Key key);

    // Focus navigation
    void UISetFocusNext(); // Focus next focusable widget
    void UISetFocusPrev(); // Focus previous focusable widget

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
    
    // Theme management
    void UISetTheme(UITheme theme);
    const char* UIGetThemeName(UITheme theme);
    void UIApplyLightTheme();
    void UIApplyDarkTheme();
    void UIApplyBlueTheme();
    void UIApplyGreenTheme();
    void UIApplyPurpleTheme();
    void UIApplyHighContrastTheme();
}
