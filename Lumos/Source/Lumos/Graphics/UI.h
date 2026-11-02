#pragma once
#include "Core/Core.h"
#include "Core/String.h"
#include "Core/DataStructures/Map.h"
#include "Core/OS/Allocators/PoolAllocator.h"
#include "Core/DataStructures/TDArray.h"
#include "Core/OS/KeyCodes.h"

#include "Maths/Vector2.h"
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
        WidgetFlags_TopMost           = (1 << 19), // root-level widget drawn above everything built after it (popups, drag ghosts)
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
        // Text the ENGINE formats for a widget (slider readouts) lives here
        // rather than in UIFrameArena: an arena string is only guaranteed for
        // the frame that built it, and a widget that survives to the next
        // frame without being rebuilt would draw whatever now sits at that
        // address. Lua-supplied labels are owned by the caller and are fine.
        char OwnedText[64];
        Graphics::Texture2D* texture;
        bool MonoText = false; // drawn in the fixed-width font (reset each frame; set after PushWidget)
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
        u64 BuildIndex;       // order built, across frames: later = drawn over earlier
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

        // UIImageWarp: textured quad with arbitrary corners (tl, tr, br, bl) relative to
        // the widget position, drawn as vertical strips with perspective-correct u.
        // WarpStrips == 0 means an ordinary rect.
        Vec2 WarpCorners[4];
        f32 WarpDepth[2];      // view depth of the left/right edges (top/bottom when WarpVertical)
        Vec4 WarpColourBottom; // bottom-edge vertex colour; the top uses BackgroundColor
        u8 WarpStrips;
        bool WarpFlipV;
        bool WarpVertical; // depth changes top to bottom (a card tilted back): horizontal strips

        bool FlipV; // texture drawn upside down (render targets on back ends that store them bottom row first)
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

    // Memory a value widget is about to edit (see UIPushUndoTarget).
    using UIUndoEditFn = void (*)(u64 owner);

    struct UIUndoTarget
    {
        void* Ptr;
        u64 Size;
        u64 Owner;
        u64 TypeKey;
        // For data a byte snapshot cannot capture: called when an edit starts / has finished.
        UIUndoEditFn Begin;
        UIUndoEditFn End;
    };

    struct UIKeyEvent
    {
        u16 Key        = 0; // InputCode::Key, 0 for a typed character
        char Character = 0;
        bool Shift     = false;
        bool Ctrl      = false; // Cmd on a Mac
        bool Alt       = false;
    };

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
        Vec4 AccentColour = Vec4(0.0f); // w == 0: widgets use their built-in accent

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
        String8 TooltipText     = {}; // points into TooltipBuffer
        char TooltipBuffer[512] = {}; // copied: it outlives the caller's string
        bool ShowTooltip        = false;
        Vec2 TooltipPos         = {};
        f32 TooltipFontSize     = 0.0f; // of the widget it belongs to

        // A widget that reads the keyboard itself (a code editor) instead of handing over a
        // text buffer. While it is FocusedTextInput, keys and characters queue up here.
        u64 KeySink        = 0;
        u32 KeySinkCount   = 0;
        UIKeyEvent KeySinkEvents[64];

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

        // > 0: widgets built now draw and measure in the monospace font (UIPushMonoText)
        u32 MonoTextDepth = 0;

        UISafeAreaConfig SafeArea;
        // The root covers the whole framebuffer; the owner keeps controls out of the insets itself (UIRawSafeAreaInsets).
        bool FullBleed = false;

        TDArray<UIUndoTarget> UndoTargets;
        bool UndoPending         = false;
        UIUndoEditFn UndoEndFn   = nullptr;
        u64 UndoEndOwner         = 0;

        // false: quads and text are drawn strictly in tree order (a pass switch per widget)
        bool LayeredBatching = true;
        // Last frame's strings: a widget that stops being built is only found at the end of
        // this frame, and its text has to survive until then to be copied for the exit fade.
        Arena* UIPrevFrameArena;
    };

    UI_State* GetUIState();

    // More than one UI can exist (the game's, and a window-sized one for tools).
    // Every UI call works on the current context.
    UI_State* UICreateContext(Arena* arena);
    void UIDestroyContext(UI_State* state);
    void UISetContext(UI_State* state);

    // Measures in the mono font while a UIPushMonoText scope is open.
    Vec2 GetStringSize(String8 text, float size = 32.0f);
    Vec2 GetStringSizeFont(String8 text, float size, bool mono);

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

    // Undo for edits made through widgets. While a target is pushed, pressing any
    // widget built inside the scope snapshots that memory, and the step is closed
    // when the pointer is released (or the text field loses focus) - so a whole
    // drag is one undo step and the calling code never touches the undo stack.
    // owner / typeKey: see UndoPushOwned (memory that can move).
    void UIPushUndoTarget(void* ptr, u64 size, u64 owner = 0, u64 typeKey = 0);
    // Same scope, for data that is not plain bytes: begin runs when an edit starts,
    // end once it is over (before the step is committed).
    void UIPushUndoCallbacks(UIUndoEditFn begin, UIUndoEditFn end, u64 owner);
    void UIPopUndoTarget();

    // Pointer in the current context's layout space.
    Vec2 UIMousePosition();

    void UIPushStyle(StyleVar style_variable, float value);
    void UIPushStyle(StyleVar style_variable, const Vec2& value);
    void UIPushStyle(StyleVar style_variable, const Vec3& value);
    void UIPushStyle(StyleVar style_variable, const Vec4& value);
    void UIPopStyle(StyleVar style_variable);

    UI_Interaction UILabelCStr(const char* str, const char* text);
    UI_Interaction UILabel(const char* str, const String8& text);

    UI_Interaction UILabelWrapped(const char* str, const String8& text, float maxWidth);

    UI_Interaction UILabelEllipsis(const char* str, const String8& text, float maxWidth);
    // UILabelEllipsis that reports clicks (no hover background); maxWidth <= 0 = no limit
    UI_Interaction UILabelLink(const char* str, const String8& text, float maxWidth);
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

    // Free-standing shapes at a position on screen (logical px from the view's top-left).
    // Not laid out and never clickable - crosshairs, markers, hand-drawn bars.
    void UIRect(const char* str, float x, float y, float width, float height,
                const Vec4& colour, float cornerRadius = 0.0f);
    // Circle outline centred on (x, y).
    void UIRing(const char* str, float x, float y, float radius, const Vec4& colour, float thickness = 1.0f);

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

    // Textured quad with arbitrary corners (logical px, relative to the widget's top-left) - cover
    // flows, cards tilted in 3D. depthLeft/Right are the edges' view depths so the texture foreshortens
    // correctly. The widget itself is floating at `pos` with size `size` (its hit rect).
    UI_Interaction UIImageWarp(const char* str, Graphics::Texture2D* texture, const Vec2& pos, const Vec2& size,
                               const Vec2 corners[4], float depthLeft, float depthRight,
                               const Vec4& topColour, const Vec4& bottomColour, bool flipV = false, bool clickable = true,
                               float cornerRadius = 0.0f, bool vertical = false);

    // Thin click/drag scrubber for seek bars and volume. value is 0..1; sizes are logical px.
    // *active is true while the user is dragging it (so callers can stop overwriting value).
    UI_Interaction UIScrubBar(const char* str, float* value, float width, float height,
                              const Vec4& fillColour, const Vec4& trackColour, bool* active = nullptr);

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
    // a row that scrolls sideways (clipped to its parent's width)
    void UIBeginHScrollArea(const char* str, float* offset);
    void UIEndHScrollArea();
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

    // On-screen keyboard (touch platforms; nothing happens elsewhere). This is the plain key
    // keyboard for a key sink / game input - text fields bring up their own.
    enum UIKeyboardStyle
    {
        UIKeyboardStyle_Plain = 0, // the system keyboard as it comes
        UIKeyboardStyle_Code       // no autocorrect / smart quotes, and a bar of code keys (tab, arrows, brackets, undo, hide)
    };
    void UIShowKeyboard(bool show);

    // Mouse pointer shape for this frame (the window applies the last request, then forgets it).
    // Same numbering as the window backends' cursor tables.
    enum UICursor
    {
        UICursor_Arrow = 0,
        UICursor_Text,
        UICursor_ResizeAll,
        UICursor_ResizeNS,
        UICursor_ResizeEW,
        UICursor_ResizeNESW,
        UICursor_ResizeNWSE,
        UICursor_Hand,
        UICursor_NotAllowed,
    };
    void UISetCursor(UICursor cursor);
    // The shape asked for since the last call (-1 = none), cleared by reading.
    int UITakeCursor();
    void UISetKeyboardStyle(UIKeyboardStyle style);
    // How much of the screen's height the keyboard covers, 0..1 (0 when it is away).
    float UIKeyboardCover();

    // Process text input (call from app event handler)
    // Give `hash` the keyboard as a key sink (0 lets go of it).
    void UISetKeySink(u64 hash);
    bool UIIsKeySink(u64 hash);
    // Events queued for the sink since the last call; empties the queue.
    u32 UITakeKeyEvents(u64 hash, UIKeyEvent* out, u32 maxEvents);
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
    // Colour for toggles / selected rows. Alpha 0 goes back to the theme's own.
    void UISetAccentColour(const Vec4& colour);
    void UISetClickOnRelease(bool enable);
    // Widgets built between these draw (and are sized) in the monospace font.
    void UIPushMonoText();
    void UIPopMonoText();
    // Quads then text per layer (default) instead of strict tree order - see UI_State::LayeredBatching.
    void UISetLayeredBatching(bool enable);
    // scroll bars fade out when idle and back in while scrolling (off by default)
    void UISetScrollbarFade(bool enable);
    // momentum after a flick / drag and a rubber band past the ends (off by default)
    void UISetScrollPhysics(bool enable);

    // Safe-area tuning (see UISafeAreaConfig).
    void UISetSafeAreaConfig(const UISafeAreaConfig& cfg);
    UISafeAreaConfig UIGetSafeAreaConfig();
    SafeAreaInsets UIResolveSafeAreaInsets(const SafeAreaInsets& raw, bool isEditor);
    // What the OS reports, in framebuffer px, with no margins added.
    SafeAreaInsets UIRawSafeAreaInsets();

    bool UIIsMouseOverUI();

    // Locate a live widget by its displayed text, for scripted input. outMousePos
    // comes back in Input::GetMousePosition space so it can be injected directly.
    bool UIFindWidgetByText(const char* text, Vec2* outMousePos, Vec4* outRect = nullptr, bool substring = false);
    // Every clickable thing on screen, as x, y, w, h in the units of Lua's Input.GetMousePosition.
    u32 UIGetClickableRects(Vec4* out, u32 max);

    // Layout regression net: report text widgets whose rects overlap each
    // other. A label and an AlignRight value in the same row are laid out
    // independently, so a long pair silently collides - there is no layout
    // negotiation to catch it and screenshots only catch it if a human looks.
    // Returns the number of overlapping pairs found; the first `maxOut` are
    // written to outA/outB as widget text, so a test can name them.
    struct UIOverlapPair
    {
        char A[64];
        char B[64];
    };
    u32 UIFindOverlappingText(UIOverlapPair* out, u32 maxOut, float minOverlapPx = 2.0f);

    // Same net, for text that has left the screen: a panel anchored centre on a
    // window too short for it, or a row that grew past its panel, silently runs
    // off the framebuffer. Reports each offending label with how far out it is.
    struct UIOffscreenText
    {
        char Text[64];
        float OutPx;
    };
    u32 UIFindOffscreenText(UIOffscreenText* out, u32 maxOut, float tolPx = 1.0f);

    // Read the live widget tree: what is on screen, where, and with which
    // resolved style. Tests assert against this instead of against a golden
    // image, and it answers "why is this panel faint?" without a bespoke build.
    // Colours are the widget's own style vars, BEFORE the renderer folds
    // StyleVar_Alpha and the hot/active transitions into them - Alpha is
    // reported separately so a caller can see both.
    struct UIWidgetInfo
    {
        char Text[64];
        float X, Y, W, H;
        float Alpha;                 // resolved StyleVar_Alpha
        float BG[4];                 // StyleVar_BackgroundColor
        float Border[4];             // StyleVar_BorderColor
        float TextCol[4];            // StyleVar_TextColor
        float CornerRadius;
        float Clip[4];               // x, y, right, bottom
        u32 Flags;
        bool Hot;
        bool Offscreen;              // any edge past the framebuffer
        bool Clipped;                // entirely outside its own clip rect
    };
    // Returns the number of live widgets this frame, writing up to maxOut of
    // them. Pass a filter to keep only widgets whose text contains it.
    u32 UIQueryWidgets(UIWidgetInfo* out, u32 maxOut, const char* textFilter = nullptr);
    const char* UIGetThemeName(UITheme theme);
    void UIApplyLightTheme();
    void UIApplyDarkTheme();
    void UIApplyBlueTheme();
    void UIApplyGreenTheme();
    void UIApplyPurpleTheme();
    void UIApplyHighContrastTheme();

    // ---- Tool widgets (UIWidgets.cpp) ----

    // Plain container. Children stack vertically unless flags say otherwise.
    UI_Interaction UIBeginChild(const char* str, UI_Size width, UI_Size height, u32 flags = WidgetFlags_StackVertically, bool clip = false);
    void UIEndChild();

    // Rect of the widget built last under the current parent: x, y, width, height (last frame's layout).
    Vec4 UIGetLastWidgetRect();
    bool UIIsLastWidgetHovered();

    // Popups: a floating list at root level, closed by a click outside it.
    void UIOpenPopup(const char* id);
    void UIOpenPopupAt(const char* id, const Vec2& position);
    bool UIIsPopupOpen(const char* id);
    bool UIBeginPopup(const char* id);
    void UIEndPopup();
    void UICloseCurrentPopup();
    void UICloseAllPopups();

    // Menu bar: a strip of titles, each opening a popup of items. Menus nest.
    bool UIBeginMenuBar(const char* str);
    void UIEndMenuBar();
    bool UIBeginMenu(const char* str, bool enabled = true);
    void UIEndMenu();
    // true when chosen. `checked` draws a tick and is flipped on click.
    bool UIMenuItem(const char* str, const char* shortcut = nullptr, bool* checked = nullptr, bool enabled = true);
    void UIMenuSeparator();
    // Shortcuts are written "Ctrl X"; returns the text for this platform ("Cmd X" on a Mac).
    const char* UIShortcutText(const char* hint, char* out, u32 outSize);

    // Square button holding an icon glyph (or any short text).
    UI_Interaction UIIconButton(const char* str, float size = 24.0f, bool selected = false);
    UI_Interaction UIImageButton(const char* str, Graphics::Texture2D* texture, const Vec2& size, bool flipV = false);
    // UIImage with an explicit size in px; flipV for render targets.
    UI_Interaction UIImageSized(const char* str, Graphics::Texture2D* texture, const Vec2& size, bool flipV = false);

    // Number fields: drag to scrub, double-click to type. width <= 0 fills the row.
    bool UIDragFloatField(const char* str, float* value, float speed = 0.01f, float minValue = 0.0f, float maxValue = 0.0f,
                          const char* fmt = "%.3f", float width = 0.0f, const Vec4* tint = nullptr);
    bool UIDragIntField(const char* str, int* value, float speed = 0.2f, int minValue = 0, int maxValue = 0, float width = 0.0f);

    // Two-column property rows: label on the left, editor on the right.
    void UIBeginPropertyGrid(const char* str, float labelFraction = 0.38f);
    void UIEndPropertyGrid();
    void UIPropertyLabel(const char* label, const String8& value);
    bool UIPropertyFloat(const char* label, float* value, float speed = 0.01f, float minValue = 0.0f, float maxValue = 0.0f, const char* fmt = "%.3f");
    bool UIPropertyInt(const char* label, int* value, float speed = 0.2f, int minValue = 0, int maxValue = 0);
    bool UIPropertyBool(const char* label, bool* value);
    bool UIPropertyVec2(const char* label, float* xy, float speed = 0.01f);
    bool UIPropertyVec3(const char* label, float* xyz, float speed = 0.01f);
    bool UIPropertyVec4(const char* label, float* xyzw, float speed = 0.01f);
    bool UIPropertyColour(const char* label, float* rgba, bool alpha = true);
    bool UIPropertyText(const char* label, char* buffer, u32 bufferSize, const char* placeholder = "");
    // File picker over everything. One at a time; the callback gets the full path that was chosen.
    struct UIFileDialogDesc
    {
        const char* Title       = "Open";
        const char* StartFolder = nullptr; // null = the user's home
        const char* Extension   = nullptr; // ".png": other files are hidden. null = any
        const char* Accept      = "Open";
        bool PickFolder         = false;
        void (*OnPicked)(const char* path, void* user) = nullptr;
        void* User                                     = nullptr;
    };
    void UIOpenFileDialog(const UIFileDialogDesc& desc);
    bool UIFileDialogIsOpen();
    // Builds it when one is open. Call once a frame, after everything it should cover.
    void UIDrawFileDialog();

    // A larger view of a picture beside the pointer, once it has rested on the widget for a moment.
    // Call straight after the widget's interaction; caption is one or more lines under the picture.
    void UIImageTooltip(const UI_Interaction& hovered, Graphics::Texture2D* texture, const char* caption = nullptr, bool flipV = false);

    // Thin slider in the tool theme: no label, no number. width in logical px (0 = fill the row). True while it changes the value.
    bool UIToolSlider(const char* str, float* value, float minValue, float maxValue, float width = 120.0f);
    // Property grid rows. Slider + the number beside it, for values with a fixed range.
    bool UIPropertySlider(const char* label, float* value, float minValue, float maxValue, const char* fmt = "%.2f");
    // A title for the rows that follow, with a hairline. Not collapsible.
    void UIPropertyHeading(const char* text);
    // A lighter, collapsible group inside a grid. Draw its rows when it returns true.
    // thumbnail = a small picture before the title (a material's texture, say).
    bool UIPropertyFold(const char* str, bool defaultOpen = true, Graphics::Texture2D* thumbnail = nullptr);
    // Single-line text box in the tool theme: UISearchField with the tools' field colours, rounding and focus ring.
    UI_Interaction UITextField(const char* str, char* buffer, u32 bufferSize, const char* placeholder = "", float width = 0.0f);
    bool UIPropertyDropdown(const char* label, int* selected, const char** options, int optionCount);
    // Starts the row and leaves the right-hand cell open for custom widgets.
    void UIBeginPropertyRow(const char* label);
    void UIEndPropertyRow();

    // Collapsing section header for inspectors; state kept by the engine. true = draw the body.
    bool UISectionHeader(const char* str, bool defaultOpen = true, bool* removeClicked = nullptr);

    // Table: rows of cells sharing column widths (fractions of the table width, owned by the caller).
    void UIBeginTable(const char* str, int columns, float* fractions);
    // Header row; the dividers drag to resize.
    void UITableHeader(const char** names);
    // stableId keeps a row's widgets the same when rows above it come and go (0 = use the row index).
    void UITableNextRow(float height = 0.0f, u64 stableId = 0);
    void UITableNextColumn();
    void UIEndTable();

    // Bar between two regions; dragging it changes *value (px). Returns true while dragged.
    bool UISplitter(const char* str, bool vertical, float* value, float minValue, float maxValue, float thickness = 4.0f, bool invert = false);

    // Drag and drop between widgets. Call each right after the widget it applies to.
    // The payload is copied (up to 256 bytes).
    bool UIDragSource(const char* type, const void* data, u32 size, const String8& preview);
    // true on the frame a matching payload is dropped on the widget; the data is copied out.
    bool UIDropTarget(const char* type, void* outData, u32 size);
    bool UIIsDragDropActive(const char* type = nullptr);

    inline float EaseOutCubic(float t) { return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t); }
    inline float EaseInOutCubic(float t) { return t < 0.5f ? 4.0f * t * t * t : 1.0f - (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) / 2.0f; }
}
