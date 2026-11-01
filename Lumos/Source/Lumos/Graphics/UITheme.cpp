#include "Precompiled.h"
#include "UITheme.h"
#include "UI.h"

#include "Core/OS/FileSystem.h"
#include "Core/OS/Memory.h"
#include "Maths/MathsUtilities.h"

#include <cereal/archives/json.hpp>
#include <sstream>

namespace Lumos
{
    namespace
    {
        struct Preset
        {
            const char* Name;
            Vec4 Canvas;
            Vec4 Panel;
            Vec4 Text;
            Vec4 Accent;
            f32 PanelRounding;
            f32 PanelGap;
            i32 TabStyle;
        };

        Vec4 RGB(int r, int g, int b)
        {
            return Vec4((f32)r / 255.0f, (f32)g / 255.0f, (f32)b / 255.0f, 1.0f);
        }

        // The first three are the engine's own; the rest follow the ImGui editor themes.
        const Preset kPresets[] = {
            { "Lumos", RGB(24, 22, 19), RGB(40, 36, 31), RGB(234, 228, 218), RGB(233, 150, 64), 10.0f, 6.0f, UIToolTabStyle_Pill },
            { "Midnight", RGB(13, 15, 20), RGB(24, 27, 35), RGB(226, 230, 238), RGB(92, 148, 255), 10.0f, 6.0f, UIToolTabStyle_Pill },
            { "Paper", RGB(226, 224, 218), RGB(250, 249, 246), RGB(38, 36, 33), RGB(214, 104, 54), 10.0f, 6.0f, UIToolTabStyle_Pill },
            { "Dark", RGB(40, 42, 54), RGB(52, 54, 64), RGB(200, 200, 200), RGB(155, 130, 207), 4.0f, 2.0f, UIToolTabStyle_Block },
            { "Dracula", RGB(33, 34, 44), RGB(40, 42, 54), RGB(244, 244, 244), RGB(145, 111, 186), 6.0f, 4.0f, UIToolTabStyle_Underline },
            { "Black", RGB(0, 0, 0), RGB(26, 26, 26), RGB(255, 255, 255), RGB(84, 171, 219), 0.0f, 1.0f, UIToolTabStyle_Underline },
            { "Grey", RGB(48, 48, 48), RGB(64, 64, 64), RGB(255, 255, 255), RGB(166, 166, 166), 3.0f, 2.0f, UIToolTabStyle_Block },
            { "Light", RGB(214, 214, 214), RGB(240, 240, 240), RGB(0, 0, 0), RGB(66, 150, 250), 4.0f, 2.0f, UIToolTabStyle_Block },
            { "Cherry", RGB(24, 26, 32), RGB(33, 36, 43), RGB(242, 245, 250), RGB(181, 56, 69), 6.0f, 4.0f, UIToolTabStyle_Pill },
            { "Blue", RGB(33, 46, 60), RGB(44, 62, 80), RGB(236, 240, 241), RGB(41, 128, 185), 4.0f, 3.0f, UIToolTabStyle_Underline },
            { "Cinder", RGB(24, 26, 32), RGB(33, 36, 43), RGB(242, 245, 250), RGB(235, 46, 74), 4.0f, 3.0f, UIToolTabStyle_Underline },
            { "Classic", RGB(20, 20, 28), RGB(36, 36, 48), RGB(230, 230, 230), RGB(102, 102, 178), 0.0f, 1.0f, UIToolTabStyle_Block },
            { "Classic Dark", RGB(15, 15, 15), RGB(30, 30, 30), RGB(255, 255, 255), RGB(66, 150, 250), 0.0f, 1.0f, UIToolTabStyle_Block },
            { "Classic Light", RGB(210, 210, 210), RGB(240, 240, 240), RGB(0, 0, 0), RGB(66, 150, 250), 0.0f, 1.0f, UIToolTabStyle_Block },
            { "Cosy", RGB(32, 33, 39), RGB(42, 44, 54), RGB(230, 175, 137), RGB(47, 179, 135), 8.0f, 5.0f, UIToolTabStyle_Pill },
            { "Apple Light", RGB(222, 222, 224), RGB(242, 242, 242), RGB(26, 26, 26), RGB(0, 122, 255), 10.0f, 6.0f, UIToolTabStyle_Pill },
            { "Graphite Dark", RGB(26, 26, 26), RGB(38, 38, 38), RGB(230, 230, 230), RGB(153, 153, 153), 6.0f, 4.0f, UIToolTabStyle_Underline },
            { "Pastel", RGB(235, 230, 242), RGB(250, 247, 242), RGB(38, 38, 38), RGB(128, 115, 191), 10.0f, 6.0f, UIToolTabStyle_Pill },
        };
        constexpr i32 kPresetCount = (i32)(sizeof(kPresets) / sizeof(kPresets[0]));

        UIToolTheme s_Theme;
        i32 s_Preset       = -1;
        bool s_Initialised = false;

        f32 Luminance(const Vec4& c)
        {
            return c.x * 0.299f + c.y * 0.587f + c.z * 0.114f;
        }

        Vec4 Mix(const Vec4& a, const Vec4& b, f32 t)
        {
            return Vec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, 1.0f);
        }

        void EnsureInitialised()
        {
            if(s_Initialised)
                return;
            s_Initialised = true;
            UISetToolThemePreset(0);
        }
    }

    void UIDeriveToolTheme(UIToolTheme& theme, const Vec4& canvas, const Vec4& panel, const Vec4& text, const Vec4& accent)
    {
        const bool dark = Luminance(panel) < 0.5f;
        const Vec4 tint = dark ? Vec4(1.0f, 1.0f, 1.0f, 1.0f) : Vec4(0.0f, 0.0f, 0.0f, 1.0f);

        theme.Dark       = dark;
        theme.Canvas     = canvas;
        theme.Panel      = panel;
        theme.Header     = Mix(canvas, panel, 0.45f);
        theme.Popup      = Mix(panel, tint, dark ? 0.045f : 0.0f);
        theme.Popup.w    = 0.99f;
        theme.Field      = Vec4(tint.x, tint.y, tint.z, dark ? 0.07f : 0.06f);
        theme.Hover      = Vec4(tint.x, tint.y, tint.z, dark ? 0.10f : 0.08f);
        theme.Border     = Vec4(tint.x, tint.y, tint.z, dark ? 0.09f : 0.13f);
        theme.Text       = text;
        theme.TextDim    = Mix(text, panel, 0.42f);
        theme.Accent     = accent;
        theme.AccentText = Luminance(accent) > 0.6f ? Vec4(0.08f, 0.08f, 0.09f, 1.0f) : Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        theme.Selection  = Vec4(accent.x, accent.y, accent.z, dark ? 0.26f : 0.22f);
        theme.Warning    = Vec4(1.0f, 0.76f, 0.33f, 1.0f);
        theme.Error      = Vec4(1.0f, 0.44f, 0.41f, 1.0f);
        theme.Success    = Vec4(0.42f, 0.82f, 0.52f, 1.0f);
        theme.AxisX      = Vec4(1.00f, 0.50f, 0.47f, 1.0f);
        theme.AxisY      = Vec4(0.55f, 0.86f, 0.50f, 1.0f);
        theme.AxisZ      = Vec4(0.50f, 0.72f, 1.00f, 1.0f);
        if(!dark)
        {
            // the same hues, deep enough to read on a pale panel
            theme.Warning = Vec4(0.72f, 0.46f, 0.05f, 1.0f);
            theme.Error   = Vec4(0.78f, 0.20f, 0.18f, 1.0f);
            theme.Success = Vec4(0.16f, 0.55f, 0.28f, 1.0f);
            theme.AxisX   = Vec4(0.80f, 0.22f, 0.20f, 1.0f);
            theme.AxisY   = Vec4(0.22f, 0.58f, 0.18f, 1.0f);
            theme.AxisZ   = Vec4(0.16f, 0.42f, 0.85f, 1.0f);
        }
    }

    static UIToolTheme s_PlainTheme;
    static bool s_Plain = false;

    // follows the theme's dark / light, font size and density
    static void RebuildPlain()
    {
        const bool dark = s_Theme.Dark;
        const f32 a     = dark ? 0.0f : 1.0f;
        const f32 b     = dark ? 1.0f : 0.0f;
        UIToolTheme& t  = s_PlainTheme;
        t               = s_Theme;
        snprintf(t.Name, sizeof(t.Name), "Plain");
        t.Canvas     = Vec4(a, a, a, 1.0f);
        t.Panel      = t.Canvas;
        t.Header     = t.Canvas;
        t.Popup      = t.Canvas;
        t.Field      = Vec4(b, b, b, dark ? 0.09f : 0.07f);
        t.Hover      = Vec4(b, b, b, dark ? 0.14f : 0.10f);
        t.Border     = Vec4(b, b, b, dark ? 0.22f : 0.26f);
        t.Text       = Vec4(b, b, b, 1.0f);
        // grey on white reads weaker than grey on black
        const f32 dim = dark ? 0.62f : 0.30f;
        t.TextDim     = Vec4(dim, dim, dim, 1.0f);
        t.Accent      = t.Text;
        t.AccentText  = t.Canvas;
        t.Selection   = Vec4(b, b, b, dark ? 0.20f : 0.14f);

        t.PanelRounding    = 0.0f;
        t.WidgetRounding   = 0.0f;
        t.PanelGap         = 0.0f;
        t.OuterMargin      = 0.0f;
        t.TabStyle         = UIToolTabStyle_Underline;
        t.PanelBorder      = true;
        t.FloatingToolbars = false;
    }

    void UISetToolThemePlain(bool plain)
    {
        EnsureInitialised();
        s_Plain = plain;
        if(plain)
            RebuildPlain();
    }

    bool UIIsToolThemePlain() { return s_Plain; }

    UIToolTheme& UIGetToolTheme()
    {
        EnsureInitialised();
        return s_Plain ? s_PlainTheme : s_Theme;
    }

    i32 UIGetToolThemePresetCount() { return kPresetCount; }

    const char* UIGetToolThemePresetName(i32 preset)
    {
        return (preset >= 0 && preset < kPresetCount) ? kPresets[preset].Name : "Custom";
    }

    void UISetToolThemePreset(i32 preset)
    {
        s_Initialised   = true;
        preset          = Maths::Clamp(preset, 0, kPresetCount - 1);
        const Preset& p = kPresets[preset];

        s_Theme = {};
        snprintf(s_Theme.Name, sizeof(s_Theme.Name), "%s", p.Name);
        UIDeriveToolTheme(s_Theme, p.Canvas, p.Panel, p.Text, p.Accent);

        s_Theme.FontSize         = 14.5f;
        s_Theme.PanelRounding    = p.PanelRounding;
        s_Theme.WidgetRounding   = Maths::Min(5.0f, p.PanelRounding * 0.5f + 1.0f);
        s_Theme.PanelGap         = p.PanelGap;
        s_Theme.OuterMargin      = p.PanelGap;
        s_Theme.TabHeight        = 32.0f;
        s_Theme.Density          = 1.0f;
        s_Theme.TabStyle         = p.TabStyle;
        s_Theme.PanelBorder      = p.PanelGap < 3.0f;
        s_Theme.FloatingToolbars = p.PanelRounding >= 6.0f;
        s_Preset                 = preset;
        if(s_Plain)
            RebuildPlain();
    }

    i32 UIGetToolThemePreset() { return s_Preset; }

    void UIMarkToolThemeEdited()
    {
        s_Preset     = -1;
        s_Theme.Dark = Luminance(s_Theme.Panel) < 0.5f;
        if(s_Plain)
            RebuildPlain();
    }

    void UIApplyToolThemeToContext()
    {
        EnsureInitialised();
        UI_State* ui = GetUIState();
        if(!ui)
            return;
        const UIToolTheme& theme = UIGetToolTheme();
        const UITheme wanted     = theme.Dark ? UITheme_Dark : UITheme_Light;
        if(ui->CurrentTheme != wanted)
            UISetTheme(wanted);
        ui->AccentColour = theme.Accent;
    }

    // ---- files ----

    namespace
    {
        template <typename Archive>
        void Colour(Archive& archive, const char* name, Vec4& c)
        {
            float v[4] = { c.x, c.y, c.z, c.w };
            archive(cereal::make_nvp(name, v));
            c = Vec4(v[0], v[1], v[2], v[3]);
        }

        template <typename Archive>
        void Fields(Archive& archive, UIToolTheme& t, std::string& name, int& preset)
        {
            archive(cereal::make_nvp("Name", name), cereal::make_nvp("Preset", preset));
            Colour(archive, "Canvas", t.Canvas);
            Colour(archive, "Panel", t.Panel);
            Colour(archive, "Header", t.Header);
            Colour(archive, "Popup", t.Popup);
            Colour(archive, "Field", t.Field);
            Colour(archive, "Hover", t.Hover);
            Colour(archive, "Border", t.Border);
            Colour(archive, "Text", t.Text);
            Colour(archive, "TextDim", t.TextDim);
            Colour(archive, "Accent", t.Accent);
            Colour(archive, "AccentText", t.AccentText);
            Colour(archive, "Selection", t.Selection);
            Colour(archive, "Warning", t.Warning);
            Colour(archive, "Error", t.Error);
            Colour(archive, "Success", t.Success);
            Colour(archive, "AxisX", t.AxisX);
            Colour(archive, "AxisY", t.AxisY);
            Colour(archive, "AxisZ", t.AxisZ);
            archive(cereal::make_nvp("FontSize", t.FontSize), cereal::make_nvp("PanelRounding", t.PanelRounding),
                    cereal::make_nvp("WidgetRounding", t.WidgetRounding), cereal::make_nvp("PanelGap", t.PanelGap),
                    cereal::make_nvp("OuterMargin", t.OuterMargin), cereal::make_nvp("TabHeight", t.TabHeight),
                    cereal::make_nvp("Density", t.Density), cereal::make_nvp("TabStyle", t.TabStyle),
                    cereal::make_nvp("PanelBorder", t.PanelBorder), cereal::make_nvp("FloatingToolbars", t.FloatingToolbars));
        }
    }

    bool UISaveToolTheme(const String8& path)
    {
        EnsureInitialised();
        if(path.size == 0)
            return false;

        std::string name = s_Theme.Name;
        int preset       = s_Preset;
        std::stringstream storage;
        {
            cereal::JSONOutputArchive output { storage };
            Fields(output, s_Theme, name, preset);
        }
        const std::string text = storage.str();
        return FileSystem::WriteTextFile(path, Str8((u8*)text.data(), text.size()));
    }

    bool UILoadToolTheme(const String8& path)
    {
        EnsureInitialised();
        if(path.size == 0 || !FileSystem::FileExists(path))
            return false;

        ArenaTemp scratch = ScratchBegin(0, 0);
        String8 text      = FileSystem::ReadTextFile(scratch.arena, path);
        bool loaded       = false;
        if(text.size > 0)
        {
            UIToolTheme theme = s_Theme;
            std::string name;
            int preset = -1;
            try
            {
                std::stringstream storage(std::string((const char*)text.str, (size_t)text.size));
                cereal::JSONInputArchive input(storage);
                Fields(input, theme, name, preset);

                snprintf(theme.Name, sizeof(theme.Name), "%s", name.c_str());
                // files saved with the old default size follow the new one
                if(theme.FontSize == 13.0f)
                    theme.FontSize = 14.5f;
                theme.FontSize       = Maths::Clamp(theme.FontSize, 9.0f, 24.0f);
                theme.PanelRounding  = Maths::Clamp(theme.PanelRounding, 0.0f, 24.0f);
                theme.WidgetRounding = Maths::Clamp(theme.WidgetRounding, 0.0f, 16.0f);
                theme.PanelGap       = Maths::Clamp(theme.PanelGap, 0.0f, 24.0f);
                theme.OuterMargin    = Maths::Clamp(theme.OuterMargin, 0.0f, 32.0f);
                theme.TabHeight      = Maths::Clamp(theme.TabHeight, 20.0f, 48.0f);
                theme.Density        = Maths::Clamp(theme.Density, 0.6f, 1.8f);
                theme.TabStyle       = Maths::Clamp(theme.TabStyle, 0, (i32)UIToolTabStyle_Count - 1);
                theme.Dark           = Luminance(theme.Panel) < 0.5f;

                s_Theme  = theme;
                s_Preset = (preset >= 0 && preset < kPresetCount) ? preset : -1;
                loaded   = true;
                if(s_Plain)
                    RebuildPlain();
            }
            catch(...)
            {
                LWARN("UITheme: could not read '%.*s'", (int)path.size, (const char*)path.str);
            }
        }
        ScratchEnd(scratch);
        return loaded;
    }
}
