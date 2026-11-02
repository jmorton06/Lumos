#pragma once
#include "Graphics/UI.h"

// Building blocks shared by the UI source files. Not for game code.
namespace Lumos
{
    UI_Widget* PushWidget(u32 flags, const String8& text, u64 hash, UI_Size semantic_size_x, UI_Size semantic_size_y);
    UI_Interaction HandleWidgetInteraction(UI_Widget* widget);

    UI_Widget* GetCurrentParent();
    void PushParent(UI_Widget* widget);
    void PopParent(UI_Widget* widget);

    String8 HandleUIString(const char* str, u64* out_hash);
    u64 UIHashCombine(u64 a, u64 b);
    void UIAdoptText(UI_Widget* widget, const String8& src);
    bool UIThemeIsDark();
    // A finger held still for half a second this frame (touch only): the stand-in for a right click.
    bool UILongPressed();

    // Handwriting (Apple Pencil Scribble). A field registers itself each frame it is built so
    // the platform knows where it can be written into; `number` = a drag field, which has to be
    // switched to typing first - it asks with UIScribbleTake.
    void UIScribbleRegister(UI_Widget* widget, u64 hash, bool number);
    bool UIScribbleTake(u64 hash);

    // Hooks from UIBeginBuild / UIEndBuild into UIWidgets.cpp.
    void UIWidgetsBeginBuild();
    void UIWidgetsEndBuild();

    // Clears the three border colours and the shadow: a clickable is always hot
    // under the cursor and would otherwise be tinted with the theme's hot border.
    inline void UIClearBorders(UI_Widget* widget)
    {
        widget->style_vars[StyleVar_Border]            = Vec4(0.0f);
        widget->style_vars[StyleVar_BorderColor]       = Vec4(0.0f);
        widget->style_vars[StyleVar_HotBorderColor]    = Vec4(0.0f);
        widget->style_vars[StyleVar_ActiveBorderColor] = Vec4(0.0f);
        widget->style_vars[StyleVar_ShadowColor]       = Vec4(0.0f);
    }

    inline void UISetBackground(UI_Widget* widget, const Vec4& idle, const Vec4& hot, const Vec4& active)
    {
        widget->style_vars[StyleVar_BackgroundColor]       = idle;
        widget->style_vars[StyleVar_HotBackgroundColor]    = hot;
        widget->style_vars[StyleVar_ActiveBackgroundColor] = active;
    }

    inline void UISetTextColour(UI_Widget* widget, const Vec4& colour)
    {
        widget->style_vars[StyleVar_TextColor]       = colour;
        widget->style_vars[StyleVar_HotTextColor]    = colour;
        widget->style_vars[StyleVar_ActiveTextColor] = colour;
    }
}
