#pragma once
#include "Core/Core.h"
#include "Core/String.h"
#include "Maths/Vector4.h"

namespace Lumos
{
    enum UIToolTabStyle : i32
    {
        UIToolTabStyle_Pill = 0,  // rounded chip behind the front tab
        UIToolTabStyle_Underline, // line under the front tab
        UIToolTabStyle_Block,     // front tab joins the panel below it
        UIToolTabStyle_Count
    };

    // Look of the tools UI (dock, panels, menus, property rows). One shared
    // theme: presets fill it in, and every field can be edited live.
    struct UIToolTheme
    {
        char Name[32];

        Vec4 Canvas;   // behind the panels
        Vec4 Panel;    // panel body
        Vec4 Header;   // tab strips and bars when they are drawn as their own band
        Vec4 Popup;    // menus, popups, drawers
        Vec4 Field;    // text fields, number fields, checkboxes
        Vec4 Hover;    // laid over whatever is under the pointer
        Vec4 Border;
        Vec4 Text;
        Vec4 TextDim;
        Vec4 Accent;
        Vec4 AccentText; // text on an accent fill
        Vec4 Selection;  // selected rows
        Vec4 Warning;
        Vec4 Error;
        Vec4 Success;
        Vec4 AxisX;
        Vec4 AxisY;
        Vec4 AxisZ;

        // logical px
        f32 FontSize;
        f32 PanelRounding;
        f32 WidgetRounding;
        f32 PanelGap;    // space between panels (0 = panels touch)
        f32 OuterMargin; // space round the whole layout
        f32 TabHeight;
        f32 Density; // multiplies paddings and row heights (1 = normal)

        i32 TabStyle;
        bool PanelBorder;
        bool FloatingToolbars; // viewport toolbars float over the view instead of taking a strip
        bool Dark;
    };

    UIToolTheme& UIGetToolTheme();

    i32 UIGetToolThemePresetCount();
    const char* UIGetToolThemePresetName(i32 preset);
    // Replaces the whole theme with a preset.
    void UISetToolThemePreset(i32 preset);
    // Preset last applied, -1 once it has been edited away from one.
    i32 UIGetToolThemePreset();
    void UIMarkToolThemeEdited();

    // Black and white, square, no gaps: laid over the theme while on (the theme itself is kept and is what gets saved).
    void UISetToolThemePlain(bool plain);
    bool UIIsToolThemePlain();

    // Rebuilds every colour from the four that define a theme.
    void UIDeriveToolTheme(UIToolTheme& theme, const Vec4& canvas, const Vec4& panel, const Vec4& text, const Vec4& accent);

    // JSON (cereal).
    bool UISaveToolTheme(const String8& path);
    bool UILoadToolTheme(const String8& path);

    // Applies the parts of the theme the core widgets read (dark / light, accent) to the current UI.
    void UIApplyToolThemeToContext();
}
