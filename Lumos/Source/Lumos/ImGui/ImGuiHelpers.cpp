#include "Precompiled.h"
#include "ImGui/ImGuiHelpers.h"
#include "Graphics/API/Texture.h"
#include "Graphics/API/GraphicsContext.h"
#include "Core/OS/OS.h"

#include <imgui/imgui_internal.h>

namespace Lumos
{
    Maths::Vector4 SelectedColour = Maths::Vector4(0.28f, 0.56f, 0.9f, 1.0f);

    bool ImGuiHelpers::Property(const std::string& name, bool& value)
    {
        bool updated = false;
        ImGui::TextUnformatted(name.c_str());
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        std::string id = "##" + name;
        if(ImGui::Checkbox(id.c_str(), &value))
            updated = true;

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiHelpers::Property(const std::string& name, float& value, float min, float max, ImGuiHelpers::PropertyFlag flags)
    {
        bool updated = false;

        ImGui::TextUnformatted(name.c_str());
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        std::string id = "##" + name;
        if(ImGui::SliderFloat(id.c_str(), &value, min, max))
            updated = true;

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiHelpers::Property(const std::string& name, Maths::Vector2& value, ImGuiHelpers::PropertyFlag flags)
    {
        return ImGuiHelpers::Property(name, value, -1.0f, 1.0f, flags);
    }

    bool ImGuiHelpers::Property(const std::string& name, Maths::Vector2& value, float min, float max, ImGuiHelpers::PropertyFlag flags)
    {
        bool updated = false;

        ImGui::TextUnformatted(name.c_str());
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        std::string id = "##" + name;
        if(ImGui::SliderFloat2(id.c_str(), Maths::ValuePointer(value), min, max))
            updated = true;

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiHelpers::Property(const std::string& name, Maths::Vector3& value, ImGuiHelpers::PropertyFlag flags)
    {
        return ImGuiHelpers::Property(name, value, -1.0f, 1.0f, flags);
    }

    bool ImGuiHelpers::Property(const std::string& name, Maths::Vector3& value, float min, float max, ImGuiHelpers::PropertyFlag flags)
    {
        bool updated = false;

        ImGui::TextUnformatted(name.c_str());
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        std::string id = "##" + name;
        if((int)flags & (int)PropertyFlag::ColourProperty)
        {
            if(ImGui::ColorEdit3(id.c_str(), Maths::ValuePointer(value), ImGuiColorEditFlags_NoInputs))
                updated = true;
        }
        else
        {
            if(ImGui::SliderFloat3(id.c_str(), Maths::ValuePointer(value), min, max))
                updated = true;
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiHelpers::Property(const std::string& name, Maths::Vector4& value, bool exposeW, ImGuiHelpers::PropertyFlag flags)
    {
        return Property(name, value, -1.0f, 1.0f, exposeW, flags);
    }

    bool ImGuiHelpers::Property(const std::string& name, Maths::Vector4& value, float min, float max, bool exposeW, ImGuiHelpers::PropertyFlag flags)
    {
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name.c_str());
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        std::string id = "##" + name;
        if((int)flags & (int)PropertyFlag::ColourProperty)
        {
            if(ImGui::ColorEdit4(id.c_str(), Maths::ValuePointer(value), ImGuiColorEditFlags_NoInputs))
                updated = true;
        }
        else if((exposeW ? ImGui::SliderFloat4(id.c_str(), Maths::ValuePointer(value), min, max) : ImGui::SliderFloat3(id.c_str(), Maths::ValuePointer(value), min, max)))
            updated = true;

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    void ImGuiHelpers::Tooltip(const std::string& text)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
        Tooltip(text.c_str());
        ImGui::PopStyleVar();
    }

    void ImGuiHelpers::Tooltip(const char* text)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

        if(ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(text);
            ImGui::EndTooltip();
        }

        ImGui::PopStyleVar();
    }

    void ImGuiHelpers::Tooltip(Graphics::Texture2D* texture, const Maths::Vector2& size)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

        if(ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            bool flipImage = Graphics::GraphicsContext::GetContext()->FlipImGUITexture();
            ImGui::Image(texture ? texture->GetHandle() : nullptr, ImVec2(size.x, size.y), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
            ImGui::EndTooltip();
        }

        ImGui::PopStyleVar();
    }

    void ImGuiHelpers::Tooltip(Graphics::Texture2D* texture, const Maths::Vector2& size, const std::string& text)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

        if(ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            bool flipImage = Graphics::GraphicsContext::GetContext()->FlipImGUITexture();
            ImGui::Image(texture ? texture->GetHandle() : nullptr, ImVec2(size.x, size.y), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
            ImGui::TextUnformatted(text.c_str());
            ImGui::EndTooltip();
        }

        ImGui::PopStyleVar();
    }

    void ImGuiHelpers::Image(Graphics::Texture2D* texture, const Maths::Vector2& size)
    {
        bool flipImage = Graphics::GraphicsContext::GetContext()->FlipImGUITexture();
        ImGui::Image(texture ? texture->GetHandle() : nullptr, ImVec2(size.x, size.y), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
    }

    void ImGuiHelpers::Image(Graphics::TextureCube* texture, const Maths::Vector2& size)
    {
        bool flipImage = Graphics::GraphicsContext::GetContext()->FlipImGUITexture();
        ImGui::Image(texture ? texture->GetHandle() : nullptr, ImVec2(size.x, size.y), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
    }

    bool ImGuiHelpers::BufferingBar(const char* label, float value, const Maths::Vector2& size_arg, const uint32_t& bg_col, const uint32_t& fg_col)
    {
        auto g = ImGui::GetCurrentContext();
        auto drawList = ImGui::GetWindowDrawList();
        const ImGuiStyle& style = ImGui::GetStyle();
        const ImGuiID id = ImGui::GetID(label);

        ImVec2 pos = ImGui::GetCursorPos();
        ImVec2 size = { size_arg.x, size_arg.y };
        size.x -= style.FramePadding.x * 2;

        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb, style.FramePadding.y);
        if(!ImGui::ItemAdd(bb, id))
            return false;

        // Render
        const float circleStart = size.x * 0.7f;
        const float circleEnd = size.x;
        const float circleWidth = circleEnd - circleStart;

        drawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart, bb.Max.y), bg_col);
        drawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart * value, bb.Max.y), fg_col);

        const float t = float(g->Time);
        const float r = size.y / 2.0f;
        const float speed = 1.5f;

        const float a = speed * 0.f;
        const float b = speed * 0.333f;
        const float c = speed * 0.666f;

        const float o1 = (circleWidth + r) * (t + a - speed * (int)((t + a) / speed)) / speed;
        const float o2 = (circleWidth + r) * (t + b - speed * (int)((t + b) / speed)) / speed;
        const float o3 = (circleWidth + r) * (t + c - speed * (int)((t + c) / speed)) / speed;

        drawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o1, bb.Min.y + r), r, bg_col);
        drawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o2, bb.Min.y + r), r, bg_col);
        drawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o3, bb.Min.y + r), r, bg_col);

        return true;
    }

    bool ImGuiHelpers::Spinner(const char* label, float radius, int thickness, const uint32_t& colour)
    {
        auto g = ImGui::GetCurrentContext();
        const ImGuiStyle& style = g->Style;
        const ImGuiID id = ImGui::GetID(label);
        auto drawList = ImGui::GetWindowDrawList();

        ImVec2 pos = ImGui::GetCursorPos();
        ImVec2 size((radius)*2, (radius + style.FramePadding.y) * 2);

        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb, style.FramePadding.y);
        if(!ImGui::ItemAdd(bb, id))
            return false;

        // Render
        drawList->PathClear();

        int num_segments = 30;
        float start = abs(ImSin(float(g->Time) * 1.8f) * (num_segments - 5));

        const float a_min = IM_PI * 2.0f * (start / float(num_segments));
        const float a_max = IM_PI * 2.0f * (float(num_segments) - 3.0f) / (float)num_segments;

        const ImVec2 centre = ImVec2(pos.x + radius, pos.y + radius + style.FramePadding.y);

        for(int i = 0; i < num_segments; i++)
        {
            const float a = a_min + (float(i) / float(num_segments)) * (a_max - a_min);
            drawList->PathLineTo(ImVec2(centre.x + ImCos(a + float(g->Time) * 8) * radius,
                centre.y + ImSin(a + float(g->Time) * 8) * radius));
        }

        drawList->PathStroke(colour, false, float(thickness));

        return true;
    }

    void ImGuiHelpers::SetTheme(Theme theme)
    {
        ImVec4 colour_for_text = ImVec4(236.f / 255.f, 240.f / 255.f, 241.f / 255.f, 1.0f);
        ImVec4 colour_for_head = ImVec4(41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 1.0f);
        ImVec4 colour_for_area = ImVec4(57.f / 255.f, 79.f / 255.f, 105.f / 255.f, 1.0f);
        ImVec4 colour_for_body = ImVec4(44.f / 255.f, 62.f / 255.f, 80.f / 255.f, 1.0f);
        ImVec4 colour_for_pops = ImVec4(33.f / 255.f, 46.f / 255.f, 60.f / 255.f, 1.0f);

        ImVec4 black(0, 0, 0, 0);
        ImVec4 white(1, 1, 1, 1);
        ImVec4 colour1(0.86f, 0.93f, 0.89f, 1); // text ->
        ImVec4 colour2(0.20f, 0.22f, 0.27f, 1); // blur
        ImVec4 colour3(0.92f, 0.18f, 0.29f, 1); // active
        ImVec4 colour4(0.47f, 0.77f, 0.83f, 1); // slider ->
        ImVec4 colour5(0.13f, 0.14f, 0.17f, 1); // windowbg
        ImVec4 colour6(0.31f, 0.31f, 1.00f, 1); // border, unused -> black
        ImVec4 colour7(0.09f, 0.15f, 0.16f, 1); // ScrollbarGrab
        ImVec4 colour8(0.71f, 0.22f, 0.27f, 1); // CheckMark
        ImVec4 colour9(0.14f, 0.16f, 0.19f, 1); // Column

        static const float max = 255.0f;
        ImVec4 blue = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
        ImVec4 red = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

        ImVec4 Titlebar = ImVec4(20.0f / max, 28.0f / max, 50.0f / max, 1.0f);
        ImVec4 windowBackground = Titlebar + ImVec4(12.0f / max, 12.0f / max, 12.0f / max, 0.0f);
        ImVec4 TabActive = windowBackground;
        ImVec4 TabUnactive = ImVec4(35.0f / max, 43.0f / max, 59.0f / max, 1.0f);

        ImVec4* colours = ImGui::GetStyle().Colors;
        switch(theme)
        {
        case Lumos::ImGuiHelpers::Black:
            ImGui::StyleColorsDark();
            colours[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            colours[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
            colours[ImGuiCol_WindowBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
            colours[ImGuiCol_ChildBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
            colours[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
            colours[ImGuiCol_Border] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
            colours[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colours[ImGuiCol_FrameBg] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
            colours[ImGuiCol_FrameBgHovered] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
            colours[ImGuiCol_FrameBgActive] = ImVec4(0.37f, 0.37f, 0.37f, 1.00f);
            colours[ImGuiCol_TitleBg] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
            colours[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
            colours[ImGuiCol_TitleBgCollapsed] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
            colours[ImGuiCol_MenuBarBg] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
            colours[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.00f);
            colours[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
            colours[ImGuiCol_CheckMark] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
            colours[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
            colours[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
            colours[ImGuiCol_Button] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
            colours[ImGuiCol_ButtonHovered] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
            colours[ImGuiCol_ButtonActive] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
            colours[ImGuiCol_Header] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
            colours[ImGuiCol_HeaderHovered] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
            colours[ImGuiCol_HeaderActive] = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
            colours[ImGuiCol_Separator] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
            colours[ImGuiCol_SeparatorHovered] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
            colours[ImGuiCol_SeparatorActive] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
            colours[ImGuiCol_ResizeGrip] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
            colours[ImGuiCol_ResizeGripHovered] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
            colours[ImGuiCol_ResizeGripActive] = ImVec4(0.37f, 0.37f, 0.37f, 1.00f);
            colours[ImGuiCol_Tab] = ImVec4(0.26f, 0.26f, 0.26f, 0.40f);
            colours[ImGuiCol_TabHovered] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
            colours[ImGuiCol_TabActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
            colours[ImGuiCol_TabUnfocused] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
            colours[ImGuiCol_TabUnfocusedActive] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
            colours[ImGuiCol_DockingPreview] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
            colours[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
            colours[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
            colours[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
            colours[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            colours[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            colours[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
            colours[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
            colours[ImGuiCol_NavHighlight] = ImVec4(0.78f, 0.88f, 1.00f, 1.00f);
            colours[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
            colours[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
            colours[ImGuiCol_ModalWindowDimBg] = ImVec4(0.44f, 0.44f, 0.44f, 0.35f);
            break;
        case Lumos::ImGuiHelpers::Dark:
            ImGui::StyleColorsDark();

            colours[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
            colours[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);

            colours[ImGuiCol_WindowBg] = TabActive;
            colours[ImGuiCol_ChildBg] = TabActive;

            colours[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
            colours[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
            colours[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colours[ImGuiCol_FrameBg] = ImVec4(61.0f / 255.0f, 83.0f / 255.0f, 103.0f / 255.0f, 1.00f);
            colours[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
            colours[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);

            colours[ImGuiCol_TitleBg] = Titlebar;
            colours[ImGuiCol_TitleBgActive] = Titlebar;
            colours[ImGuiCol_TitleBgCollapsed] = Titlebar;
            colours[ImGuiCol_MenuBarBg] = Titlebar;

            colours[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
            colours[ImGuiCol_ScrollbarGrab] = ImVec4(0.6f, 0.6f, 0.6f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.7f, 0.7f, 0.7f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.8f, 0.8f, 0.8f, 1.00f);

            colours[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
            colours[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
            colours[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
            colours[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
            colours[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f) + ImVec4(0.1f, 0.1f, 0.1f, 0.1f);
            colours[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f) + ImVec4(0.1f, 0.1f, 0.1f, 0.1f);

            colours[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
            colours[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
            colours[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);

            colours[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
            colours[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            colours[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);

            colours[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
            colours[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
            colours[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            colours[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            colours[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
            colours[ImGuiCol_DragDropTarget] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colours[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colours[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
            colours[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
            colours[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

            colours[ImGuiCol_Header] = TabActive + ImVec4(0.1f, 0.1f, 0.1f, 0.1f);
            colours[ImGuiCol_HeaderHovered] = TabActive + ImVec4(0.1f, 0.1f, 0.1f, 0.1f);
            colours[ImGuiCol_HeaderActive] = TabActive + ImVec4(0.05f, 0.05f, 0.05f, 0.1f);

#ifdef IMGUI_HAS_DOCK

            colours[ImGuiCol_Tab] = TabUnactive;
            colours[ImGuiCol_TabHovered] = TabActive + ImVec4(0.1f, 0.1f, 0.1f, 0.1f);
            colours[ImGuiCol_TabActive] = TabActive;
            colours[ImGuiCol_TabUnfocused] = TabUnactive;
            colours[ImGuiCol_TabUnfocusedActive] = TabActive;
            colours[ImGuiCol_DockingEmptyBg] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
            colours[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);

#endif
            break;
        case Lumos::ImGuiHelpers::Grey:
            ImGui::StyleColorsDark();
            colours[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            colours[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
            colours[ImGuiCol_ChildBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colours[ImGuiCol_WindowBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colours[ImGuiCol_PopupBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colours[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
            colours[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
            colours[ImGuiCol_FrameBg] = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
            colours[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
            colours[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
            colours[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
            colours[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
            colours[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
            colours[ImGuiCol_MenuBarBg] = ImVec4(0.335f, 0.335f, 0.335f, 1.000f);
            colours[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
            colours[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
            colours[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
            colours[ImGuiCol_SliderGrab] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
            colours[ImGuiCol_SliderGrabActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
            colours[ImGuiCol_Button] = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
            colours[ImGuiCol_ButtonHovered] = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
            colours[ImGuiCol_ButtonActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
            colours[ImGuiCol_Header] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
            colours[ImGuiCol_HeaderHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
            colours[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
            colours[ImGuiCol_Separator] = ImVec4(0.000f, 0.000f, 0.000f, 0.137f);
            colours[ImGuiCol_SeparatorHovered] = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
            colours[ImGuiCol_SeparatorActive] = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
            colours[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
            colours[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            colours[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
            colours[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
            colours[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
            colours[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            colours[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            colours[ImGuiCol_TextSelectedBg] = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
            colours[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
            colours[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
            colours[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colours[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
            colours[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

#ifdef IMGUI_HAS_DOCK
            colours[ImGuiCol_DockingEmptyBg] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
            colours[ImGuiCol_Tab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colours[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
            colours[ImGuiCol_TabActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
            colours[ImGuiCol_TabUnfocused] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colours[ImGuiCol_TabUnfocusedActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
            colours[ImGuiCol_DockingPreview] = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);
#endif
            break;
        case Lumos::ImGuiHelpers::Light:
            ImGui::StyleColorsLight();
            colours[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
            colours[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
            colours[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
            colours[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
            colours[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
            colours[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
            colours[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
            colours[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
            colours[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            colours[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
            colours[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
            colours[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
            colours[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
            colours[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
            colours[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
            colours[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colours[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
            colours[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colours[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
            colours[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colours[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
            colours[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
            colours[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
            colours[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colours[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
            colours[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            colours[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
            colours[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
            colours[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
            colours[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            colours[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            colours[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
            break;
        case Lumos::ImGuiHelpers::Cherry:
            ImGui::StyleColorsDark();
#define HI(v) ImVec4(0.502f, 0.075f, 0.256f, v)
#define MED(v) ImVec4(0.455f, 0.198f, 0.301f, v)
#define LOW(v) ImVec4(0.232f, 0.201f, 0.271f, v)
#define BG(v) ImVec4(0.200f, 0.220f, 0.270f, v)
#define TEXTCol(v) ImVec4(0.860f, 0.930f, 0.890f, v)

            colours[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
            colours[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
            colours[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
            colours[ImGuiCol_PopupBg] = BG(0.9f);
            colours[ImGuiCol_Border] = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
            colours[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colours[ImGuiCol_FrameBg] = BG(1.00f);
            colours[ImGuiCol_FrameBgHovered] = MED(0.78f);
            colours[ImGuiCol_FrameBgActive] = MED(1.00f);
            colours[ImGuiCol_TitleBg] = LOW(1.00f);
            colours[ImGuiCol_TitleBgActive] = HI(1.00f);
            colours[ImGuiCol_TitleBgCollapsed] = BG(0.75f);
            colours[ImGuiCol_MenuBarBg] = BG(0.47f);
            colours[ImGuiCol_ScrollbarBg] = BG(1.00f);
            colours[ImGuiCol_ScrollbarGrab] = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabHovered] = MED(0.78f);
            colours[ImGuiCol_ScrollbarGrabActive] = MED(1.00f);
            colours[ImGuiCol_CheckMark] = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
            colours[ImGuiCol_SliderGrab] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
            colours[ImGuiCol_SliderGrabActive] = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
            colours[ImGuiCol_Button] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
            colours[ImGuiCol_ButtonHovered] = MED(0.86f);
            colours[ImGuiCol_ButtonActive] = MED(1.00f);
            colours[ImGuiCol_Header] = MED(0.76f);
            colours[ImGuiCol_HeaderHovered] = MED(0.86f);
            colours[ImGuiCol_HeaderActive] = HI(1.00f);
            colours[ImGuiCol_ResizeGrip] = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
            colours[ImGuiCol_ResizeGripHovered] = MED(0.78f);
            colours[ImGuiCol_ResizeGripActive] = MED(1.00f);
            colours[ImGuiCol_PlotLines] = TEXTCol(0.63f);
            colours[ImGuiCol_PlotLinesHovered] = MED(1.00f);
            colours[ImGuiCol_PlotHistogram] = TEXTCol(0.63f);
            colours[ImGuiCol_PlotHistogramHovered] = MED(1.00f);
            colours[ImGuiCol_TextSelectedBg] = MED(0.43f);
            colours[ImGuiCol_Border] = ImVec4(0.539f, 0.479f, 0.255f, 0.162f);
            colours[ImGuiCol_TabHovered] = colours[ImGuiCol_ButtonHovered];

            break;
        case Blue:
            colours[ImGuiCol_Text] = ImVec4(colour_for_text.x, colour_for_text.y, colour_for_text.z, 1.00f);
            colours[ImGuiCol_TextDisabled] = ImVec4(colour_for_text.x, colour_for_text.y, colour_for_text.z, 0.58f);
            colours[ImGuiCol_WindowBg] = ImVec4(colour_for_body.x, colour_for_body.y, colour_for_body.z, 0.95f);
            colours[ImGuiCol_Border] = ImVec4(colour_for_body.x, colour_for_body.y, colour_for_body.z, 0.00f);
            colours[ImGuiCol_BorderShadow] = ImVec4(colour_for_body.x, colour_for_body.y, colour_for_body.z, 0.00f);
            colours[ImGuiCol_FrameBg] = ImVec4(colour_for_area.x, colour_for_area.y, colour_for_area.z, 1.00f);
            colours[ImGuiCol_FrameBgHovered] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.78f);
            colours[ImGuiCol_FrameBgActive] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 1.00f);
            colours[ImGuiCol_TitleBg] = ImVec4(colour_for_area.x, colour_for_area.y, colour_for_area.z, 1.00f);
            colours[ImGuiCol_TitleBgCollapsed] = ImVec4(colour_for_area.x, colour_for_area.y, colour_for_area.z, 0.75f);
            colours[ImGuiCol_TitleBgActive] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 1.00f);
            colours[ImGuiCol_MenuBarBg] = ImVec4(colour_for_area.x, colour_for_area.y, colour_for_area.z, 0.47f);
            colours[ImGuiCol_ScrollbarBg] = ImVec4(colour_for_area.x, colour_for_area.y, colour_for_area.z, 1.00f);
            colours[ImGuiCol_ScrollbarGrab] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.21f);
            colours[ImGuiCol_ScrollbarGrabHovered] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.78f);
            colours[ImGuiCol_ScrollbarGrabActive] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 1.00f);
            colours[ImGuiCol_CheckMark] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.80f);
            colours[ImGuiCol_SliderGrab] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.50f);
            colours[ImGuiCol_SliderGrabActive] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 1.00f);
            colours[ImGuiCol_Button] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.50f);
            colours[ImGuiCol_ButtonHovered] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.86f);
            colours[ImGuiCol_ButtonActive] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 1.00f);
            colours[ImGuiCol_Header] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.76f);
            colours[ImGuiCol_HeaderHovered] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.86f);
            colours[ImGuiCol_HeaderActive] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 1.00f);
            colours[ImGuiCol_ResizeGrip] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.15f);
            colours[ImGuiCol_ResizeGripHovered] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.78f);
            colours[ImGuiCol_ResizeGripActive] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 1.00f);
            colours[ImGuiCol_PlotLines] = ImVec4(colour_for_text.x, colour_for_text.y, colour_for_text.z, 0.63f);
            colours[ImGuiCol_PlotLinesHovered] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 1.00f);
            colours[ImGuiCol_PlotHistogram] = ImVec4(colour_for_text.x, colour_for_text.y, colour_for_text.z, 0.63f);
            colours[ImGuiCol_PlotHistogramHovered] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 1.00f);
            colours[ImGuiCol_TextSelectedBg] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.43f);
            colours[ImGuiCol_PopupBg] = ImVec4(colour_for_pops.x, colour_for_pops.y, colour_for_pops.z, 0.92f);
            break;
        case Classic:
            ImGui::StyleColorsClassic();
            break;
        case ClassicDark:
            ImGui::StyleColorsDark();
            break;
        case ClassicLight:
            ImGui::StyleColorsLight();
            break;
        case Cinder:
            colours[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
            colours[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
            colours[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
            colours[ImGuiCol_Border] = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
            colours[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colours[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
            colours[ImGuiCol_FrameBgHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
            colours[ImGuiCol_FrameBgActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
            colours[ImGuiCol_TitleBg] = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
            colours[ImGuiCol_TitleBgCollapsed] = ImVec4(0.20f, 0.22f, 0.27f, 0.75f);
            colours[ImGuiCol_TitleBgActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
            colours[ImGuiCol_MenuBarBg] = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
            colours[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
            colours[ImGuiCol_ScrollbarGrab] = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
            colours[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
            colours[ImGuiCol_CheckMark] = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
            colours[ImGuiCol_SliderGrab] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
            colours[ImGuiCol_SliderGrabActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
            colours[ImGuiCol_Button] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
            colours[ImGuiCol_ButtonHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
            colours[ImGuiCol_ButtonActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
            colours[ImGuiCol_Header] = ImVec4(0.92f, 0.18f, 0.29f, 0.76f);
            colours[ImGuiCol_HeaderHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
            colours[ImGuiCol_HeaderActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
            colours[ImGuiCol_ResizeGrip] = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
            colours[ImGuiCol_ResizeGripHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
            colours[ImGuiCol_ResizeGripActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
            colours[ImGuiCol_PlotLines] = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
            colours[ImGuiCol_PlotLinesHovered] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
            colours[ImGuiCol_PlotHistogram] = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
            colours[ImGuiCol_PlotHistogramHovered] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
            colours[ImGuiCol_TextSelectedBg] = ImVec4(0.92f, 0.18f, 0.29f, 0.43f);
            colours[ImGuiCol_PopupBg] = ImVec4(0.20f, 0.22f, 0.27f, 0.9f);
            break;
        default:
            break;
        }

        colours[ImGuiCol_Separator] = colours[ImGuiCol_TitleBg];
        colours[ImGuiCol_SeparatorActive] = colours[ImGuiCol_Separator];
        colours[ImGuiCol_SeparatorHovered] = colours[ImGuiCol_Separator];

        colours[ImGuiCol_Tab] = colours[ImGuiCol_MenuBarBg];
        colours[ImGuiCol_TabUnfocused] = colours[ImGuiCol_MenuBarBg];

        colours[ImGuiCol_TabUnfocusedActive] = colours[ImGuiCol_WindowBg];
        colours[ImGuiCol_TabActive] = colours[ImGuiCol_WindowBg];
        colours[ImGuiCol_ChildBg] = colours[ImGuiCol_TabActive];
        colours[ImGuiCol_ScrollbarBg] = colours[ImGuiCol_TabActive];

        colours[ImGuiCol_TitleBgActive] = colours[ImGuiCol_TitleBg];
        colours[ImGuiCol_TitleBgCollapsed] = colours[ImGuiCol_TitleBg];
        colours[ImGuiCol_MenuBarBg] = colours[ImGuiCol_TitleBg];
        colours[ImGuiCol_PopupBg] = colours[ImGuiCol_WindowBg] + ImVec4(0.1f, 0.1f, 0.1f, 0.0f);

        colours[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 0.00f);
        colours[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    }

    Maths::Vector4 ImGuiHelpers::GetSelectedColour()
    {
        return SelectedColour;
    }
}

namespace ImGui
{
    bool DragFloatN_Coloured(const char* label, float* v, int components, float v_speed, float v_min, float v_max, const char* display_format, float power)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if(window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        bool value_changed = false;
        BeginGroup();
        PushID(label);
        PushMultiItemsWidths(components, CalcItemWidth());
        for(int i = 0; i < components; i++)
        {
            static const ImU32 colours[] = {
                0xBB0000FF, // red
                0xBB00FF00, // green
                0xBBFF0000, // blue
                0xBBFFFFFF, // white for alpha?
            };

            PushID(i);
            value_changed |= DragFloat("##v", &v[i], v_speed, v_min, v_max, display_format, power);

            const ImVec2 min = GetItemRectMin();
            const ImVec2 max = GetItemRectMax();
            const float spacing = g.Style.FrameRounding;
            const float halfSpacing = spacing / 2;

            // This is the main change
            window->DrawList->AddLine({ min.x + spacing, max.y - halfSpacing }, { max.x - spacing, max.y - halfSpacing }, colours[i], 4);

            SameLine(0, g.Style.ItemInnerSpacing.x);
            PopID();
            PopItemWidth();
        }
        PopID();

        TextUnformatted(label, FindRenderedTextEnd(label));
        EndGroup();

        return value_changed;
    }

    void PushMultiItemsWidthsAndLabels(const char* labels[], int components, float w_full)
    {
        ImGuiWindow* window = GetCurrentWindow();
        const ImGuiStyle& style = GImGui->Style;
        if(w_full <= 0.0f)
            w_full = GetContentRegionAvail().x;

        const float w_item_one = ImMax(1.0f, (w_full - (style.ItemInnerSpacing.x * 2.0f) * (components - 1)) / (float)components) - style.ItemInnerSpacing.x;
        for(int i = 0; i < components; i++)
            window->DC.ItemWidthStack.push_back(w_item_one - CalcTextSize(labels[i]).x);
        window->DC.ItemWidth = window->DC.ItemWidthStack.back();
    }

    bool DragFloatNEx(const char* labels[], float* v, int components, float v_speed, float v_min, float v_max,
        const char* display_format, float power)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if(window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        bool value_changed = false;
        BeginGroup();

        PushMultiItemsWidthsAndLabels(labels, components, 0.0f);
        for(int i = 0; i < components; i++)
        {
            PushID(labels[i]);
            PushID(i);
            TextUnformatted(labels[i], FindRenderedTextEnd(labels[i]));
            SameLine();
            value_changed |= DragFloat("", &v[i], v_speed, v_min, v_max, display_format, power);
            SameLine(0, g.Style.ItemInnerSpacing.x);
            PopID();
            PopID();
            PopItemWidth();
        }

        EndGroup();

        return value_changed;
    }

    bool DragFloat3Coloured(const char* label, float* v, float v_speed, float v_min, float v_max)
    {
        return DragFloatN_Coloured(label, v, 3, v_speed, v_min, v_max);
    }

    bool DragFloat4Coloured(const char* label, float* v, float v_speed, float v_min, float v_max)
    {
        return DragFloatN_Coloured(label, v, 4, v_speed, v_min, v_max);
    }

    bool DragFloat2Coloured(const char* label, float* v, float v_speed, float v_min, float v_max)
    {
        return DragFloatN_Coloured(label, v, 2, v_speed, v_min, v_max);
    }
}
