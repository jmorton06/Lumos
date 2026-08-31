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
    struct SafeAreaInsets;

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
        WidgetFlags_IsToggle          = (1 << 13), // For animated toggle switches
        WidgetFlags_CentreChildrenX   = (1 << 14),
        WidgetFlags_CentreChildrenY   = (1 << 15),
        WidgetFlags_AnimateAppear     = (1 << 16), // Pop-in scale/fade when widget first appears
        WidgetFlags_AlignRight        = (1 << 17), // Position at the parent's right edge (X-axis, non-floating)
        WidgetFlags_AnimateExit       = (1 << 18),
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
        SizeKind_MaxChild,
        SizeKind_PercentOfViewport,  // value = fraction of root (framebuffer minus safe area), 0..1
        SizeKind_Grow,
    };

    // Panel anchor positions. Used by UIWindowAnchor. None = use natural layout.
    enum UIAnchor : u8
    {
        UIAnchor_None = 0,
        UIAnchor_TopLeft,
        UIAnchor_TopCenter,
        UIAnchor_TopRight,
        UIAnchor_MiddleLeft,
        UIAnchor_MiddleCenter,
        UIAnchor_MiddleRight,
        UIAnchor_BottomLeft,
        UIAnchor_BottomCenter,
        UIAnchor_BottomRight,
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
        StyleVar_ItemSpacing,   // .x = horizontal gap, .y = vertical gap between stacked children
        StyleVar_Alpha,         // .x = opacity multiplier applied to border/background/text/shadow
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
        bool released; // mouse released over this widget while it was active (click-on-release)

        bool is_initial_dragging_position_set;
        bool dragging;
        bool drag_constraint_x;
        bool drag_constraint_y;
        Vec2 drag_offset;
        Vec2 drag_mouse_p;
        Vec2 press_mouse_p; // pointer pos at press - drag-slop cancel for release-mode clicks
        
        f32 HotTransition;
        f32 ActiveTransition;
        f32 ToggleTransition; // 0 = off, 1 = on (for animated toggles)
        f32 ScaleAnimation;   // For press scale effect
        f32 AppearTransition; // 0 → 1 after creation; drives AnimateAppear pop-in
        f32 ExitTransition;   // 0 while alive; ramps 0 → 1 after the widget stops
                              // being built (AnimateExit), rendered as a fade-out;
                              // freed when it reaches 1
        i32 ExitDepth;        // tree depth cached at fade start — parent pointers
                              // can dangle mid-fade, so the render sort uses this

        u64 LastFrameIndexActive;

        UIAnchor Anchor;
        Vec2 AnchorMargin;

        f32 Rotation;

        Vec2 RenderSize;

        bool Clip;
        Vec2 ChildOffset;
        Vec4 ClipRect;

        bool FirstFrame;

        u32 StateValue;

        // Per-byte glyph highlight - bit i tints byte i of the text with
        // HighlightColour. Used for fuzzy-search matches. Bytes past 128 are
        // never highlighted.
        u64 HighlightBits[2];
        Vec4 HighlightColour;

        // Per-axis stamp for the downwards (ChildSum/MaxChild) size pass, so a
        // subtree solved by recursion isn't solved again when the outer walk
        // reaches it. Without it the pass is exponential in nesting depth.
        u64 DownwardsSizePass[UIAxis_Count];
    };

    // Glyph highlight mask for UILabelHighlighted - bit i = byte i of the text.
    struct UIHighlightMask
    {
        u64 bits[2] = { 0, 0 };

        void Set(u32 byteIndex)
        {
            if(byteIndex < 128)
                bits[byteIndex >> 6] |= (1ull << (byteIndex & 63));
        }
        bool Any() const { return (bits[0] | bits[1]) != 0; }
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

    struct UISafeAreaConfig
    {
        f32 MinMargin   = 0.0f;
        f32 ScaleTop    = 1.0f;
        f32 ScaleBottom = 1.0f;
        f32 ScaleLeft   = 1.0f;
        f32 ScaleRight  = 1.0f;
    };

    static constexpr f32 kSafeAreaEditorMarginPt = 18.0f;

    struct UI_State
    {
        Arena* UIArena;
        Arena* UIFrameArena;
        Arena* UIExitArena;

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
        Vec2 FrameBufferSize; // logical viewport the UI was laid out for; UI render projection must use this, not the (render-scaled) target pixels
        f32 DPIScale = 1.0f;
        f32 InputScale = 1.0f;
        u64 FrameIndex;
        f32 AnimationRate = 10.0f;
        f32 AnimationRateDT = 10.0f;
        
        UITheme CurrentTheme = UITheme_Light;

        // Double-click tracking
        f32 LastClickTime     = 0.0f;
        u64 LastClickedWidget = 0;
        f32 DoubleClickTime   = 0.3f; // seconds
        bool ClickOnRelease = false;
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
        u64 PendingFocusWidget = 0;

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
        bool OverlayBlocksInput = false;

        // Context menu state
        bool ContextMenuOpen    = false;
        Vec2 ContextMenuPos     = {};
        u64 ContextMenuTrigger  = 0; // Widget that triggered the menu
        u64 ContextMenuCloseRequestFrame = 0;

        u64 WidgetIdCounter = 0;

        // One-shot flags OR'd into the next widget created (see UISetNextFlags).
        u32 NextWidgetFlags = 0;

        UISafeAreaConfig SafeArea;
    };

    UI_State* GetUIState();

    Vec2 GetStringSize(String8 text, float size = 32.0f);

    bool InitialiseUI(Arena* arena);
    void ShutDownUI();

    void UIBeginFrame(const Vec2& frame_buffer_size, f32 dt, const Vec2& inputOffset);

    void UIEndFrame(Graphics::Font* font);

    UI_Interaction UIBeginPanel(const char* str, u32 extraFlags = 0);
    UI_Interaction UIBeginPanel(const char* str, SizeKind sizeKindX, float xValue, SizeKind sizeKindY, float yValue, u32 extraFlags = 0);

    void UIEndPanel();

    UI_Interaction UIBeginOverlay(const char* str, SizeKind sizeKindX = SizeKind_MaxChild, float xValue = 1.0f, SizeKind sizeKindY = SizeKind_ChildSum, float yValue = 1.0f, u32 extraFlags = WidgetFlags_StackVertically);

    UI_Interaction UIArrow(const char* str, float angleRad, float length, float thickness, const Vec4& color);

    // Window dock/positioning helpers (call between UIBeginPanel and first child)
    enum UIDockPosition { Dock_Left, Dock_Right, Dock_Top, Dock_Bottom, Dock_Fill };
    void UIWindowDock(UIDockPosition pos, float sizePercent = 0.5f);
    void UIWindowCenter();
    void UIWindowFillScreen();
    void UIWindowSetSize(float wPercent, float hPercent);
    void UIWindowAnchor(UIAnchor anchor, float marginX = 0.0f, float marginY = 0.0f);

    void UISetNextFlags(u32 flags);

    void UIPushStyle(StyleVar style_variable, float value);
    void UIPushStyle(StyleVar style_variable, const Vec2& value);
    void UIPushStyle(StyleVar style_variable, const Vec3& value);
    void UIPushStyle(StyleVar style_variable, const Vec4& value);
    void UIPopStyle(StyleVar style_variable);

    UI_Interaction UILabelCStr(const char* str, const char* text);
    UI_Interaction UILabel(const char* str, const String8& text);

    UI_Interaction UILabelWrapped(const char* str, const String8& text, float maxWidth);

    UI_Interaction UILabelEllipsis(const char* str, const String8& text, float maxWidth);
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

    // Filled rounded box (a circle when cornerRadius >= size*0.5). Generic dot/swatch.
    UI_Interaction UIColouredBox(const char* str, float width, float height,
                                 const Vec4& colour, float cornerRadius = 0.0f);

    UI_Interaction UIBeginRadar(const char* str, float diameter,
                                const Vec4& bgColour, const Vec4& ringColour);
    void UIEndRadar();

    // texture == nullptr draws a plain coloured dot (radius = size*0.5) instead.
    UI_Interaction UIRadarBlip(const char* str, float x, float y, float size,
                               const Vec4& tint, Graphics::Texture2D* texture = nullptr);

    void UIRadarRing(const char* str, float radius, const Vec4& colour,
                     float thickness = 1.0f);

    UI_Interaction UISliderRow(const char* str, float* value,
                               float min_value = 0.0f, float max_value = 1.0f,
                               const char* valueFmt = "%.2f");

    UI_Interaction UICheckbox(const char* str, bool* value);

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
    // Row that spans the parent's content width - needed when a child uses
    // UIFlexSpacer or a Grow size, which have no leftover space in a ChildSum row.
    void UIBeginRowFullWidth();
    void UIEndRow();

    // Vertical layout container
    void UIBeginColumn();
    void UIEndColumn();

    // Expander (collapsible section header)
    UI_Interaction UIExpander(const char* str, bool* expanded);
    void UIBeginExpanderContent(const char* str);
    void UIEndExpanderContent();

    // ---- Overlay/palette building blocks ----
    // Label whose masked bytes are drawn in highlightColour (fuzzy-match hits).
    UI_Interaction UILabelHighlighted(const char* str, const String8& text,
                                      const UIHighlightMask& mask, const Vec4& highlightColour);

    // Full-width clickable list row with hover + selected fills. Children are
    // stacked horizontally and vertically centred.
    UI_Interaction UIBeginSelectableRow(const char* str, bool selected, float height = 0.0f);
    void UIEndSelectableRow();

    // Small rounded pill of text - categories, ON/OFF state, key hints.
    UI_Interaction UIBadge(const char* str, const String8& text, const Vec4& bg, const Vec4& fg);

    // Grow spacer: eats the leftover space, pushing what follows to the far edge.
    void UIFlexSpacer(float weight = 1.0f);

    // Borderless search box with placeholder text. width <= 0 fills the parent.
    UI_Interaction UISearchField(const char* str, char* buffer, u32 buffer_size,
                                 const char* placeholder, float width = 0.0f);

    // Bar graph of the last `count` samples. markerValue > 0 draws a reference line.
    void UIPlotHistogram(const char* str, const float* values, u32 count,
                         float minValue, float maxValue, float width, float height,
                         const Vec4& colour, const Vec4& bgColour, float markerValue = 0.0f);

    // Draggable number field - horizontal drag scrubs the value. minValue >= maxValue
    // leaves it unbounded. Editable numeric field without needing a slider range.
    UI_Interaction UIDragFloat(const char* str, float* value, float speed = 0.01f,
                               float minValue = 0.0f, float maxValue = 0.0f,
                               const char* fmt = "%.3f", float width = 84.0f);
    // Labelled rows built on it. Return true while the value is being changed.
    bool UIDragFloatRow(const char* str, const String8& label, float* value, float speed = 0.01f,
                        float minValue = 0.0f, float maxValue = 0.0f, const char* fmt = "%.3f");
    bool UIDragVec3Row(const char* str, const String8& label, float* xyz, float speed = 0.01f);

    // Full-viewport dimming quad that swallows clicks. Build at root level.
    // The returned interaction is how a modal picks up click-outside-to-close.
    UI_Interaction UIDimBackdrop(const char* str, float alpha);

    void UIBeginScrollArea(const char* str, float height, float* scroll_offset);
    // Lua-facing variant: engine-held scroll offset, transparent skin.
    void UIBeginScrollAreaAuto(const char* str, float height);
    void UIFocusNextTextInput();
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
    UI_Widget* UIWidgetRecurseDepthFirstPostOrder(UI_Widget* Node, UI_Widget* SubtreeRoot = nullptr);

    void UILayoutSolveStandaloneSizes(UI_Widget* Root, UIAxis Axis);
    void UILayoutSolveUpwardsSizes(UI_Widget* Root, UIAxis Axis);
    void UILayoutSolveDownwardsSizes(UI_Widget* Root, UIAxis Axis);
    void UILayoutFinalisePositions(UI_Widget* Root, UIAxis Axis);
    float UIGetTextHeight(Graphics::Font* font, UI_Widget* widget);

    void RefreshUI();
    void DearIMGUIDebugPanel();
    
    // Theme management
    void UISetTheme(UITheme theme);
    void UISetClickOnRelease(bool enable);

    // Safe-area tuning (see UISafeAreaConfig).
    void UISetSafeAreaConfig(const UISafeAreaConfig& cfg);
    UISafeAreaConfig UIGetSafeAreaConfig();
    SafeAreaInsets UIResolveSafeAreaInsets(const SafeAreaInsets& raw, bool isEditor);

    bool UIIsMouseOverUI();

    // Locate a live widget by its displayed text, for scripted input. outMousePos
    // comes back in Input::GetMousePosition space so it can be injected directly.
    bool UIFindWidgetByText(const char* text, Vec2* outMousePos, Vec4* outRect = nullptr, bool substring = false);
    const char* UIGetThemeName(UITheme theme);
    void UIApplyLightTheme();
    void UIApplyDarkTheme();
    void UIApplyBlueTheme();
    void UIApplyGreenTheme();
    void UIApplyPurpleTheme();
    void UIApplyHighContrastTheme();

    inline float EaseOutCubic(float t) { return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t); }
    inline float EaseInOutCubic(float t) { return t < 0.5f ? 4.0f * t * t * t : 1.0f - (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) / 2.0f; }
}
