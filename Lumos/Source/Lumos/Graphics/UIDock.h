#pragma once
#include "Graphics/UI.h"

namespace Lumos
{
    // Dockable panels on the engine UI: a tree of splits with tab stacks at the
    // leaves, panels dragged between them, and drawers that slide in over the
    // layout. One OS window; everything is built into the current UI context.
    struct UIDockSpace;

    enum UIDockPanelFlags : u32
    {
        UIDockPanel_None      = 0,
        UIDockPanel_NoScroll  = 1 << 0, // the panel does its own scrolling; the body is only clipped
        UIDockPanel_NoPadding = 1 << 1,
        UIDockPanel_NoClose   = 1 << 2,
        UIDockPanel_NoTabBar  = 1 << 3, // no tab strip while it is alone in its node
        UIDockPanel_FullBleed = 1 << 4, // solo: draws under the safe insets and keeps its own controls clear
    };

    // Where a panel goes in the default layout.
    enum UIDockSlot : u8
    {
        UIDockSlot_Centre = 0,
        UIDockSlot_Left,
        UIDockSlot_LeftBottom,
        UIDockSlot_Right,
        UIDockSlot_RightBottom,
        UIDockSlot_Bottom,
        UIDockSlot_Top,
        UIDockSlot_Count
    };

    struct UIDockPanelContext
    {
        Vec2 Position; // top-left of the area the panel can draw in
        Vec2 Size;
        float DPIScale;
        bool Focused; // last panel clicked
        bool Hovered;
        void* UserData;
    };

    using UIDockPanelFn = void (*)(const UIDockPanelContext& context);

    struct UIDockPanelDesc
    {
        const char* Name    = nullptr; // unique; also the key in the saved layout
        const char* Title   = nullptr; // on the tab (can start with an icon glyph)
        UIDockPanelFn Draw  = nullptr;
        void* UserData      = nullptr;
        UIDockSlot Slot     = UIDockSlot_Centre;
        u32 Flags           = UIDockPanel_None;
        bool Open           = true;
        bool Drawer         = false; // starts in the drawer bar rather than the layout
    };

    UIDockSpace* UIDockSpaceCreate(const char* name);
    void UIDockSpaceDestroy(UIDockSpace* space);

    // The centre of the layout is left empty and see-through (a game shows behind it)
    // instead of holding panels. Set before the first draw.
    void UIDockSetCentralPassthrough(UIDockSpace* space, bool passthrough);
    // 0 = follow the tool theme.
    void UIDockSetFontSize(UIDockSpace* space, float logicalPx);
    // Short status line at the right-hand end of the drawer bar. Copied; null clears it.
    void UIDockSetBarText(UIDockSpace* space, const char* text);
    // Keep the drawer bar up even with nothing on it, so any tab can be dropped there.
    void UIDockSetDrawerBarAlwaysVisible(UIDockSpace* space, bool always);

    // Drawers keep working (shortcuts, UIDockToggleDrawer) with no bar drawn.
    void UIDockSetDrawerBarHidden(UIDockSpace* space, bool hidden);
    // With a solo panel, keep the bar and list the layout's panels on it as well: it is how the solo one is picked.
    void UIDockSetBarInSolo(UIDockSpace* space, bool show);
    // Notch / rounded-corner insets in px from the rect's edges (left, top, right, bottom).
    // Panels still fill the rect; tab strips, the drawer bar and scrolling bodies keep inside.
    // edgeToEdge drops the margin round the layout.
    void UIDockSetSafeInsets(UIDockSpace* space, const Vec4& insets, bool edgeToEdge);
    // One panel fills the rect, no tabs or dividers. null = back to the layout.
    void UIDockSetSolo(UIDockSpace* space, const char* name);
    const char* UIDockGetSolo(UIDockSpace* space);
    const char* UIDockGetOpenDrawer(UIDockSpace* space);
    // Last panel pressed or focused; null before any.
    const char* UIDockGetFocusedPanel(UIDockSpace* space);

    void UIDockRegisterPanel(UIDockSpace* space, const UIDockPanelDesc& desc);

    // Builds everything. rect = x, y, width, height in the UI's layout space.
    void UIDockSpaceDraw(UIDockSpace* space, const Vec4& rect);

    void UIDockSetPanelOpen(UIDockSpace* space, const char* name, bool open);
    bool UIDockIsPanelOpen(UIDockSpace* space, const char* name);
    void UIDockFocusPanel(UIDockSpace* space, const char* name);
    // Being drawn right now (open, and the front tab or an open drawer).
    bool UIDockIsPanelVisible(UIDockSpace* space, const char* name);
    // Body rect from the last draw. false when the panel is not visible.
    bool UIDockGetPanelRect(UIDockSpace* space, const char* name, Vec4* outRect);
    // The see-through centre (central passthrough only).
    Vec4 UIDockGetCentralRect(UIDockSpace* space);

    // Drawers: panels parked on a bar along the bottom that slide up over the layout.
    // Any panel can go there: drag its tab onto the bar, press the arrow at the end of its
    // tab strip, or right-click the tab. Ctrl/Cmd+Space brings back the last one used,
    // Alt+1..9 picks by place on the bar, Escape or a press elsewhere puts it away.
    void UIDockSetPanelDrawer(UIDockSpace* space, const char* name, bool drawer);
    // How far up the layout an open drawer comes (0.12 .. 0.92 of its height). name = null sets every panel.
    void UIDockSetDrawerSize(UIDockSpace* space, const char* name, float fraction);
    void UIDockToggleDrawer(UIDockSpace* space, const char* name);
    bool UIDockIsPanelDrawer(UIDockSpace* space, const char* name);

    u32 UIDockGetPanelCount(UIDockSpace* space);
    const char* UIDockGetPanelName(UIDockSpace* space, u32 index);
    const char* UIDockGetPanelTitle(UIDockSpace* space, u32 index);

    // Pointer over any dock UI (not the see-through centre).
    bool UIDockIsMouseOver(UIDockSpace* space);
    // A tab or splitter is being dragged.
    bool UIDockIsDragging(UIDockSpace* space);

    void UIDockResetLayout(UIDockSpace* space);
    // JSON (cereal). Paths are plain file paths.
    bool UIDockSaveLayout(UIDockSpace* space, const String8& path);
    bool UIDockLoadLayout(UIDockSpace* space, const String8& path);
}
