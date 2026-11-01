#include "Precompiled.h"
#include "WidgetsLua.h"
#include "LuaSmartPtrTraits.h"
#include "Graphics/UI.h"
#include "Graphics/Font.h"
#include "Graphics/RHI/Texture.h"
#include "Maths/MathsUtilities.h"

#include <imgui/imgui.h>
#include "Core/Application.h"
#include "ImGui/ImGuiManager.h"
#include "Graphics/RHI/IMGUIRenderer.h"
#include <unordered_map>

namespace Lumos
{
    // Extra custom-UI widgets for Lua. Engine widgets taking raw pointers get
    // value-in/value-out wrappers with state kept here, keyed by id.
    void BindWidgetsLua(sol::state& lua)
    {
        lua.set_function("UIBeginRow", []() { UIBeginRow(); });
        lua.set_function("UIBeginRowFullWidth", []() { UIBeginRowFullWidth(); });
        lua.set_function("UIEndRow", []() { UIEndRow(); });
        lua.set_function("UIBeginColumn", []() { UIBeginColumn(); });
        lua.set_function("UIEndColumn", []() { UIEndColumn(); });
        lua.set_function("UIFlexSpacer", [](sol::optional<float> weight) { UIFlexSpacer(weight.value_or(1.0f)); });

        lua.set_function("UIBeginSelectableRow", [](const char* id, bool selected, sol::optional<float> height)
                         { return UIBeginSelectableRow(id, selected, height.value_or(0.0f)); });
        lua.set_function("UIEndSelectableRow", []() { UIEndSelectableRow(); });

        lua.set_function("UIBadge", [](const char* id, const char* text, const Vec4& bg, const Vec4& fg)
                         { return UIBadge(id, Str8C((char*)text), bg, fg); });

        // Full-viewport dimming quad that swallows clicks - build it at root level right before a modal/popup
        lua.set_function("UIDimBackdrop", [](const char* id, float alpha) { return UIDimBackdrop(id, alpha); });

        lua.set_function("UITooltip", [](const char* text) { UITooltip(text); });

        // Image at an explicit logical size (DPI-scaled here), whatever the texture's pixel size.
        // Untinted, no border; dims slightly on hover. radius is logical px. nil texture draws `placeholder` colour.
        lua.set_function("UIImageSized", [](const char* id, Graphics::Texture2D* tex, float w, float h,
                                            sol::optional<float> radius, sol::optional<Vec4> placeholder)
                         {
            float dpi = GetUIState() ? GetUIState()->DPIScale : 1.0f;
            Vec2 scale(1.0f);
            if(tex && tex->GetWidth() > 0 && tex->GetHeight() > 0)
                scale = Vec2(w * dpi / (float)tex->GetWidth(), h * dpi / (float)tex->GetHeight());
            else
                scale = Vec2(w * dpi / 16.0f, h * dpi / 16.0f);
            UI_Interaction it = UIImage(id, tex, scale);
            if(UI_Widget* wd = it.widget)
            {
                // UIImage ignores scale for a missing texture - pin the size either way
                wd->semantic_size[UIAxis_X] = { SizeKind_Pixels, w * dpi };
                wd->semantic_size[UIAxis_Y] = { SizeKind_Pixels, h * dpi };
                // Set on the widget, not pushed: hovered clickables otherwise take the theme's hot fill
                Vec4 base = tex ? Vec4(1.0f) : placeholder.value_or(Vec4(1.0f, 1.0f, 1.0f, 0.08f));
                Vec4 hot  = tex ? Vec4(0.85f, 0.85f, 0.85f, 1.0f) : base;
                wd->style_vars[StyleVar_BackgroundColor]       = base;
                wd->style_vars[StyleVar_HotBackgroundColor]    = hot;
                wd->style_vars[StyleVar_ActiveBackgroundColor] = hot;
                wd->style_vars[StyleVar_Border]                = Vec4(0.0f);
                wd->style_vars[StyleVar_BorderColor]           = Vec4(0.0f);
                wd->style_vars[StyleVar_HotBorderColor]        = Vec4(0.0f);
                wd->style_vars[StyleVar_ActiveBorderColor]     = Vec4(0.0f);
                wd->style_vars[StyleVar_ShadowColor]           = Vec4(0.0f);
                wd->style_vars[StyleVar_Padding]               = Vec4(0.0f);
                wd->style_vars[StyleVar_CornerRadius]          = Vec4(radius.value_or(0.0f) * dpi, 0.0f, 0.0f, 0.0f);
            }
            return it; });

        // Fixed-size button with its own colours (the theme's hover fill otherwise wins on clickables).
        // text may use "label###id". w/h/radius/fontSize are logical px; w or h <= 0 sizes to the text.
        lua.set_function("UIButtonStyled", [](const char* text, float w, float h, const Vec4& bg, const Vec4& hot,
                                              const Vec4& fg, sol::optional<float> radius, sol::optional<float> fontSize)
                         {
            float dpi         = GetUIState() ? GetUIState()->DPIScale : 1.0f;
            UI_Interaction it = UIButton(text);
            if(UI_Widget* wd = it.widget)
            {
                if(w > 0.0f)
                    wd->semantic_size[UIAxis_X] = { SizeKind_Pixels, w * dpi };
                if(h > 0.0f)
                    wd->semantic_size[UIAxis_Y] = { SizeKind_Pixels, h * dpi };
                wd->flags &= ~WidgetFlags_AnimateScale;
                wd->style_vars[StyleVar_BackgroundColor]       = bg;
                wd->style_vars[StyleVar_HotBackgroundColor]    = hot;
                wd->style_vars[StyleVar_ActiveBackgroundColor] = hot;
                wd->style_vars[StyleVar_TextColor]             = fg;
                wd->style_vars[StyleVar_HotTextColor]          = fg;
                wd->style_vars[StyleVar_ActiveTextColor]       = fg;
                wd->style_vars[StyleVar_Border]                = Vec4(0.0f);
                wd->style_vars[StyleVar_BorderColor]           = Vec4(0.0f);
                wd->style_vars[StyleVar_HotBorderColor]        = Vec4(0.0f);
                wd->style_vars[StyleVar_ActiveBorderColor]     = Vec4(0.0f);
                wd->style_vars[StyleVar_ShadowColor]           = Vec4(0.0f);
                wd->style_vars[StyleVar_CornerRadius]          = Vec4(radius.value_or(6.0f) * dpi, 0.0f, 0.0f, 0.0f);
                if(w <= 0.0f || h <= 0.0f)
                    wd->style_vars[StyleVar_Padding] = Vec4(10.0f * dpi, 5.0f * dpi, 0.0f, 0.0f);
                else
                    wd->style_vars[StyleVar_Padding] = Vec4(0.0f);
                if(fontSize)
                    wd->style_vars[StyleVar_FontSize] = Vec4(*fontSize * dpi, 0.0f, 0.0f, 0.0f);
            }
            return it; });

        // Warped textured quad. corners = { tlx, tly, trx, try, brx, bry, blx, bly } in logical px relative to (x, y).
        // vertical: the two depths are the top / bottom edges (a card tilted back) instead of left / right.
        lua.set_function("UIImageWarp", [](const char* id, Graphics::Texture2D* tex, float x, float y, float w, float h,
                                           sol::table corners, float depthLeft, float depthRight,
                                           const Vec4& top, const Vec4& bottom, sol::optional<bool> flipV, sol::optional<bool> clickable,
                                           sol::optional<float> radius, sol::optional<bool> vertical)
                         {
            Vec2 c[4];
            for(int i = 0; i < 4; i++)
                c[i] = Vec2(corners.get_or(i * 2 + 1, 0.0f), corners.get_or(i * 2 + 2, 0.0f));
            return UIImageWarp(id, tex, Vec2(x, y), Vec2(w, h), c, depthLeft, depthRight, top, bottom,
                               flipV.value_or(false), clickable.value_or(true), radius.value_or(0.0f), vertical.value_or(false)); });

        // values: array of numbers. Sizes are logical px (scaled by the engine).
        lua.set_function("UIPlotHistogram", [](const char* id, sol::table values, float minValue, float maxValue,
                                               float w, float h, const Vec4& colour, sol::optional<Vec4> bg)
                         {
            float buf[512];
            u32 n = (u32)Maths::Min<size_t>(values.size(), 512);
            for(u32 i = 0; i < n; i++)
                buf[i] = values.get_or(i + 1, 0.0f);
            UIPlotHistogram(id, buf, n, minValue, maxValue, w, h, colour, bg.value_or(Vec4(0.0f))); });

        // Search box. Returns the current text; setText overwrites it (e.g. to clear).
        // true when the UI font can draw every character (currency symbols, other scripts...)
        lua.set_function("FontHasGlyphs", [](const std::string& text)
                         {
            auto font = Graphics::Font::GetDefaultFont();
            return font && font->HasGlyphs(text); });

        lua.set_function("UISearchField", [](const std::string& id, sol::optional<std::string> placeholder,
                                             sol::optional<float> width, sol::optional<std::string> setText) -> std::string
                         {
            static std::unordered_map<std::string, std::string> s_Buffers;
            static std::unordered_map<std::string, std::string> s_Placeholders; // widget keeps a pointer to it
            auto& buf = s_Buffers[id];
            auto& ph  = s_Placeholders[id];
            if(placeholder && ph != *placeholder)
                ph = *placeholder;
            if(buf.size() < 256)
                buf.resize(256, '\0'); // fixed size => &buf[0] stays valid
            if(setText)
            {
                memset(&buf[0], 0, 256);
                strncpy(&buf[0], setText->c_str(), 255);
            }
            UISearchField(id.c_str(), &buf[0], 256, ph.c_str(), width.value_or(0.0f));
            return std::string(buf.c_str()); });

        lua.set_function("UIDragFloat", [](const char* id, float value, sol::optional<float> speed, sol::optional<float> minValue,
                                           sol::optional<float> maxValue, sol::optional<std::string> fmt, sol::optional<float> width) -> float
                         {
            float v = value;
            UIDragFloat(id, &v, speed.value_or(0.01f), minValue.value_or(0.0f), maxValue.value_or(0.0f),
                        fmt ? fmt->c_str() : "%.3f", width.value_or(84.0f));
            return v; });

        // Scroll area whose offset Lua can read/write - lets a long list build only its visible rows.
        // Height is raw px (DPI-scale it yourself), like UIBeginScrollArea. Returns the offset in px.
        // Seek bar / volume scrubber. Returns value, isDragging, changedThisFrame.
        lua.set_function("UIScrubBar", [](const char* id, float value, float w, float h, const Vec4& fill, const Vec4& track) -> std::tuple<float, bool, bool>
                         {
            float v     = value;
            bool active = false;
            UI_Interaction it = UIScrubBar(id, &v, w, h, fill, track, &active);
            (void)it;
            return { v, active, v != value }; });

        // Whether ImGui wants the mouse/keyboard this frame, so tools can skip their own camera input
        sol::table imguiTable = lua["imgui"].get_or_create<sol::table>();
        imguiTable.set_function("wantCaptureMouse", []() { return ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse; });
        imguiTable.set_function("wantCaptureKeyboard", []() { return ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard; });

        imguiTable.set_function("getMousePos", []() { return ImGui::GetMousePos(); });
        imguiTable.set_function("isMouseDown", [](int button) { return ImGui::IsMouseDown(button); });
        imguiTable.set_function("textRGBA", [](float r, float g, float b, float a, const char* text)
                                { ImGui::TextColored(ImVec4(r, g, b, a), "%s", text); });

        // Image with explicit UVs - no automatic backend flip, for textures already stored top-down
        imguiTable.set_function("imageUV", [](Graphics::Texture2D* texture, float w, float h, float u0, float v0, float u1, float v1)
                                {
            if(!texture || !Application::Get().GetImGuiManager())
                return;
            auto* renderer = Application::Get().GetImGuiManager()->GetImGuiRenderer();
            ImGui::Image(renderer->AddTexture(texture), ImVec2(w, h), ImVec2(u0, v0), ImVec2(u1, v1)); });

        static std::unordered_map<std::string, float> s_ScrollOffsets; // node-based: pointers stay valid
        static std::unordered_map<std::string, Vec2> s_ScrollAreaRects;  // last layout: top, height (px)
        lua.set_function("UIBeginScrollAreaEx", [](const std::string& id, float height, sol::optional<bool> transparent) -> float
                         {
            float& offset = s_ScrollOffsets[id];
            UIBeginScrollArea(id.c_str(), height, &offset);
            if(GetUIState() && !GetUIState()->parents.Empty())
            {
                UI_Widget* area = GetUIState()->parents.Back();
                s_ScrollAreaRects[id] = Vec2(area->position.y, area->size.y);
                if(transparent.value_or(true))
                {
                    area->style_vars[StyleVar_BackgroundColor] = Vec4(0.0f);
                    area->style_vars[StyleVar_BorderColor]     = Vec4(0.0f);
                }
            }
            return offset; });
        static std::unordered_map<std::string, float> s_HScrollOffsets;
        lua.set_function("UIBeginHScrollArea", [](const std::string& id) -> float
                         {
            float& offset = s_HScrollOffsets[id];
            UIBeginHScrollArea(id.c_str(), &offset);
            return offset; });
        lua.set_function("UIEndHScrollArea", []() { UIEndHScrollArea(); });
        lua.set_function("UIGetHScrollOffset", [](const std::string& id) -> float
                         {
            auto it = s_HScrollOffsets.find(id);
            return it == s_HScrollOffsets.end() ? 0.0f : it->second; });
        // Where a named scroll area sat last frame: top and height in pixels (0, 0 if never drawn)
        lua.set_function("UIGetScrollAreaRect", [](const std::string& id) -> std::tuple<float, float>
                         {
            auto it = s_ScrollAreaRects.find(id);
            if(it == s_ScrollAreaRects.end())
                return { 0.0f, 0.0f };
            return { it->second.x, it->second.y }; });
        lua.set_function("UIGetScrollOffset", [](const std::string& id) -> float
                         {
            auto it = s_ScrollOffsets.find(id);
            return it == s_ScrollOffsets.end() ? 0.0f : it->second; });
        lua.set_function("UISetScrollOffset", [](const std::string& id, float offset)
                         { s_ScrollOffsets[id] = Maths::Max(0.0f, offset); });
    }
}
