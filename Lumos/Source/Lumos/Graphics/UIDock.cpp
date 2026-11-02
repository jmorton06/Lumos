#include "Precompiled.h"
#include "UIDock.h"
#include "UIInternal.h"
#include "UITheme.h"

#include "Core/OS/Input.h"
#include "Core/OS/FileSystem.h"
#include "Core/OS/Memory.h"
#include "Maths/MathsUtilities.h"
#include "Utilities/StringUtilities.h"
#include "ImGui/IconsMaterialDesignIcons.h"
#include <imgui/imgui.h>

#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <sstream>

namespace Lumos
{
    namespace
    {
        constexpr u32 kMaxPanels = 48;
        constexpr i32 kMaxNodes  = 64;
        constexpr i32 kMaxTabs   = 16;

        struct DockPanel
        {
            char Name[48];
            char Title[64];
            u64 Hash;
            UIDockPanelFn Draw;
            void* UserData;
            u32 Flags;
            UIDockSlot Slot;

            bool Open;
            bool Drawer;
            bool DefaultOpen;
            bool DefaultDrawer;

            i32 Node;     // leaf it sits in, -1 = none
            i32 LastNode; // where it was when it was closed
            f32 Scroll;
            Vec4 BodyRect;
            bool Visible;
            f32 Appear; // 0 -> 1 fade when it comes to the front

            f32 DrawerT;    // 0 = hidden, 1 = fully out
            f32 DrawerSize; // share of the layout height
        };

        // Either a split (two children) or a leaf (a stack of tabs).
        struct DockNode
        {
            bool Used;
            bool Central; // the middle of the layout: never removed, may be empty
            i32 Parent;
            i32 Child[2];
            bool SideBySide; // children left / right rather than top / bottom
            f32 Ratio;       // share of child 0
            f32 AnimRatio;   // what is drawn; eases towards Ratio
            i32 Collapsing;  // child being closed (its share eases to nothing), -1 = none

            i32 Tabs[kMaxTabs];
            i32 TabCount;
            i32 ActiveTab;
            f32 TabScroll;     // tab strip: sideways offset when the tabs do not fit
            i32 TabLastActive; // to bring a newly active tab into view

            Vec4 Rect;
        };

        inline bool IsLeaf(const DockNode& node) { return node.Child[0] < 0; }

        inline bool PointInRect(const Vec2& p, const Vec4& r)
        {
            return p.x >= r.x && p.x <= r.x + r.z && p.y >= r.y && p.y <= r.y + r.w;
        }

        struct DockColours
        {
            Vec4 Gap;
            Vec4 Strip;
            Vec4 Body;
            Vec4 Hover;
            Vec4 Text;
            Vec4 Dim;
            Vec4 Accent;
            Vec4 Border;
        };

        DockColours Colours()
        {
            const UIToolTheme& theme = UIGetToolTheme();
            DockColours c;
            c.Gap    = theme.Canvas;
            c.Strip  = theme.Header;
            c.Body   = theme.Panel;
            c.Hover  = theme.Hover;
            c.Text   = theme.Text;
            c.Dim    = theme.TextDim;
            c.Border = theme.Border;
            c.Accent = theme.Accent;
            return c;
        }

        void CopyString(char* dst, size_t size, const char* src)
        {
            size_t n = 0;
            if(src)
                for(; src[n] && n < size - 1; n++)
                    dst[n] = src[n];
            dst[n] = 0;
        }
    }

    struct UIDockSpace
    {
        Arena* Memory;
        char Name[32];
        u64 Hash;

        DockPanel Panels[kMaxPanels];
        u32 PanelCount;

        DockNode Nodes[kMaxNodes];
        i32 Root;
        i32 SlotNode[UIDockSlot_Count];

        bool LayoutBuilt;
        bool CentralPassthrough;
        bool DrawerBarAlways; // the bar shows with nothing on it, as somewhere to drop tabs
        f32 FontSize;         // 0 = the theme's
        i32 LastDrawer;       // the one the shortcut brings back
        char BarText[160];
        Vec4 BarRect;

        i32 FocusedPanel;
        i32 MenuPanel;
        i32 OpenDrawer;

        bool Dragging;
        i32 DragPanel;
        bool SplitterActive;

        Vec4 Rect;
        Vec4 CentralRect;
        bool MouseOver;

        Vec4 Safe;       // left, top, right, bottom: where controls must not go
        bool EdgeToEdge; // no margin round the layout
        bool BarHidden;  // drawers still work, the bar is not drawn
        i32 Solo;        // one panel fills the rect, -1 = none
        bool BarInSolo;  // the bar stays up in solo and lists the layout's panels too
        f32 BarScroll;   // edge to edge: the buttons slide sideways when there are too many
        f32 BarContent;  // their width last frame, to centre them
    };

    namespace
    {
        i32 FindPanel(UIDockSpace* s, const char* name)
        {
            if(!s || !name)
                return -1;
            for(u32 i = 0; i < s->PanelCount; i++)
                if(strcmp(s->Panels[i].Name, name) == 0)
                    return (i32)i;
            return -1;
        }

        i32 AllocNode(UIDockSpace* s)
        {
            for(i32 i = 0; i < kMaxNodes; i++)
            {
                if(!s->Nodes[i].Used)
                {
                    DockNode& n  = s->Nodes[i];
                    n            = {};
                    n.Used       = true;
                    n.Parent     = -1;
                    n.Child[0]   = -1;
                    n.Child[1]   = -1;
                    n.Collapsing = -1;
                    n.Ratio      = 0.5f;
                    n.AnimRatio  = 0.5f;
                    return i;
                }
            }
            return -1;
        }

        // Puts a new empty leaf beside `node` and returns it. side: 0 left, 1 right, 2 top, 3 bottom.
        // share = how much of the space the new leaf gets.
        i32 SplitNode(UIDockSpace* s, i32 node, int side, f32 share, bool animate)
        {
            if(node < 0 || !s->Nodes[node].Used)
                return -1;
            const i32 split = AllocNode(s);
            const i32 leaf  = split >= 0 ? AllocNode(s) : -1;
            if(split < 0 || leaf < 0)
            {
                if(split >= 0)
                    s->Nodes[split].Used = false;
                return -1;
            }

            DockNode& S  = s->Nodes[split];
            S.Parent     = s->Nodes[node].Parent;
            if(S.Parent >= 0)
            {
                DockNode& parent = s->Nodes[S.Parent];
                if(parent.Child[0] == node)
                    parent.Child[0] = split;
                else
                    parent.Child[1] = split;
            }
            else
                s->Root = split;

            const bool newFirst          = (side == 0 || side == 2);
            S.SideBySide                 = side < 2;
            S.Child[newFirst ? 0 : 1]    = leaf;
            S.Child[newFirst ? 1 : 0]    = node;
            S.Ratio                      = newFirst ? share : 1.0f - share;
            S.AnimRatio                  = animate ? (newFirst ? 0.0f : 1.0f) : S.Ratio; // slides open from nothing
            s->Nodes[node].Parent        = split;
            s->Nodes[leaf].Parent        = split;
            return leaf;
        }

        // `leaf` has closed: its sibling takes over the parent's place in the tree.
        void RemoveLeaf(UIDockSpace* s, i32 leaf)
        {
            const i32 parent = s->Nodes[leaf].Parent;
            if(parent < 0)
                return;
            DockNode& P       = s->Nodes[parent];
            const i32 sibling = P.Child[0] == leaf ? P.Child[1] : P.Child[0];
            const i32 grand   = P.Parent;

            s->Nodes[sibling].Parent = grand;
            if(grand >= 0)
            {
                DockNode& G = s->Nodes[grand];
                if(G.Child[0] == parent)
                    G.Child[0] = sibling;
                else
                    G.Child[1] = sibling;
            }
            else
                s->Root = sibling;

            for(i32 i = 0; i < UIDockSlot_Count; i++)
                if(s->SlotNode[i] == leaf)
                    s->SlotNode[i] = -1;
            for(u32 i = 0; i < s->PanelCount; i++)
                if(s->Panels[i].LastNode == leaf)
                    s->Panels[i].LastNode = -1;

            s->Nodes[leaf].Used   = false;
            s->Nodes[parent].Used = false;
        }

        void DetachPanel(UIDockSpace* s, i32 panel)
        {
            DockPanel& p   = s->Panels[panel];
            const i32 node = p.Node;
            if(node < 0)
                return;

            DockNode& n = s->Nodes[node];
            for(i32 i = 0; i < n.TabCount; i++)
            {
                if(n.Tabs[i] != panel)
                    continue;
                for(i32 k = i + 1; k < n.TabCount; k++)
                    n.Tabs[k - 1] = n.Tabs[k];
                n.TabCount--;
                if(n.ActiveTab > i || n.ActiveTab >= n.TabCount)
                    n.ActiveTab = Maths::Max(0, n.ActiveTab - 1);
                break;
            }

            p.Node     = -1;
            p.LastNode = node;
            p.Visible  = false;

            // nothing left in it: ease it shut (the node goes once the animation is done)
            if(n.TabCount == 0 && !n.Central && n.Parent >= 0)
            {
                DockNode& parent  = s->Nodes[n.Parent];
                parent.Collapsing = parent.Child[0] == node ? 0 : 1;
            }
        }

        void AttachPanel(UIDockSpace* s, i32 panel, i32 node)
        {
            if(node < 0 || !s->Nodes[node].Used || !IsLeaf(s->Nodes[node]))
                return;
            DockNode& n = s->Nodes[node];
            if(n.TabCount >= kMaxTabs)
                return;

            n.Tabs[n.TabCount] = panel;
            n.ActiveTab        = n.TabCount;
            n.TabCount++;

            DockPanel& p = s->Panels[panel];
            p.Node       = node;
            p.Appear     = 0.0f;

            // it was on its way out: bring it back
            if(n.Parent >= 0)
            {
                DockNode& parent = s->Nodes[n.Parent];
                if(parent.Collapsing >= 0 && parent.Child[parent.Collapsing] == node)
                    parent.Collapsing = -1;
            }
        }

        i32 SlotLeaf(UIDockSpace* s, UIDockSlot slot, bool animate)
        {
            const i32 existing = s->SlotNode[slot];
            if(existing >= 0 && s->Nodes[existing].Used && IsLeaf(s->Nodes[existing]))
                return existing;

            i32 leaf = -1;
            switch(slot)
            {
            case UIDockSlot_Left:
                leaf = SplitNode(s, s->Root, 0, 0.18f, animate);
                break;
            case UIDockSlot_Right:
                leaf = SplitNode(s, s->Root, 1, 0.22f, animate);
                break;
            case UIDockSlot_Bottom:
                leaf = SplitNode(s, s->SlotNode[UIDockSlot_Centre], 3, 0.28f, animate);
                break;
            case UIDockSlot_Top:
                leaf = SplitNode(s, s->SlotNode[UIDockSlot_Centre], 2, 0.14f, animate);
                break;
            case UIDockSlot_LeftBottom:
                leaf = SplitNode(s, SlotLeaf(s, UIDockSlot_Left, animate), 3, 0.45f, animate);
                break;
            case UIDockSlot_RightBottom:
                leaf = SplitNode(s, SlotLeaf(s, UIDockSlot_Right, animate), 3, 0.45f, animate);
                break;
            default:
                return s->SlotNode[UIDockSlot_Centre];
            }

            if(leaf < 0)
                return s->SlotNode[UIDockSlot_Centre];
            s->SlotNode[slot] = leaf;
            return leaf;
        }

        // A see-through centre cannot hold panels: they go to the right instead.
        UIDockSlot UsableSlot(UIDockSpace* s, UIDockSlot slot)
        {
            if(slot == UIDockSlot_Centre && s->CentralPassthrough)
                return UIDockSlot_Right;
            return slot;
        }

        void BuildDefaultLayout(UIDockSpace* s)
        {
            for(i32 i = 0; i < kMaxNodes; i++)
                s->Nodes[i].Used = false;
            for(i32 i = 0; i < UIDockSlot_Count; i++)
                s->SlotNode[i] = -1;

            s->Root                       = AllocNode(s);
            s->Nodes[s->Root].Central     = true;
            s->SlotNode[UIDockSlot_Centre] = s->Root;
            s->OpenDrawer                 = -1;
            s->Dragging                   = false;
            s->DragPanel                  = -1;

            for(u32 i = 0; i < s->PanelCount; i++)
            {
                DockPanel& p = s->Panels[i];
                p.Node       = -1;
                p.LastNode   = -1;
                p.Open       = p.DefaultOpen;
                p.Drawer     = p.DefaultDrawer;
                p.DrawerT    = 0.0f;
                p.Visible    = false;
            }

            // fixed order, so the result does not depend on the order panels were registered in
            static const UIDockSlot kOrder[] = { UIDockSlot_Left, UIDockSlot_Right, UIDockSlot_Bottom, UIDockSlot_Top,
                                                 UIDockSlot_LeftBottom, UIDockSlot_RightBottom, UIDockSlot_Centre };
            for(UIDockSlot slot : kOrder)
            {
                for(u32 i = 0; i < s->PanelCount; i++)
                {
                    DockPanel& p = s->Panels[i];
                    if(!p.Open || p.Drawer || p.Node >= 0 || UsableSlot(s, p.Slot) != slot)
                        continue;
                    AttachPanel(s, (i32)i, SlotLeaf(s, slot, false));
                }
            }

            for(i32 i = 0; i < kMaxNodes; i++)
                if(s->Nodes[i].Used)
                    s->Nodes[i].ActiveTab = 0;

            s->LayoutBuilt = true;
        }

        void ComputeRects(UIDockSpace* s, i32 node, const Vec4& rect, f32 gap)
        {
            DockNode& n = s->Nodes[node];
            n.Rect      = rect;
            if(IsLeaf(n))
                return;

            // the divider shrinks away with the side that is closing
            f32 gapNow = gap;
            if(n.Collapsing == 0)
                gapNow *= Maths::Clamp(n.AnimRatio / Maths::Max(n.Ratio, 0.01f), 0.0f, 1.0f);
            else if(n.Collapsing == 1)
                gapNow *= Maths::Clamp((1.0f - n.AnimRatio) / Maths::Max(1.0f - n.Ratio, 0.01f), 0.0f, 1.0f);

            if(n.SideBySide)
            {
                const f32 total = Maths::Max(0.0f, rect.z - gapNow);
                const f32 first = floorf(total * n.AnimRatio);
                ComputeRects(s, n.Child[0], Vec4(rect.x, rect.y, first, rect.w), gap);
                ComputeRects(s, n.Child[1], Vec4(rect.x + first + gapNow, rect.y, total - first, rect.w), gap);
            }
            else
            {
                const f32 total = Maths::Max(0.0f, rect.w - gapNow);
                const f32 first = floorf(total * n.AnimRatio);
                ComputeRects(s, n.Child[0], Vec4(rect.x, rect.y, rect.z, first), gap);
                ComputeRects(s, n.Child[1], Vec4(rect.x, rect.y + first + gapNow, rect.z, total - first), gap);
            }
        }

        // A widget placed by hand: floating under `parent` at an absolute rect. Ends with EndFloating.
        UI_Widget* BeginFloating(UI_Widget* parent, const Vec2& parentPosition, u64 hash, const Vec4& rect, u32 flags, bool clip)
        {
            UI_State* ui = GetUIState();
            ui->parents.PushBack(parent);

            UI_Widget* widget = PushWidget(flags | WidgetFlags_Floating_X | WidgetFlags_Floating_Y, Str8Lit(""), hash,
                                           { SizeKind_Pixels, Maths::Max(rect.z, 0.0f) },
                                           { SizeKind_Pixels, Maths::Max(rect.w, 0.0f) });
            widget->relative_position = Vec2(rect.x, rect.y) - parentPosition;
            widget->Clip              = clip;
            UIClearBorders(widget);
            widget->style_vars[StyleVar_Padding]      = Vec4(0.0f);
            widget->style_vars[StyleVar_ItemSpacing]  = Vec4(0.0f);
            widget->style_vars[StyleVar_CornerRadius] = Vec4(0.0f);

            PushParent(widget);
            return widget;
        }

        UI_Widget* BeginRootFloating(u64 hash, const Vec4& rect, u32 flags, bool clip)
        {
            UI_State* ui = GetUIState();
            return BeginFloating(&ui->root_parent, ui->root_parent.position, hash, rect, flags, clip);
        }

        void EndFloating()
        {
            PopParent(GetCurrentParent());
            GetUIState()->parents.PopBack();
        }

        struct DrawState
        {
            UIDockSpace* Space;
            UI_State* UI;
            DockColours Colour;
            f32 DPI;
            f32 DT;
            Vec2 Mouse;
            bool Held;
            bool Pressed;
            i32 ClosePanel;
            i32 ToggleDrawerPanel;
            i32 SoloPanel;
            f32 Radius;      // panel corners, px
            f32 WidgetRadius;
            f32 TabHeight;
        };

        void StartDragIfMoved(DrawState& d, UI_Widget* handle, i32 panel)
        {
            UIDockSpace* s = d.Space;
            if(s->Dragging || !d.Held || d.UI->active_widget != handle->hash)
                return;
            const Vec2 moved = d.Mouse - handle->press_mouse_p;
            const f32 slop   = 8.0f * d.DPI;
            if(moved.x * moved.x + moved.y * moved.y > slop * slop)
            {
                s->Dragging  = true;
                s->DragPanel = panel;
            }
        }

        // The panel's own content, in a clipped box. `parent` is the root, or the
        // sliding container of a drawer.
        void DrawPanelBody(DrawState& d, i32 panelIndex, UI_Widget* parent, const Vec2& parentPosition, const Vec4& body)
        {
            UIDockSpace* s = d.Space;
            DockPanel& p   = s->Panels[panelIndex];
            p.BodyRect     = body;
            p.Visible      = true;
            p.Appear       = Maths::Min(1.0f, p.Appear + d.DT * 9.0f);

            if(body.z < 2.0f || body.w < 2.0f)
                return;

            const bool scrolls = (p.Flags & UIDockPanel_NoScroll) == 0;
            const f32 padding  = (scrolls || (p.Flags & UIDockPanel_NoPadding)) ? 0.0f : 8.0f * d.DPI;

            UIPushStyle(StyleVar_Alpha, EaseOutCubic(p.Appear));

            UI_Widget* box = BeginFloating(parent, parentPosition, UIHashCombine(s->Hash, p.Hash), body,
                                           WidgetFlags_StackVertically, true);
            // the island drawn behind the leaf is the panel; this only holds and clips the content
            box->style_vars[StyleVar_BackgroundColor] = Vec4(0.0f);
            box->style_vars[StyleVar_Padding]         = Vec4(padding, padding, 0.0f, 0.0f);
            // a panel that asked for no padding places everything itself: no gaps added between its parts
            box->style_vars[StyleVar_ItemSpacing]     = Vec4(0.0f, (p.Flags & UIDockPanel_NoPadding) ? 0.0f : 3.0f * d.DPI, 0.0f, 0.0f);

            const bool hovered = PointInRect(d.Mouse, body) && !d.UI->OverlayBlocksInput;
            if(hovered && d.Pressed)
                s->FocusedPanel = panelIndex;

            UIDockPanelContext context;
            context.Position = Vec2(body.x + padding, body.y + padding);
            context.Size     = Vec2(body.z - padding * 2.0f, body.w - padding * 2.0f);
            context.DPIScale = d.DPI;
            context.Focused  = s->FocusedPanel == panelIndex;
            context.Hovered  = hovered;
            context.UserData = p.UserData;

            if(p.Draw)
            {
                if(scrolls)
                {
                    char id[80];
                    snprintf(id, sizeof(id), "##dockscroll%s", p.Name);
                    UIBeginScrollArea(id, body.w, &p.Scroll);
                    // the scroll bar floats over the right edge: the margin keeps content clear of it
                    const f32 marginX                         = 10.0f * d.DPI;
                    const f32 marginY                         = 6.0f * d.DPI;
                    UI_Widget* area                           = d.UI->parents.Back();
                    area->style_vars[StyleVar_BackgroundColor] = Vec4(0.0f);
                    area->style_vars[StyleVar_BorderColor]     = Vec4(0.0f);
                    area->style_vars[StyleVar_Border]          = Vec4(0.0f);
                    area->style_vars[StyleVar_Padding]         = Vec4(marginX, marginY, 0.0f, 0.0f);
                    area->style_vars[StyleVar_ItemSpacing]     = Vec4(0.0f, 3.0f * d.DPI, 0.0f, 0.0f);

                    context.Position = Vec2(body.x + marginX, body.y + marginY);
                    context.Size     = Vec2(body.z - marginX * 2.0f, body.w - marginY * 2.0f);
                    p.Draw(context);
                    UIEndScrollArea();
                }
                else
                {
                    p.Draw(context);
                }
            }

            EndFloating();
            UIPopStyle(StyleVar_Alpha);
        }

        // The icon a title starts with, or the whole title when it has none.
        inline String8 TabIconOf(const char* title)
        {
            String8 label = Str8C((char*)title);
            if((u8)title[0] >= 0xC0)
                label.size = (u8)title[0] >= 0xF0 ? 4 : ((u8)title[0] >= 0xE0 ? 3 : 2);
            return label;
        }

        // Returns the tab's width. iconOnly: just the icon each title starts with (the full name in a tooltip).
        f32 DrawTab(DrawState& d, DockNode& node, i32 tabIndex, bool isActive, bool iconOnly)
        {
            UIDockSpace* s          = d.Space;
            const i32 panelIndex    = node.Tabs[tabIndex];
            DockPanel& p            = s->Panels[panelIndex];
            const DockColours& c    = d.Colour;
            const f32 dpi           = d.DPI;
            const i32 style         = UIGetToolTheme().TabStyle;
            const bool focused      = s->FocusedPanel == panelIndex;
            const bool pill         = style == UIToolTabStyle_Pill;
            const f32 height        = pill ? floorf(d.TabHeight - 8.0f * dpi) : d.TabHeight;

            // children centre themselves: centring from the tab would drag the floating marker to the middle too
            UI_Widget* tab = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground | WidgetFlags_StackHorizontally | WidgetFlags_CentreY,
                                        Str8Lit(""), UIHashCombine(p.Hash, 0x544142ull),
                                        { SizeKind_ChildSum, 1.0f }, { SizeKind_Pixels, height });
            UIClearBorders(tab);
            tab->style_vars[StyleVar_Padding]      = Vec4(10.0f * dpi, 0.0f, 0.0f, 0.0f);
            tab->style_vars[StyleVar_ItemSpacing]  = Vec4(6.0f * dpi, 0.0f, 0.0f, 0.0f);
            tab->style_vars[StyleVar_CornerRadius] = Vec4(pill ? d.WidgetRadius + 2.0f * dpi : 0.0f, 0.0f, 0.0f, 0.0f);

            Vec4 front = Vec4(0.0f);
            if(isActive)
            {
                if(pill)
                    front = focused ? UIGetToolTheme().Selection : UIGetToolTheme().Field;
                else if(style == UIToolTabStyle_Block)
                    front = c.Body;
            }
            UISetBackground(tab, front, isActive ? front : c.Hover, isActive ? front : c.Hover);

            UI_Interaction interaction = HandleWidgetInteraction(tab);
            if(interaction.pressed)
            {
                if(node.ActiveTab != tabIndex)
                    p.Appear = 0.0f;
                node.ActiveTab  = tabIndex;
                s->FocusedPanel = panelIndex;
            }
            if(interaction.right_clicked)
            {
                s->MenuPanel = panelIndex;
                char menuId[48];
                snprintf(menuId, sizeof(menuId), "##dockmenu%s", s->Name);
                UIOpenPopupAt(menuId, d.Mouse);
            }
            StartDragIfMoved(d, tab, panelIndex);
            if(iconOnly)
                UITooltip(p.Title);

            PushParent(tab);
            {
                if(isActive && !pill)
                {
                    // marker: along the top for a block tab, underneath for an underlined one
                    const f32 thickness = Maths::Max(2.0f, 2.0f * dpi);
                    UI_Widget* line     = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y, Str8Lit(""),
                                                     0x4C494E45ull,
                                                     { SizeKind_Pixels, tab->size.x }, { SizeKind_Pixels, thickness });
                    UIClearBorders(line);
                    line->relative_position                 = Vec2(0.0f, style == UIToolTabStyle_Underline ? height - thickness : 0.0f);
                    line->style_vars[StyleVar_Padding]      = Vec4(0.0f);
                    line->style_vars[StyleVar_CornerRadius] = Vec4(0.0f);
                    Vec4 marker                             = focused ? c.Accent : c.Dim;
                    if(!focused)
                        marker.w = 0.45f;
                    line->style_vars[StyleVar_BackgroundColor] = marker;
                }

                UI_Widget* label = PushWidget(WidgetFlags_DrawText | WidgetFlags_CentreY, iconOnly ? TabIconOf(p.Title) : Str8C(p.Title), 0x4C41424Cull,
                                              { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
                label->style_vars[StyleVar_Padding] = Vec4(0.0f, 2.0f * dpi, 0.0f, 0.0f);
                UISetTextColour(label, isActive ? c.Text : c.Dim);

                if(!iconOnly && !(p.Flags & UIDockPanel_NoClose) && (isActive || interaction.hovering))
                {
                    UIPushStyle(StyleVar_FontSize, label->style_vars[StyleVar_FontSize].x * 0.9f);
                    UIPushStyle(StyleVar_TextColor, c.Dim);
                    UISetNextFlags(WidgetFlags_CentreY);
                    if(UIIconButton(ICON_MDI_CLOSE "###close", 16.0f).clicked)
                        d.ClosePanel = panelIndex;
                    UIPopStyle(StyleVar_TextColor);
                    UIPopStyle(StyleVar_FontSize);
                }
            }
            PopParent(tab);
            return tab->size.x;
        }

        void DrawLeaf(DrawState& d, i32 nodeIndex)
        {
            UIDockSpace* s = d.Space;
            DockNode& node = s->Nodes[nodeIndex];
            const Vec4 r   = node.Rect;

            if(node.TabCount == 0)
            {
                if(node.Central)
                    s->CentralRect = r;
                return;
            }

            const i32 active = Maths::Clamp(node.ActiveTab, 0, node.TabCount - 1);
            node.ActiveTab   = active;
            for(i32 i = 0; i < node.TabCount; i++)
                s->Panels[node.Tabs[i]].Visible = false;

            if(r.z < 4.0f || r.w < 4.0f)
                return;

            if(PointInRect(d.Mouse, r))
                s->MouseOver = true;

            const UIToolTheme& theme = UIGetToolTheme();
            const DockColours& c     = d.Colour;

            // the panel itself: one rounded card that the tab strip and the body both sit on
            {
                UI_Widget* island = BeginRootFloating(UIHashCombine(s->Hash, 0x49534C0000ull + (u64)nodeIndex), r,
                                                      WidgetFlags_DrawBackground | (theme.PanelBorder ? WidgetFlags_DrawBorder : 0), false);
                island->style_vars[StyleVar_BackgroundColor] = c.Body;
                island->style_vars[StyleVar_CornerRadius]    = Vec4(s->EdgeToEdge ? 0.0f : Maths::Min(d.Radius, Maths::Min(r.z, r.w) * 0.5f), 0.0f, 0.0f, 0.0f);
                if(theme.PanelBorder)
                {
                    island->style_vars[StyleVar_Border]      = Vec4(Maths::Max(1.0f, d.DPI), Maths::Max(1.0f, d.DPI), 0.0f, 0.0f);
                    island->style_vars[StyleVar_BorderColor] = c.Border;
                }
                EndFloating();
            }

            DockPanel& front    = s->Panels[node.Tabs[active]];
            const bool showTabs = !((front.Flags & UIDockPanel_NoTabBar) && node.TabCount == 1);
            const f32 tabHeight = showTabs ? Maths::Min(d.TabHeight, r.w) : 0.0f;

            // controls keep clear of the notch / corners; the card itself runs to the edge
            const f32 safeL = Maths::Max(0.0f, s->Rect.x + s->Safe.x - r.x);
            const f32 safeT = showTabs ? Maths::Max(0.0f, s->Rect.y + s->Safe.y - r.y) : 0.0f;
            const f32 safeR = Maths::Max(0.0f, r.x + r.z - (s->Rect.x + s->Rect.z - s->Safe.z));

            if(showTabs)
            {
                const bool block = theme.TabStyle == UIToolTabStyle_Block;
                UI_Widget* strip = BeginRootFloating(UIHashCombine(s->Hash, 0x5354520000ull + (u64)nodeIndex),
                                                     Vec4(r.x + safeL, r.y + safeT, r.z - safeL - safeR, tabHeight),
                                                     WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY | (block ? WidgetFlags_DrawBackground : 0), true);
                strip->style_vars[StyleVar_BackgroundColor] = c.Strip;
                strip->style_vars[StyleVar_CornerRadius]    = Vec4(Maths::Min(d.Radius, tabHeight * 0.5f), 0.0f, 0.0f, 0.0f);
                strip->style_vars[StyleVar_Padding]         = Vec4(block ? 0.0f : 6.0f * d.DPI, 0.0f, 0.0f, 0.0f);
                strip->style_vars[StyleVar_ItemSpacing]     = Vec4((block ? 1.0f : 2.0f) * d.DPI, 0.0f, 0.0f, 0.0f);

                // Not enough room for the names: the icons alone; not enough for those: the strip slides sideways.
                const f32 fontPx    = (s->FontSize > 0.0f ? s->FontSize : theme.FontSize) * d.DPI;
                const f32 stripRoom = Maths::Max(1.0f, r.z - safeL - safeR - (block ? 0.0f : 6.0f * d.DPI) - 32.0f * d.DPI);
                f32 fullW = 0.0f;
                for(i32 i = 0; i < node.TabCount; i++)
                {
                    const DockPanel& tp = s->Panels[node.Tabs[i]];
                    const f32 pad       = 10.0f * d.DPI + 2.0f * d.DPI;
                    fullW += GetStringSize(Str8C((char*)tp.Title), fontPx).x + pad + (i == node.ActiveTab ? 22.0f * d.DPI : 0.0f);
                }
                const bool iconOnly = fullW > stripRoom;

                char scrollId[48];
                snprintf(scrollId, sizeof(scrollId), "##tabscroll%d", nodeIndex);
                UIBeginHScrollArea(scrollId, &node.TabScroll);
                UI_Widget* area = d.UI->parents.Back();
                area->semantic_size[UIAxis_X]                 = { SizeKind_Grow, 1.0f };
                area->flags |= WidgetFlags_CentreChildrenY;
                area->style_vars[StyleVar_ItemSpacing]        = Vec4((block ? 1.0f : 2.0f) * d.DPI, 0.0f, 0.0f, 0.0f);

                f32 x = 0.0f, activeX0 = 0.0f, activeX1 = 0.0f;
                for(i32 i = 0; i < node.TabCount; i++)
                {
                    const f32 w = DrawTab(d, node, i, i == node.ActiveTab, iconOnly);
                    if(i == node.ActiveTab)
                    {
                        activeX0 = x;
                        activeX1 = x + w;
                    }
                    x += w + area->style_vars[StyleVar_ItemSpacing].x;
                }
                UIEndHScrollArea();

                // a tab that just came to the front slides into view
                if(node.TabLastActive != node.ActiveTab)
                {
                    const f32 view = area->size.x;
                    if(activeX1 - node.TabScroll > view)
                        node.TabScroll = activeX1 - view;
                    if(activeX0 < node.TabScroll)
                        node.TabScroll = activeX0;
                    node.TabLastActive = node.ActiveTab;
                }

                // park the front panel on the drawer bar
                UIPushStyle(StyleVar_TextColor, c.Dim);
                if(UIIconButton(ICON_MDI_CHEVRON_DOWN "###todrawer", 20.0f).clicked)
                    d.ToggleDrawerPanel = node.Tabs[node.ActiveTab];
                UIPopStyle(StyleVar_TextColor);
                UITooltip("Move to drawer");
                UISpacer(4.0f);

                EndFloating();
            }

            UI_State* ui = d.UI;
            // a panel that scrolls is rows of controls: those come in from the sides too
            const bool rows = (front.Flags & UIDockPanel_NoScroll) == 0;
            const f32 bodyL = rows ? safeL : 0.0f;
            const f32 bodyR = rows ? safeR : 0.0f;
            DrawPanelBody(d, node.Tabs[node.ActiveTab], &ui->root_parent, ui->root_parent.position,
                          Vec4(r.x + bodyL, r.y + safeT + tabHeight, r.z - bodyL - bodyR, r.w - safeT - tabHeight));
        }

        void DrawSplitter(DrawState& d, i32 nodeIndex, f32 gap)
        {
            UIDockSpace* s = d.Space;
            DockNode& n    = s->Nodes[nodeIndex];
            if(n.Collapsing >= 0)
                return;

            const Vec4 first = s->Nodes[n.Child[0]].Rect;
            const f32 grab   = 3.0f * d.DPI; // wider than the gap
            Vec4 bar;
            if(n.SideBySide)
                bar = Vec4(first.x + first.z - grab, n.Rect.y, gap + grab * 2.0f, n.Rect.w);
            else
                bar = Vec4(n.Rect.x, first.y + first.w - grab, n.Rect.z, gap + grab * 2.0f);

            UI_Widget* widget = BeginRootFloating(UIHashCombine(s->Hash, 0x53504C0000ull + (u64)nodeIndex), bar,
                                                  WidgetFlags_Clickable, false);
            HandleWidgetInteraction(widget);

            // nothing drawn: the pointer shows it can be dragged
            const bool dragging = d.UI->active_widget == widget->hash && d.Held;
            if(dragging || (d.UI->hot_widget == widget->hash && !d.Held))
                UISetCursor(n.SideBySide ? UICursor_ResizeEW : UICursor_ResizeNS);

            if(dragging)
            {
                const f32 total   = n.SideBySide ? n.Rect.z : n.Rect.w;
                const f32 along   = n.SideBySide ? (d.Mouse.x - n.Rect.x) : (d.Mouse.y - n.Rect.y);
                const f32 minimum = Maths::Min(0.45f, (60.0f * d.DPI) / Maths::Max(total, 1.0f));
                n.Ratio           = Maths::Clamp(along / Maths::Max(total, 1.0f), minimum, 1.0f - minimum);
                n.AnimRatio       = n.Ratio;
                s->SplitterActive = true;
                s->MouseOver      = true;
            }
            if(PointInRect(d.Mouse, bar))
                s->MouseOver = true;

            EndFloating();
        }

        void DrawDrawers(DrawState& d, const Vec4& layoutRect, const Vec4& barRect)
        {
            UIDockSpace* s       = d.Space;
            UI_State* ui         = d.UI;
            const DockColours& c = d.Colour;
            const f32 dpi        = d.DPI;

            // ---- the panel that is out (or on its way in / out) ----
            Vec4 shownRect = Vec4(0.0f);
            for(u32 i = 0; i < s->PanelCount; i++)
            {
                DockPanel& p = s->Panels[i];
                if(!p.Open || !p.Drawer || s->Solo == (i32)i)
                {
                    p.DrawerT = 0.0f;
                    continue;
                }

                const f32 target = (s->OpenDrawer == (i32)i) ? 1.0f : 0.0f;
                p.DrawerT += (target - p.DrawerT) * (1.0f - expf(-d.DT * 15.0f));
                if(Maths::Abs(target - p.DrawerT) < 0.003f)
                    p.DrawerT = target;
                if(p.DrawerT <= 0.0f)
                {
                    p.Visible = false;
                    continue;
                }

                const f32 fullHeight = Maths::Clamp(p.DrawerSize * layoutRect.w, Maths::Min(120.0f * dpi, layoutRect.w), layoutRect.w);
                const f32 shown      = floorf(fullHeight * p.DrawerT);
                const f32 header     = floorf(32.0f * dpi);
                if(shown < 2.0f)
                    continue;

                // a window onto the full-height panel: it slides up from behind the bar
                const Vec4 window = Vec4(layoutRect.x, barRect.y - shown, layoutRect.z, shown);
                if(s->OpenDrawer == (i32)i)
                    shownRect = window;
                if(PointInRect(d.Mouse, window))
                    s->MouseOver = true;

                UI_Widget* slide = BeginRootFloating(UIHashCombine(s->Hash, UIHashCombine(p.Hash, 0x445257ull)), window,
                                                     WidgetFlags_DrawBackground | WidgetFlags_Clickable, true);
                // clickable so presses do not fall through to the layout underneath
                // edge to edge there are no cards to lift it off: the same surface as the panels
                const Vec4 sheet = s->EdgeToEdge ? c.Body : UIGetToolTheme().Popup;
                UISetBackground(slide, sheet, sheet, sheet);
                slide->flags |= WidgetFlags_DrawBorder;
                slide->style_vars[StyleVar_Border]            = Vec4(Maths::Max(1.0f, dpi), Maths::Max(1.0f, dpi), 0.0f, 0.0f);
                slide->style_vars[StyleVar_BorderColor]       = c.Border;
                slide->style_vars[StyleVar_HotBorderColor]    = c.Border;
                slide->style_vars[StyleVar_ActiveBorderColor] = c.Border;
                slide->style_vars[StyleVar_CornerRadius]      = Vec4(d.Radius, 0.0f, 0.0f, 0.0f);
                slide->style_vars[StyleVar_ShadowColor]  = Vec4(0.0f, 0.0f, 0.0f, 0.45f);
                slide->style_vars[StyleVar_ShadowOffset] = Vec4(0.0f, -3.0f * dpi, 0.0f, 0.0f);
                slide->style_vars[StyleVar_ShadowBlur]   = Vec4(12.0f * dpi, 0.0f, 0.0f, 0.0f);
                HandleWidgetInteraction(slide);
                const Vec2 slidePosition = Vec2(window.x, window.y);

                // header: title, dock, hide
                UI_Widget* head = BeginFloating(slide, slidePosition, 0x48454144ull, Vec4(window.x + s->Safe.x, window.y, window.z - s->Safe.x - s->Safe.z, header),
                                                WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY, true);
                head->style_vars[StyleVar_Padding]         = Vec4(12.0f * dpi, 0.0f, 0.0f, 0.0f);
                head->style_vars[StyleVar_ItemSpacing]     = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);
                {
                    UI_Widget* title = PushWidget(WidgetFlags_DrawText, Str8C(p.Title), 0x5449544Cull,
                                                  { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
                    title->style_vars[StyleVar_Padding] = Vec4(0.0f, 2.0f * dpi, 0.0f, 0.0f);
                    UISetTextColour(title, c.Text);

                    UIFlexSpacer();
                    if(UIIconButton(ICON_MDI_PIN "###dock", 20.0f).clicked)
                        d.ToggleDrawerPanel = (i32)i;
                    UITooltip("Dock in layout");
                    if(UIIconButton(ICON_MDI_CHEVRON_DOWN "###hide", 20.0f).clicked)
                        s->OpenDrawer = -1;
                }
                EndFloating();

                DrawPanelBody(d, (i32)i, slide, slidePosition,
                              Vec4(window.x + s->Safe.x, window.y + header, window.z - s->Safe.x - s->Safe.z, fullHeight - header));

                // top edge drags to resize
                UI_Widget* grip = BeginFloating(slide, slidePosition, 0x47524950ull, Vec4(window.x, window.y, window.z, 6.0f * dpi),
                                                WidgetFlags_Clickable, false);
                HandleWidgetInteraction(grip);
                if((ui->active_widget == grip->hash && d.Held) || (ui->hot_widget == grip->hash && !d.Held))
                    UISetCursor(UICursor_ResizeNS);
                if(ui->active_widget == grip->hash && d.Held && layoutRect.w > 1.0f)
                {
                    p.DrawerSize      = Maths::Clamp((barRect.y - d.Mouse.y) / layoutRect.w, 0.12f, 0.92f);
                    s->SplitterActive = true;
                }
                EndFloating();

                EndFloating(); // slide
            }

            // ---- the bar ----
            if(barRect.w < 1.0f)
            {
                if(s->OpenDrawer >= 0 && d.Pressed && !ui->OverlayBlocksInput && !s->Dragging && !PointInRect(d.Mouse, shownRect))
                    s->OpenDrawer = -1;
                return;
            }
            // the fill runs under the home indicator; the buttons stop short of it
            {
                UI_Widget* fill = BeginRootFloating(UIHashCombine(s->Hash, 0x44424647ull), barRect, WidgetFlags_DrawBackground, false);
                fill->style_vars[StyleVar_BackgroundColor] = c.Body;
                fill->style_vars[StyleVar_CornerRadius]    = Vec4(s->EdgeToEdge ? 0.0f : Maths::Min(d.Radius, barRect.w * 0.5f), 0.0f, 0.0f, 0.0f);
                EndFloating();
            }
            const Vec4 barInner = Vec4(barRect.x + s->Safe.x, barRect.y, barRect.z - s->Safe.x - s->Safe.z, barRect.w - s->Safe.w);
            UI_Widget* bar = BeginRootFloating(UIHashCombine(s->Hash, 0x44424152ull), barInner,
                                               WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY, true);
            bar->style_vars[StyleVar_Padding]         = Vec4(6.0f * dpi, 0.0f, 0.0f, 0.0f);
            bar->style_vars[StyleVar_ItemSpacing]     = Vec4(2.0f * dpi, 0.0f, 0.0f, 0.0f);
            if(PointInRect(d.Mouse, barRect))
                s->MouseOver = true;

            const f32 pillHeight = floorf(barInner.w - 8.0f * dpi);
            i32 slot             = 0;
            // no room for names: the icon each title starts with
            const bool withLayout = s->Solo >= 0 && s->BarInSolo;
            i32 onBar             = 0;
            for(u32 i = 0; i < s->PanelCount; i++)
                onBar += (s->Panels[i].Open && (s->Panels[i].Drawer || withLayout)) ? 1 : 0;
            const bool iconsOnly = barInner.z < (f32)onBar * 104.0f * dpi + 90.0f * dpi;
            // edge to edge the buttons sit in the middle of the bar, and slide when they do not fit
            const bool centred = s->EdgeToEdge;
            f32 content        = 0.0f;
            const f32 spacing  = 2.0f * dpi;
            if(centred)
            {
                char scrollId[48];
                snprintf(scrollId, sizeof(scrollId), "##dockbarscroll%s", s->Name);
                UIBeginHScrollArea(scrollId, &s->BarScroll);
                d.UI->parents.Back()->style_vars[StyleVar_ItemSpacing] = Vec4(spacing, 0.0f, 0.0f, 0.0f);
                d.UI->parents.Back()->flags |= WidgetFlags_CentreChildrenY;
                const f32 room = barInner.z - 12.0f * dpi;
                if(s->BarContent > 0.0f && s->BarContent < room)
                {
                    UI_Widget* lead = PushWidget(0, Str8Lit(""), UIHashCombine(s->Hash, 0x44424C44ull),
                                                 { SizeKind_Pixels, floorf((room - s->BarContent) * 0.5f) - spacing }, { SizeKind_Pixels, 1.0f });
                    lead->style_vars[StyleVar_Padding] = Vec4(0.0f);
                }
            }
            const f32 pillPad    = (iconsOnly ? Maths::Clamp((barInner.z / (f32)Maths::Max(onBar, 1) - 22.0f * dpi) * 0.5f, (centred ? 11.0f : 5.0f) * dpi, 14.0f * dpi) : 10.0f * dpi);

            // the panels of the layout: one of them has the window
            for(u32 i = 0; i < s->PanelCount && withLayout; i++)
            {
                DockPanel& p = s->Panels[i];
                if(!p.Open || p.Drawer)
                    continue;
                String8 label = Str8C(p.Title);
                if(iconsOnly && (u8)p.Title[0] >= 0xC0)
                    label.size = (u8)p.Title[0] >= 0xF0 ? 4 : ((u8)p.Title[0] >= 0xE0 ? 3 : 2);
                const bool front  = s->Solo == (i32)i;
                UI_Widget* button = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground | WidgetFlags_DrawText,
                                               label, UIHashCombine(p.Hash, 0x4442534Full),
                                               { SizeKind_TextContent, 1.0f }, { SizeKind_Pixels, pillHeight });
                UIClearBorders(button);
                button->TextAlignment                     = UI_Text_Alignment_Center_Y;
                button->style_vars[StyleVar_Padding]      = Vec4(pillPad, 0.0f, 0.0f, 0.0f);
                button->style_vars[StyleVar_CornerRadius] = Vec4(d.WidgetRadius + 2.0f * dpi, 0.0f, 0.0f, 0.0f);
                const Vec4 on                             = UIGetToolTheme().Selection;
                UISetBackground(button, front ? on : Vec4(0.0f), front ? on : c.Hover, on);
                UISetTextColour(button, front ? c.Text : c.Dim);
                content += button->size.x + spacing;
                if(HandleWidgetInteraction(button).clicked)
                    d.SoloPanel = (i32)i;
            }
            for(u32 i = 0; i < s->PanelCount; i++)
            {
                DockPanel& p = s->Panels[i];
                if(!p.Open || !p.Drawer)
                    continue;
                slot++;

                const bool out    = s->OpenDrawer == (i32)i;
                String8 label = Str8C(p.Title);
                if(iconsOnly && (u8)p.Title[0] >= 0xC0)
                    label.size = (u8)p.Title[0] >= 0xF0 ? 4 : ((u8)p.Title[0] >= 0xE0 ? 3 : 2);
                UI_Widget* button = PushWidget(WidgetFlags_Clickable | WidgetFlags_DrawBackground | WidgetFlags_DrawText,
                                               label, UIHashCombine(p.Hash, 0x4442544Eull),
                                               { SizeKind_TextContent, 1.0f }, { SizeKind_Pixels, pillHeight });
                UIClearBorders(button);
                button->TextAlignment                     = UI_Text_Alignment_Center_Y;
                button->style_vars[StyleVar_Padding]      = Vec4(pillPad, 0.0f, 0.0f, 0.0f);
                button->style_vars[StyleVar_CornerRadius] = Vec4(d.WidgetRadius + 2.0f * dpi, 0.0f, 0.0f, 0.0f);
                const Vec4 on                             = UIGetToolTheme().Selection;
                UISetBackground(button, out ? on : Vec4(0.0f), out ? on : c.Hover, on);
                UISetTextColour(button, out ? c.Text : c.Dim);

                content += button->size.x + spacing;
                UI_Interaction interaction = HandleWidgetInteraction(button);
                if(interaction.clicked)
                {
                    s->OpenDrawer   = out ? -1 : (i32)i;
                    s->FocusedPanel = (i32)i;
                    s->LastDrawer   = (i32)i;
                    if(!out)
                        p.Appear = 0.0f;
                }
                if(slot <= 9)
                {
                    char tip[48];
                    snprintf(tip, sizeof(tip), "Alt+%d", slot);
                    UITooltip(tip);
                }
                if(interaction.right_clicked)
                {
                    s->MenuPanel = (i32)i;
                    char menuId[48];
                    snprintf(menuId, sizeof(menuId), "##dockmenu%s", s->Name);
                    UIOpenPopupAt(menuId, d.Mouse);
                }
                StartDragIfMoved(d, button, (i32)i);
            }

            if(centred)
            {
                UIEndHScrollArea();
                s->BarContent = Maths::Max(content - spacing, 0.0f);
            }

            if(slot == 0 && !withLayout && !centred)
            {
                UI_Widget* hint = PushWidget(WidgetFlags_DrawText, Str8Lit("Drop a tab here to keep it as a drawer"), 0x48494E54ull,
                                             { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
                Vec4 faint = c.Dim;
                faint.w    = 0.6f;
                UISetTextColour(hint, faint);
            }

            if(s->BarText[0] && !centred)
            {
                UIFlexSpacer();
                UI_Widget* status = PushWidget(WidgetFlags_DrawText, Str8C(s->BarText), 0x53544154ull,
                                               { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
                status->style_vars[StyleVar_Padding] = Vec4(8.0f * dpi, 2.0f * dpi, 0.0f, 0.0f);
                Vec4 faint = c.Dim;
                faint.w    = 0.8f;
                UISetTextColour(status, faint);
            }
            EndFloating();

            // a press anywhere else puts the drawer away
            if(s->OpenDrawer >= 0 && d.Pressed && !ui->OverlayBlocksInput && !s->Dragging
               && !PointInRect(d.Mouse, shownRect) && !PointInRect(d.Mouse, barRect))
                s->OpenDrawer = -1;
        }

        struct DropTarget
        {
            bool Valid  = false;
            bool Root   = false;
            bool Drawer = false; // onto the drawer bar
            i32 Node   = -1;
            int Zone   = 4; // 0 left, 1 right, 2 top, 3 bottom, 4 centre (tab)
            Vec4 Preview;
        };

        Vec4 ZoneRect(const Vec4& r, int zone, f32 share)
        {
            switch(zone)
            {
            case 0: return Vec4(r.x, r.y, r.z * share, r.w);
            case 1: return Vec4(r.x + r.z * (1.0f - share), r.y, r.z * share, r.w);
            case 2: return Vec4(r.x, r.y, r.z, r.w * share);
            case 3: return Vec4(r.x, r.y + r.w * (1.0f - share), r.z, r.w * share);
            default: return r;
            }
        }

        DropTarget FindDropTarget(DrawState& d, const Vec4& layoutRect)
        {
            UIDockSpace* s = d.Space;
            DropTarget target;
            const DockPanel& dragged = s->Panels[s->DragPanel];

            if(s->BarRect.w > 0.0f && PointInRect(d.Mouse, s->BarRect))
            {
                target.Valid   = !dragged.Drawer;
                target.Drawer  = true;
                target.Preview = s->BarRect;
                return target;
            }

            if(!PointInRect(d.Mouse, layoutRect))
                return target;

            // the outer edges of the whole layout
            const f32 band = 26.0f * d.DPI;
            int rootZone   = -1;
            if(d.Mouse.x - layoutRect.x < band)
                rootZone = 0;
            else if(layoutRect.x + layoutRect.z - d.Mouse.x < band)
                rootZone = 1;
            else if(d.Mouse.y - layoutRect.y < band)
                rootZone = 2;
            else if(layoutRect.y + layoutRect.w - d.Mouse.y < band)
                rootZone = 3;

            if(rootZone >= 0)
            {
                target.Valid   = true;
                target.Root    = true;
                target.Zone    = rootZone;
                target.Preview = ZoneRect(layoutRect, rootZone, 0.22f);
                return target;
            }

            for(i32 i = 0; i < kMaxNodes; i++)
            {
                const DockNode& n = s->Nodes[i];
                if(!n.Used || !IsLeaf(n) || !PointInRect(d.Mouse, n.Rect) || n.Rect.z < 8.0f || n.Rect.w < 8.0f)
                    continue;

                const f32 fx = (d.Mouse.x - n.Rect.x) / n.Rect.z;
                const f32 fy = (d.Mouse.y - n.Rect.y) / n.Rect.w;

                int zone = 4;
                if(!(fx > 0.3f && fx < 0.7f && fy > 0.3f && fy < 0.7f))
                {
                    const f32 distance[4] = { fx, 1.0f - fx, fy, 1.0f - fy };
                    zone                  = 0;
                    for(int k = 1; k < 4; k++)
                        if(distance[k] < distance[zone])
                            zone = k;
                }

                const bool ownLeaf = dragged.Node == i && !dragged.Drawer;
                // on its own in that leaf already: any drop there changes nothing
                if(ownLeaf && (n.TabCount == 1 || zone == 4))
                    return target;
                // the see-through centre takes splits beside it, never tabs
                if(zone == 4 && n.Central && n.TabCount == 0 && s->CentralPassthrough)
                    return target;
                if(zone == 4 && n.TabCount >= kMaxTabs)
                    return target;

                target.Valid   = true;
                target.Node    = i;
                target.Zone    = zone;
                target.Preview = ZoneRect(n.Rect, zone, 0.5f);
                return target;
            }
            return target;
        }

        void ApplyDrop(UIDockSpace* s, const DropTarget& target)
        {
            const i32 panel = s->DragPanel;
            DockPanel& p    = s->Panels[panel];

            if(target.Drawer)
            {
                if(!p.Drawer)
                {
                    DetachPanel(s, panel);
                    p.Drawer      = true;
                    p.DrawerT     = 0.0f;
                    p.Open        = true;
                    s->LastDrawer = panel;
                }
                return;
            }

            if(p.Drawer)
            {
                p.Drawer = false;
                if(s->OpenDrawer == panel)
                    s->OpenDrawer = -1;
            }
            else
                DetachPanel(s, panel);

            i32 leaf = -1;
            if(target.Root)
                leaf = SplitNode(s, s->Root, target.Zone, 0.22f, true);
            else if(target.Zone == 4)
                leaf = target.Node;
            else
                leaf = SplitNode(s, target.Node, target.Zone, 0.5f, true);

            if(leaf < 0)
                leaf = SlotLeaf(s, UsableSlot(s, p.Slot), true);

            AttachPanel(s, panel, leaf);
            p.Open          = true;
            s->FocusedPanel = panel;
        }

        void DrawDrag(DrawState& d, const Vec4& layoutRect)
        {
            UIDockSpace* s = d.Space;
            if(!s->Dragging)
                return;
            if(s->DragPanel < 0 || s->DragPanel >= (i32)s->PanelCount)
            {
                s->Dragging = false;
                return;
            }

            s->MouseOver            = true;
            const DockColours& c    = d.Colour;
            const DropTarget target = FindDropTarget(d, layoutRect);

            if(target.Valid)
            {
                UI_Widget* preview = BeginRootFloating(UIHashCombine(s->Hash, 0x50524556ull), target.Preview,
                                                       WidgetFlags_DrawBackground | WidgetFlags_DrawBorder | WidgetFlags_TopMost, false);
                preview->style_vars[StyleVar_Border]          = Vec4(2.0f * d.DPI, 2.0f * d.DPI, 0.0f, 0.0f);
                preview->style_vars[StyleVar_BorderColor]     = Vec4(c.Accent.x, c.Accent.y, c.Accent.z, 0.9f);
                preview->style_vars[StyleVar_BackgroundColor] = Vec4(c.Accent.x * 0.5f, c.Accent.y * 0.5f, c.Accent.z * 0.5f, 0.55f);
                EndFloating();
            }

            // the tab, following the pointer
            {
                UI_State* ui = d.UI;
                ui->parents.PushBack(&ui->root_parent);
                UI_Widget* ghost = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawText | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | WidgetFlags_TopMost,
                                              Str8C(s->Panels[s->DragPanel].Title), UIHashCombine(s->Hash, 0x47484F53ull),
                                              { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
                UIClearBorders(ghost);
                ghost->relative_position                    = d.Mouse - ui->root_parent.position + Vec2(10.0f * d.DPI, 8.0f * d.DPI);
                ghost->style_vars[StyleVar_Padding]         = Vec4(10.0f * d.DPI, 5.0f * d.DPI, 0.0f, 0.0f);
                ghost->style_vars[StyleVar_CornerRadius]    = Vec4(4.0f * d.DPI, 0.0f, 0.0f, 0.0f);
                ghost->style_vars[StyleVar_BackgroundColor] = Vec4(c.Accent.x, c.Accent.y, c.Accent.z, 0.92f);
                UISetTextColour(ghost, Vec4(1.0f, 1.0f, 1.0f, 1.0f));
                ui->parents.PopBack();
            }

            if(!d.Held)
            {
                if(target.Valid)
                    ApplyDrop(s, target);
                s->Dragging  = false;
                s->DragPanel = -1;
            }
        }

        void OpenPanelInLayout(UIDockSpace* s, i32 panel)
        {
            DockPanel& p = s->Panels[panel];
            if(p.Node >= 0)
                return;
            i32 leaf = p.LastNode;
            if(leaf < 0 || !s->Nodes[leaf].Used || !IsLeaf(s->Nodes[leaf])
               || (s->Nodes[leaf].Central && s->CentralPassthrough))
                leaf = SlotLeaf(s, UsableSlot(s, p.Slot), true);
            AttachPanel(s, panel, leaf);
        }

        void SetPanelDrawer(UIDockSpace* s, i32 panel, bool drawer)
        {
            DockPanel& p = s->Panels[panel];
            if(p.Drawer == drawer)
                return;
            p.Drawer = drawer;
            if(!s->LayoutBuilt || !p.Open)
                return;

            if(drawer)
            {
                DetachPanel(s, panel);
                p.DrawerT = 0.0f;
            }
            else
            {
                if(s->OpenDrawer == panel)
                    s->OpenDrawer = -1;
                OpenPanelInLayout(s, panel);
            }
        }

        void SetPanelOpen(UIDockSpace* s, i32 panel, bool open)
        {
            DockPanel& p = s->Panels[panel];
            if(p.Open == open)
                return;
            p.Open = open;
            if(!s->LayoutBuilt)
            {
                p.DefaultOpen = open;
                return;
            }

            if(!open)
            {
                if(s->OpenDrawer == panel)
                    s->OpenDrawer = -1;
                if(!p.Drawer)
                    DetachPanel(s, panel);
                p.Visible = false;
            }
            else if(!p.Drawer)
            {
                OpenPanelInLayout(s, panel);
            }
        }
    }

    // ---- public ------------------------------------------------------------

    UIDockSpace* UIDockSpaceCreate(const char* name)
    {
        Arena* arena   = ArenaAlloc(sizeof(UIDockSpace) + Kilobytes(4));
        // PushObject does not clear: counts and the node pool must start at zero
        UIDockSpace* s = PushObject(arena, UIDockSpace);
        MemorySet(s, 0, sizeof(UIDockSpace));
        s->Memory      = arena;
        CopyString(s->Name, sizeof(s->Name), name ? name : "dock");
        s->Hash         = StringUtilities::BasicHashFromString(Str8C(s->Name)) ^ 0x444F434B53504143ull;
        s->Root         = -1;
        s->FontSize     = 0.0f;
        s->LastDrawer   = -1;
        s->FocusedPanel = -1;
        s->MenuPanel    = -1;
        s->OpenDrawer   = -1;
        s->DragPanel    = -1;
        s->Solo         = -1;
        for(i32 i = 0; i < UIDockSlot_Count; i++)
            s->SlotNode[i] = -1;
        return s;
    }

    void UIDockSpaceDestroy(UIDockSpace* space)
    {
        if(space)
            ArenaRelease(space->Memory);
    }

    void UIDockSetCentralPassthrough(UIDockSpace* space, bool passthrough)
    {
        if(space)
            space->CentralPassthrough = passthrough;
    }

    void UIDockSetFontSize(UIDockSpace* space, float logicalPx)
    {
        if(space)
            space->FontSize = logicalPx > 0.0f ? Maths::Clamp(logicalPx, 8.0f, 32.0f) : 0.0f;
    }

    void UIDockSetBarText(UIDockSpace* space, const char* text)
    {
        if(space)
            CopyString(space->BarText, sizeof(space->BarText), text);
    }

    void UIDockSetDrawerSize(UIDockSpace* space, const char* name, float fraction)
    {
        if(!space)
            return;
        fraction = Maths::Clamp(fraction, 0.12f, 0.92f);
        for(u32 i = 0; i < space->PanelCount; i++)
            if(!name || strcmp(space->Panels[i].Name, name) == 0)
                space->Panels[i].DrawerSize = fraction;
    }

    void UIDockSetDrawerBarAlwaysVisible(UIDockSpace* space, bool always)
    {
        if(space)
            space->DrawerBarAlways = always;
    }

    void UIDockSetDrawerBarHidden(UIDockSpace* space, bool hidden)
    {
        if(space)
            space->BarHidden = hidden;
    }

    void UIDockSetBarInSolo(UIDockSpace* space, bool show)
    {
        if(space)
            space->BarInSolo = show;
    }

    void UIDockSetSafeInsets(UIDockSpace* space, const Vec4& insets, bool edgeToEdge)
    {
        if(!space)
            return;
        space->Safe       = insets;
        space->EdgeToEdge = edgeToEdge;
    }

    void UIDockSetSolo(UIDockSpace* space, const char* name)
    {
        if(!space)
            return;
        const i32 before = space->Solo;
        space->Solo      = name ? FindPanel(space, name) : -1;
        // back in the layout, the panel that had the window is the tab in front
        if(space->Solo < 0 && before >= 0 && before < (i32)space->PanelCount)
            UIDockFocusPanel(space, space->Panels[before].Name);
        if(space->Solo >= 0)
        {
            SetPanelOpen(space, space->Solo, true);
            space->Panels[space->Solo].Appear = 0.0f;
            space->FocusedPanel               = space->Solo;
            if(space->OpenDrawer == space->Solo)
                space->OpenDrawer = -1;
        }
    }

    const char* UIDockGetSolo(UIDockSpace* space)
    {
        return (space && space->Solo >= 0) ? space->Panels[space->Solo].Name : nullptr;
    }

    const char* UIDockGetFocusedPanel(UIDockSpace* space)
    {
        return (space && space->FocusedPanel >= 0 && space->FocusedPanel < (i32)space->PanelCount) ? space->Panels[space->FocusedPanel].Name : nullptr;
    }

    const char* UIDockGetOpenDrawer(UIDockSpace* space)
    {
        return (space && space->OpenDrawer >= 0) ? space->Panels[space->OpenDrawer].Name : nullptr;
    }

    void UIDockRegisterPanel(UIDockSpace* space, const UIDockPanelDesc& desc)
    {
        if(!space || !desc.Name)
            return;

        i32 index = FindPanel(space, desc.Name);
        if(index < 0)
        {
            if(space->PanelCount >= kMaxPanels)
            {
                LWARN("UIDock: too many panels, '%s' not registered", desc.Name);
                return;
            }
            index        = (i32)space->PanelCount++;
            DockPanel& p = space->Panels[index];
            p            = {};
            CopyString(p.Name, sizeof(p.Name), desc.Name);
            p.Hash          = StringUtilities::BasicHashFromString(Str8C(p.Name));
            p.Node          = -1;
            p.LastNode      = -1;
            p.Open          = desc.Open;
            p.Drawer        = desc.Drawer;
            p.DefaultOpen   = desc.Open;
            p.DefaultDrawer = desc.Drawer;
            p.DrawerSize    = 0.38f;
            p.Slot          = desc.Slot;

            // registered after the layout exists: slot it in now
            if(space->LayoutBuilt && p.Open && !p.Drawer)
                OpenPanelInLayout(space, index);
        }

        DockPanel& p = space->Panels[index];
        CopyString(p.Title, sizeof(p.Title), desc.Title ? desc.Title : desc.Name);
        p.Draw     = desc.Draw;
        p.UserData = desc.UserData;
        p.Flags    = desc.Flags;
    }

    void UIDockSpaceDraw(UIDockSpace* space, const Vec4& rect)
    {
        if(!space || rect.z < 8.0f || rect.w < 8.0f)
            return;

        UIDockSpace* s = space;
        UI_State* ui   = GetUIState();

        if(!s->LayoutBuilt)
            BuildDefaultLayout(s);

        DrawState d;
        d.Space             = s;
        d.UI                = ui;
        d.Colour            = Colours();
        d.DPI               = ui->DPIScale;
        d.DT                = Maths::Clamp(ui->AnimationRateDT / Maths::Max(ui->AnimationRate, 0.001f), 0.0f, 0.1f);
        d.Mouse             = UIMousePosition();
        d.Held              = Input::Get().GetMouseHeld(InputCode::MouseKey::ButtonLeft);
        d.Pressed           = Input::Get().GetMouseClicked(InputCode::MouseKey::ButtonLeft);
        d.ClosePanel        = -1;
        d.ToggleDrawerPanel = -1;
        d.SoloPanel         = -1;

        const UIToolTheme& theme = UIGetToolTheme();
        const f32 density        = Maths::Clamp(theme.Density, 0.6f, 1.8f);
        d.Radius                 = theme.PanelRounding * d.DPI;
        d.WidgetRadius           = theme.WidgetRounding * d.DPI;
        d.TabHeight              = floorf(theme.TabHeight * density * d.DPI);

        s->Rect           = rect;
        s->MouseOver      = false;
        s->SplitterActive = false;
        s->CentralRect    = Vec4(0.0f);

        UIPushStyle(StyleVar_FontSize, (s->FontSize > 0.0f ? s->FontSize : theme.FontSize) * d.DPI);

        // ---- ease the splits; drop leaves that have finished closing ----
        const f32 blend = 1.0f - expf(-d.DT * 16.0f);
        for(i32 i = 0; i < kMaxNodes; i++)
        {
            DockNode& n = s->Nodes[i];
            if(!n.Used || IsLeaf(n))
                continue;
            const f32 target = n.Collapsing == 0 ? 0.0f : (n.Collapsing == 1 ? 1.0f : n.Ratio);
            n.AnimRatio += (target - n.AnimRatio) * blend;
            if(Maths::Abs(target - n.AnimRatio) < 0.002f)
                n.AnimRatio = target;
        }
        // an empty leaf nobody is closing yet (its sibling was the one that went)
        for(i32 i = 0; i < kMaxNodes; i++)
        {
            DockNode& n = s->Nodes[i];
            if(!n.Used || !IsLeaf(n) || n.TabCount > 0 || n.Central || n.Parent < 0)
                continue;
            DockNode& parent = s->Nodes[n.Parent];
            if(parent.Collapsing < 0)
                parent.Collapsing = parent.Child[0] == i ? 0 : 1;
        }
        for(i32 i = 0; i < kMaxNodes; i++)
        {
            DockNode& n = s->Nodes[i];
            if(!n.Used || IsLeaf(n) || n.Collapsing < 0)
                continue;
            const i32 closing = n.Child[n.Collapsing];
            if(!IsLeaf(s->Nodes[closing]) || s->Nodes[closing].TabCount > 0)
            {
                n.Collapsing = -1;
                continue;
            }
            const f32 target = n.Collapsing == 0 ? 0.0f : 1.0f;
            if(n.AnimRatio == target)
                RemoveLeaf(s, closing);
        }

        // ---- lay out ----
        bool anyDrawer = false;
        for(u32 i = 0; i < s->PanelCount; i++)
            anyDrawer |= (s->Panels[i].Open && s->Panels[i].Drawer);
        if(!anyDrawer)
            s->OpenDrawer = -1;

        // Panels sit as separate cards on the canvas: a margin round the lot, a gap between each.
        // Over a running game there is no canvas, so they go edge to edge.
        if(s->Solo >= (i32)s->PanelCount || (s->Solo >= 0 && !s->Panels[s->Solo].Open))
            s->Solo = -1;
        const bool solo     = s->Solo >= 0;
        const bool showBar  = anyDrawer || s->DrawerBarAlways;
        const bool drawBar  = (showBar && !s->BarHidden && !solo) || (solo && s->BarInSolo && !s->BarHidden);
        const f32 margin    = (s->CentralPassthrough || s->EdgeToEdge || solo) ? 0.0f : floorf(theme.OuterMargin * d.DPI);
        const f32 gap       = Maths::Max(2.0f, floorf(theme.PanelGap * d.DPI));
        const f32 barHeight = drawBar ? floorf(30.0f * density * d.DPI) + s->Safe.w : 0.0f;
        const f32 barGap    = drawBar ? gap : 0.0f;
        const Vec4 inner    = Vec4(rect.x + margin, rect.y + margin, rect.z - margin * 2.0f, rect.w - margin * 2.0f);
        const Vec4 layout   = Vec4(inner.x, inner.y, inner.z, inner.w - barHeight - barGap);
        const Vec4 barRect  = Vec4(inner.x, inner.y + inner.w - barHeight, inner.z, barHeight);
        s->BarRect          = drawBar ? barRect : Vec4(0.0f);

        ComputeRects(s, s->Root, layout, gap);

        // something solid behind the dividers (not when the centre shows the game)
        if(!s->CentralPassthrough)
        {
            UI_Widget* backdrop = BeginRootFloating(UIHashCombine(s->Hash, 0x4241434Bull), rect, WidgetFlags_DrawBackground, false);
            backdrop->style_vars[StyleVar_BackgroundColor] = d.Colour.Gap;
            EndFloating();
        }

        if(solo)
        {
            // one panel and nothing else: no tabs, no dividers
            for(u32 i = 0; i < s->PanelCount; i++)
                if(!s->Panels[i].Drawer)
                    s->Panels[i].Visible = false;
            UI_Widget* fill = BeginRootFloating(UIHashCombine(s->Hash, 0x534F4C4Full), rect, WidgetFlags_DrawBackground, false);
            fill->style_vars[StyleVar_BackgroundColor] = d.Colour.Body;
            EndFloating();
            if(PointInRect(d.Mouse, rect))
                s->MouseOver = true;
            // down to the bar, when it is up
            Vec4 body = Vec4(rect.x, rect.y, rect.z, rect.w - barHeight);
            if(!(s->Panels[s->Solo].Flags & UIDockPanel_FullBleed))
                body = Vec4(body.x + s->Safe.x, body.y + s->Safe.y, body.z - s->Safe.x - s->Safe.z, body.w - s->Safe.y);
            DrawPanelBody(d, s->Solo, &ui->root_parent, ui->root_parent.position, body);
        }
        else
        {
            for(i32 i = 0; i < kMaxNodes; i++)
                if(s->Nodes[i].Used && IsLeaf(s->Nodes[i]))
                    DrawLeaf(d, i);

            for(i32 i = 0; i < kMaxNodes; i++)
                if(s->Nodes[i].Used && !IsLeaf(s->Nodes[i]))
                    DrawSplitter(d, i, gap);
        }

        if(showBar || drawBar)
        {
            DrawDrawers(d, layout, barRect);

            Input& input    = Input::Get();
            const bool ctrl = input.GetKeyHeldRaw(InputCode::Key::LeftControl) || input.GetKeyHeldRaw(InputCode::Key::RightControl)
                || input.GetKeyHeldRaw(InputCode::Key::LeftSuper) || input.GetKeyHeldRaw(InputCode::Key::RightSuper);
            const bool alt = input.GetKeyHeldRaw(InputCode::Key::LeftAlt) || input.GetKeyHeldRaw(InputCode::Key::RightAlt);

            if(ui->FocusedTextInput == 0)
            {
                // Ctrl / Cmd + Space: the drawer used last, in and out
                if(ctrl && input.GetKeyPressedRaw(InputCode::Key::Space))
                {
                    if(s->OpenDrawer >= 0)
                        s->OpenDrawer = -1;
                    else
                    {
                        i32 pick = -1;
                        if(s->LastDrawer >= 0 && s->LastDrawer < (i32)s->PanelCount
                           && s->Panels[s->LastDrawer].Open && s->Panels[s->LastDrawer].Drawer)
                            pick = s->LastDrawer;
                        for(u32 i = 0; i < s->PanelCount && pick < 0; i++)
                            if(s->Panels[i].Open && s->Panels[i].Drawer)
                                pick = (i32)i;
                        s->OpenDrawer = pick;
                        if(pick >= 0)
                            s->Panels[pick].Appear = 0.0f;
                    }
                }

                // Alt + 1..9: the drawer in that place on the bar
                if(alt && !ctrl)
                {
                    i32 slot = 0;
                    for(u32 i = 0; i < s->PanelCount; i++)
                    {
                        if(!s->Panels[i].Open || !s->Panels[i].Drawer)
                            continue;
                        if(slot < 9 && input.GetKeyPressedRaw((InputCode::Key)((i32)InputCode::Key::D1 + slot)))
                        {
                            s->OpenDrawer = s->OpenDrawer == (i32)i ? -1 : (i32)i;
                            s->LastDrawer = (i32)i;
                            s->Panels[i].Appear = 0.0f;
                        }
                        slot++;
                    }
                }

                if(s->OpenDrawer >= 0 && input.GetKeyPressedRaw(InputCode::Key::Escape))
                    s->OpenDrawer = -1;
            }

            if(s->OpenDrawer >= 0)
                s->LastDrawer = s->OpenDrawer;
        }

        // ---- tab / drawer button menu ----
        bool resetLayout = false;
        {
            char menuId[48];
            snprintf(menuId, sizeof(menuId), "##dockmenu%s", s->Name);
            if(UIBeginPopup(menuId))
            {
                if(s->MenuPanel >= 0 && s->MenuPanel < (i32)s->PanelCount)
                {
                    DockPanel& p = s->Panels[s->MenuPanel];
                    if(!(p.Flags & UIDockPanel_NoClose) && UIMenuItem("Close"))
                        d.ClosePanel = s->MenuPanel;
                    if(UIMenuItem(p.Drawer ? "Dock in Layout" : "Move to Drawer"))
                        d.ToggleDrawerPanel = s->MenuPanel;
                    UIMenuSeparator();
                }
                if(UIMenuItem("Reset Layout"))
                    resetLayout = true;
                UIEndPopup();
            }
        }

        DrawDrag(d, layout);

        // ---- changes asked for while drawing ----
        if(d.SoloPanel >= 0 && s->Solo >= 0)
        {
            s->Solo                       = d.SoloPanel;
            s->FocusedPanel               = d.SoloPanel;
            s->OpenDrawer                 = -1;
            s->Panels[d.SoloPanel].Appear = 0.0f;
        }
        if(d.ClosePanel >= 0)
            SetPanelOpen(s, d.ClosePanel, false);
        if(d.ToggleDrawerPanel >= 0)
            SetPanelDrawer(s, d.ToggleDrawerPanel, !s->Panels[d.ToggleDrawerPanel].Drawer);
        if(resetLayout)
            BuildDefaultLayout(s);

        UIPopStyle(StyleVar_FontSize);
    }

    void UIDockSetPanelOpen(UIDockSpace* space, const char* name, bool open)
    {
        const i32 index = FindPanel(space, name);
        if(index >= 0)
            SetPanelOpen(space, index, open);
    }

    bool UIDockIsPanelOpen(UIDockSpace* space, const char* name)
    {
        const i32 index = FindPanel(space, name);
        return index >= 0 && space->Panels[index].Open;
    }

    void UIDockFocusPanel(UIDockSpace* space, const char* name)
    {
        const i32 index = FindPanel(space, name);
        if(index < 0)
            return;
        SetPanelOpen(space, index, true);

        DockPanel& p        = space->Panels[index];
        space->FocusedPanel = index;
        // focus mode shows one panel: this one now
        if(space->Solo >= 0 && space->Solo != index && !p.Drawer)
        {
            space->Solo = index;
            p.Appear    = 0.0f;
        }
        if(p.Drawer)
        {
            space->OpenDrawer = index;
            return;
        }
        if(p.Node >= 0)
        {
            DockNode& n = space->Nodes[p.Node];
            for(i32 i = 0; i < n.TabCount; i++)
                if(n.Tabs[i] == index)
                    n.ActiveTab = i;
        }
    }

    bool UIDockIsPanelVisible(UIDockSpace* space, const char* name)
    {
        const i32 index = FindPanel(space, name);
        return index >= 0 && space->Panels[index].Open && space->Panels[index].Visible;
    }

    bool UIDockGetPanelRect(UIDockSpace* space, const char* name, Vec4* outRect)
    {
        const i32 index = FindPanel(space, name);
        if(index < 0 || !space->Panels[index].Open || !space->Panels[index].Visible)
            return false;
        if(outRect)
            *outRect = space->Panels[index].BodyRect;
        return true;
    }

    bool UIDockIsPanelDrawer(UIDockSpace* space, const char* name)
    {
        const i32 index = FindPanel(space, name);
        return index >= 0 && space->Panels[index].Drawer;
    }

    Vec4 UIDockGetCentralRect(UIDockSpace* space)
    {
        return space ? space->CentralRect : Vec4(0.0f);
    }

    void UIDockSetPanelDrawer(UIDockSpace* space, const char* name, bool drawer)
    {
        const i32 index = FindPanel(space, name);
        if(index < 0)
            return;
        if(!space->LayoutBuilt)
            space->Panels[index].DefaultDrawer = drawer;
        SetPanelDrawer(space, index, drawer);
    }

    void UIDockToggleDrawer(UIDockSpace* space, const char* name)
    {
        const i32 index = FindPanel(space, name);
        if(index < 0 || !space->Panels[index].Drawer)
            return;
        SetPanelOpen(space, index, true);
        space->OpenDrawer = space->OpenDrawer == index ? -1 : index;
    }

    u32 UIDockGetPanelCount(UIDockSpace* space)
    {
        return space ? space->PanelCount : 0;
    }

    const char* UIDockGetPanelName(UIDockSpace* space, u32 index)
    {
        return (space && index < space->PanelCount) ? space->Panels[index].Name : "";
    }

    const char* UIDockGetPanelTitle(UIDockSpace* space, u32 index)
    {
        return (space && index < space->PanelCount) ? space->Panels[index].Title : "";
    }

    bool UIDockIsMouseOver(UIDockSpace* space)
    {
        return space && space->MouseOver;
    }

    bool UIDockIsDragging(UIDockSpace* space)
    {
        return space && (space->Dragging || space->SplitterActive);
    }

    void UIDockResetLayout(UIDockSpace* space)
    {
        if(space)
            BuildDefaultLayout(space);
    }

    // ---- saved layouts -----------------------------------------------------

    namespace
    {
        struct NodeSave
        {
            int Parent      = -1;
            int Child0      = -1;
            int Child1      = -1;
            bool SideBySide = false;
            bool Central    = false;
            float Ratio     = 0.5f;
            int ActiveTab   = 0;
            std::vector<std::string> Tabs;

            template <typename Archive>
            void serialize(Archive& archive)
            {
                archive(cereal::make_nvp("Parent", Parent), cereal::make_nvp("Child0", Child0), cereal::make_nvp("Child1", Child1),
                        cereal::make_nvp("SideBySide", SideBySide), cereal::make_nvp("Central", Central), cereal::make_nvp("Ratio", Ratio),
                        cereal::make_nvp("ActiveTab", ActiveTab), cereal::make_nvp("Tabs", Tabs));
            }
        };

        struct PanelSave
        {
            std::string Name;
            bool Open        = true;
            bool Drawer      = false;
            float DrawerSize = 0.38f;

            template <typename Archive>
            void serialize(Archive& archive)
            {
                archive(cereal::make_nvp("Name", Name), cereal::make_nvp("Open", Open), cereal::make_nvp("Drawer", Drawer),
                        cereal::make_nvp("DrawerSize", DrawerSize));
            }
        };

        struct LayoutSave
        {
            int Version = 1;
            int Root    = 0;
            std::vector<NodeSave> Nodes;
            std::vector<PanelSave> Panels;
            std::vector<int> Slots;

            template <typename Archive>
            void serialize(Archive& archive)
            {
                archive(cereal::make_nvp("Version", Version), cereal::make_nvp("Root", Root), cereal::make_nvp("Nodes", Nodes),
                        cereal::make_nvp("Panels", Panels), cereal::make_nvp("Slots", Slots));
            }
        };
    }

    bool UIDockSaveLayout(UIDockSpace* space, const String8& path)
    {
        if(!space || !space->LayoutBuilt || path.size == 0)
            return false;
        UIDockSpace* s = space;

        // nodes are written packed; this maps a live index to its place in the file
        int remap[kMaxNodes];
        int count = 0;
        for(i32 i = 0; i < kMaxNodes; i++)
            remap[i] = s->Nodes[i].Used ? count++ : -1;

        LayoutSave save;
        save.Root = s->Root >= 0 ? remap[s->Root] : 0;
        save.Nodes.resize((size_t)count);
        for(i32 i = 0; i < kMaxNodes; i++)
        {
            const DockNode& n = s->Nodes[i];
            if(!n.Used)
                continue;
            NodeSave& out  = save.Nodes[(size_t)remap[i]];
            out.Parent     = n.Parent >= 0 ? remap[n.Parent] : -1;
            out.Child0     = n.Child[0] >= 0 ? remap[n.Child[0]] : -1;
            out.Child1     = n.Child[1] >= 0 ? remap[n.Child[1]] : -1;
            out.SideBySide = n.SideBySide;
            out.Central    = n.Central;
            out.Ratio      = n.Ratio;
            out.ActiveTab  = n.ActiveTab;
            for(i32 t = 0; t < n.TabCount; t++)
                out.Tabs.push_back(s->Panels[n.Tabs[t]].Name);
        }

        for(u32 i = 0; i < s->PanelCount; i++)
        {
            const DockPanel& p = s->Panels[i];
            PanelSave out;
            out.Name       = p.Name;
            out.Open       = p.Open;
            out.Drawer     = p.Drawer;
            out.DrawerSize = p.DrawerSize;
            save.Panels.push_back(out);
        }

        for(i32 i = 0; i < UIDockSlot_Count; i++)
            save.Slots.push_back(s->SlotNode[i] >= 0 && s->Nodes[s->SlotNode[i]].Used ? remap[s->SlotNode[i]] : -1);

        std::stringstream storage;
        {
            cereal::JSONOutputArchive output { storage };
            output(cereal::make_nvp("DockLayout", save));
        }
        const std::string text = storage.str();
        return FileSystem::WriteTextFile(path, Str8((u8*)text.data(), text.size()));
    }

    bool UIDockLoadLayout(UIDockSpace* space, const String8& path)
    {
        if(!space || path.size == 0 || !FileSystem::FileExists(path))
            return false;
        UIDockSpace* s = space;

        ArenaTemp scratch = ScratchBegin(0, 0);
        String8 text      = FileSystem::ReadTextFile(scratch.arena, path);
        LayoutSave save;
        bool parsed = false;
        if(text.size > 0)
        {
            try
            {
                std::stringstream storage(std::string((const char*)text.str, (size_t)text.size));
                cereal::JSONInputArchive input(storage);
                input(cereal::make_nvp("DockLayout", save));
                parsed = true;
            }
            catch(...)
            {
                LWARN("UIDock: could not read layout '%.*s'", (int)path.size, (const char*)path.str);
            }
        }
        ScratchEnd(scratch);

        const int count = (int)save.Nodes.size();
        if(!parsed || save.Version != 1 || count <= 0 || count > kMaxNodes || save.Root < 0 || save.Root >= count)
            return false;

        // a file that does not describe a tree is ignored outright
        for(int i = 0; i < count; i++)
        {
            const NodeSave& n = save.Nodes[(size_t)i];
            const bool leaf   = n.Child0 < 0;
            if(n.Parent < -1 || n.Parent >= count)
                return false;
            if(leaf != (n.Child1 < 0))
                return false;
            if(!leaf && (n.Child0 >= count || n.Child1 >= count || n.Child0 == n.Child1 || n.Child0 == i || n.Child1 == i))
                return false;
            if(!leaf && (save.Nodes[(size_t)n.Child0].Parent != i || save.Nodes[(size_t)n.Child1].Parent != i))
                return false;
            if((i == save.Root) != (n.Parent == -1))
                return false;
        }

        // every node has to hang off the root: count what a walk down from it reaches
        {
            int stack[kMaxNodes];
            int depth   = 0;
            int reached = 0;
            stack[depth++] = save.Root;
            while(depth > 0 && reached <= count)
            {
                const NodeSave& n = save.Nodes[(size_t)stack[--depth]];
                reached++;
                if(n.Child0 >= 0 && depth + 2 <= kMaxNodes)
                {
                    stack[depth++] = n.Child0;
                    stack[depth++] = n.Child1;
                }
            }
            if(reached != count)
                return false;
        }

        // start from the defaults so every panel has somewhere to go, then lay the saved tree over it
        BuildDefaultLayout(s);
        for(i32 i = 0; i < kMaxNodes; i++)
            s->Nodes[i].Used = false;
        for(i32 i = 0; i < UIDockSlot_Count; i++)
            s->SlotNode[i] = -1;
        for(u32 i = 0; i < s->PanelCount; i++)
        {
            s->Panels[i].Node     = -1;
            s->Panels[i].LastNode = -1;
        }

        i32 central = -1;
        for(int i = 0; i < count; i++)
        {
            const NodeSave& in = save.Nodes[(size_t)i];
            DockNode& n        = s->Nodes[i];
            n                  = {};
            n.Used             = true;
            n.Parent           = in.Parent;
            n.Child[0]         = in.Child0;
            n.Child[1]         = in.Child1;
            n.SideBySide       = in.SideBySide;
            n.Central          = in.Central && in.Child0 < 0 && central < 0;
            n.Ratio            = Maths::Clamp(in.Ratio, 0.03f, 0.97f);
            n.AnimRatio        = n.Ratio;
            n.Collapsing       = -1;
            if(n.Central)
                central = i;
        }
        s->Root = save.Root;

        for(const PanelSave& in : save.Panels)
        {
            const i32 index = FindPanel(s, in.Name.c_str());
            if(index < 0)
                continue;
            DockPanel& p = s->Panels[index];
            p.Open       = in.Open;
            p.Drawer     = in.Drawer;
            p.DrawerSize = Maths::Clamp(in.DrawerSize, 0.12f, 0.92f);
        }

        for(int i = 0; i < count; i++)
        {
            const NodeSave& in = save.Nodes[(size_t)i];
            if(in.Child0 >= 0)
                continue;
            for(const std::string& name : in.Tabs)
            {
                const i32 index = FindPanel(s, name.c_str());
                if(index < 0 || !s->Panels[index].Open || s->Panels[index].Drawer || s->Panels[index].Node >= 0)
                    continue;
                AttachPanel(s, index, i);
            }
            s->Nodes[i].ActiveTab = Maths::Clamp(in.ActiveTab, 0, Maths::Max(0, s->Nodes[i].TabCount - 1));
        }

        // a layout needs a centre: take the first leaf if the file did not mark one
        if(central < 0)
        {
            for(int i = 0; i < count && central < 0; i++)
                if(IsLeaf(s->Nodes[i]))
                    central = i;
            s->Nodes[central].Central = true;
        }
        s->SlotNode[UIDockSlot_Centre] = central;

        if(save.Slots.size() == (size_t)UIDockSlot_Count)
        {
            for(i32 i = 0; i < UIDockSlot_Count; i++)
            {
                const int node = save.Slots[(size_t)i];
                if(i != UIDockSlot_Centre && node >= 0 && node < count && IsLeaf(s->Nodes[node]))
                    s->SlotNode[i] = node;
            }
        }

        // leaves the file left empty (their panels are gone or closed) fold away
        for(i32 i = 0; i < count; i++)
        {
            DockNode& n = s->Nodes[i];
            if(n.Used && IsLeaf(n) && n.TabCount == 0 && !n.Central && n.Parent >= 0)
            {
                DockNode& parent  = s->Nodes[n.Parent];
                parent.Collapsing = parent.Child[0] == i ? 0 : 1;
                parent.AnimRatio  = parent.Collapsing == 0 ? 0.0f : 1.0f;
            }
        }

        // panels the file has never heard of (added since it was saved)
        for(u32 i = 0; i < s->PanelCount; i++)
        {
            DockPanel& p = s->Panels[i];
            if(p.Open && !p.Drawer && p.Node < 0)
                AttachPanel(s, (i32)i, SlotLeaf(s, UsableSlot(s, p.Slot), false));
        }

        s->OpenDrawer  = -1;
        s->LayoutBuilt = true;
        return true;
    }
}
