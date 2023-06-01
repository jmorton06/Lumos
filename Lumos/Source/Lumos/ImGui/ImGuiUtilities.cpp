#include "Precompiled.h"
#include "ImGui/ImGuiUtilities.h"
#include "Graphics/RHI/Renderer.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/GraphicsContext.h"
#include "Core/OS/OS.h"
#include "Core/Application.h"
#include "Core/OS/Input.h"
#include <imgui/imgui_internal.h>

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKTexture.h"
#endif

namespace Lumos
{
    glm::vec4 SelectedColour       = glm::vec4(0.28f, 0.56f, 0.9f, 1.0f);
    glm::vec4 IconColour           = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    static char* s_MultilineBuffer = nullptr;
    static uint32_t s_Counter      = 0;
    static char s_IDBuffer[16]     = "##";
    static char s_LabelIDBuffer[1024];
    static int s_UIContextID = 0;

    const char* ImGuiUtilities::GenerateID()
    {
        sprintf(s_IDBuffer + 2, "%x", s_Counter++);
        //_itoa(s_Counter++, s_IDBuffer + 2, 16);
        return s_IDBuffer;
    }

    const char* ImGuiUtilities::GenerateLabelID(std::string_view label)
    {
        *fmt::format_to_n(s_LabelIDBuffer, std::size(s_LabelIDBuffer), "{}##{}", label, s_Counter++).out = 0;
        return s_LabelIDBuffer;
    }

    void ImGuiUtilities::PushID()
    {
        ImGui::PushID(s_UIContextID++);
        s_Counter = 0;
    }

    void ImGuiUtilities::PopID()
    {
        ImGui::PopID();
        s_UIContextID--;
    }

    bool ImGuiUtilities::Property(const char* name, std::string& value, PropertyFlag flags)
    {
        LUMOS_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::AlignTextToFramePadding();

        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::TextUnformatted(value.c_str());
        }
        else
        {
            if(ImGuiUtilities::InputText(value))
            {
                updated = true;
            }
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    void ImGuiUtilities::PropertyConst(const char* name, const char* value)
    {
        LUMOS_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::AlignTextToFramePadding();
        {
            ImGui::TextUnformatted(value);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
    }

    bool ImGuiUtilities::PropertyMultiline(const char* label, std::string& value)
    {
        bool modified = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::AlignTextToFramePadding();

        if(!s_MultilineBuffer)
        {
            s_MultilineBuffer = new char[1024 * 1024]; // 1KB
            memset(s_MultilineBuffer, 0, 1024 * 1024);
        }

        strcpy(s_MultilineBuffer, value.c_str());

        // std::string id = "##" + label;
        if(ImGui::InputTextMultiline(GenerateID(), s_MultilineBuffer, 1024 * 1024))
        {
            value    = s_MultilineBuffer;
            modified = true;
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return modified;
    }

    bool ImGuiUtilities::Property(const char* name, bool& value, PropertyFlag flags)
    {
        LUMOS_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::TextUnformatted(value ? "True" : "False");
        }
        else
        {
            // std::string id = "##" + std::string(name);
            if(ImGui::Checkbox(GenerateID(), &value))
                updated = true;
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiUtilities::Property(const char* name, int& value, ImGuiUtilities::PropertyFlag flags)
    {
        LUMOS_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%i", value);
        }
        else
        {
            // std::string id = "##" + name;
            if(ImGui::DragInt(GenerateID(), &value))
                updated = true;
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiUtilities::Property(const char* name, uint32_t& value, ImGuiUtilities::PropertyFlag flags)
    {
        LUMOS_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::Text("%uui", value);
        }
        else
        {
            // std::string id = "##" + name;
            int valueInt = (int)value;
            if(ImGui::DragInt(GenerateID(), &valueInt))
            {
                updated = true;
                value   = (uint32_t)valueInt;
            }
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiUtilities::Property(const char* name, float& value, float min, float max, float delta, ImGuiUtilities::PropertyFlag flags)
    {
        LUMOS_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::Text("%.2f", value);
        }
        else
        {
            // std::string id = "##" + name;
            if(ImGui::DragFloat(GenerateID(), &value, delta, min, max))
                updated = true;
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiUtilities::Property(const char* name, double& value, double min, double max, PropertyFlag flags)
    {
        LUMOS_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::Text("%.2f", (float)value);
        }
        else
        {
            // std::string id = "##" + name;
            if(ImGui::DragScalar(GenerateID(), ImGuiDataType_Double, &value))
                updated = true;
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiUtilities::Property(const char* name, int& value, int min, int max, PropertyFlag flags)
    {
        LUMOS_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::Text("%i", value);
        }
        else
        {
            // std::string id = "##" + name;
            if(ImGui::DragInt(GenerateID(), &value, 1, min, max))
                updated = true;
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiUtilities::Property(const char* name, glm::vec2& value, ImGuiUtilities::PropertyFlag flags)
    {
        LUMOS_PROFILE_FUNCTION();
        return ImGuiUtilities::Property(name, value, -1.0f, 1.0f, flags);
    }

    bool ImGuiUtilities::Property(const char* name, glm::vec2& value, float min, float max, ImGuiUtilities::PropertyFlag flags)
    {
        LUMOS_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::Text("%.2f , %.2f", value.x, value.y);
        }
        else
        {
            // std::string id = "##" + name;
            if(ImGui::DragFloat2(GenerateID(), glm::value_ptr(value)))
                updated = true;
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiUtilities::Property(const char* name, glm::vec3& value, ImGuiUtilities::PropertyFlag flags)
    {
        LUMOS_PROFILE_FUNCTION();
        return ImGuiUtilities::Property(name, value, -1.0f, 1.0f, flags);
    }

    bool ImGuiUtilities::Property(const char* name, glm::vec3& value, float min, float max, ImGuiUtilities::PropertyFlag flags)
    {
        LUMOS_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::Text("%.2f , %.2f, %.2f", value.x, value.y, value.z);
        }
        else
        {
            // std::string id = "##" + name;
            if((int)flags & (int)PropertyFlag::ColourProperty)
            {
                if(ImGui::ColorEdit3(GenerateID(), glm::value_ptr(value)))
                    updated = true;
            }
            else
            {
                if(ImGui::DragFloat3(GenerateID(), glm::value_ptr(value)))
                    updated = true;
            }
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiUtilities::Property(const char* name, glm::vec4& value, bool exposeW, ImGuiUtilities::PropertyFlag flags)
    {
        LUMOS_PROFILE_FUNCTION();
        return Property(name, value, -1.0f, 1.0f, exposeW, flags);
    }

    bool ImGuiUtilities::Property(const char* name, glm::vec4& value, float min, float max, bool exposeW, ImGuiUtilities::PropertyFlag flags)
    {
        LUMOS_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::Text("%.2f , %.2f, %.2f , %.2f", value.x, value.y, value.z, value.w);
        }
        else
        {

            // std::string id = "##" + name;
            if((int)flags & (int)PropertyFlag::ColourProperty)
            {
                if(ImGui::ColorEdit4(GenerateID(), glm::value_ptr(value)))
                    updated = true;
            }
            else if((exposeW ? ImGui::DragFloat4(GenerateID(), glm::value_ptr(value)) : ImGui::DragFloat4(GenerateID(), glm::value_ptr(value))))
                updated = true;
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiUtilities::PorpertyTransform(const char* name, glm::vec3& vector, float width)
    {
        const float labelIndetation = ImGui::GetFontSize();
        bool updated                = false;

        auto& style = ImGui::GetStyle();

        const auto showFloat = [&](int axis, float* value)
        {
            const float label_float_spacing = ImGui::GetFontSize();
            const float step                = 0.01f;
            static const std::string format = "%.4f";

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(axis == 0 ? "X" : axis == 1 ? "Y"
                                                               : "Z");
            ImGui::SameLine(label_float_spacing);
            glm::vec2 posPostLabel = ImGui::GetCursorScreenPos();

            ImGui::PushItemWidth(width);
            ImGui::PushID(static_cast<int>(ImGui::GetCursorPosX() + ImGui::GetCursorPosY()));

            if(ImGui::InputFloat("##no_label", value, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max(), format.c_str()))
                updated = true;

            ImGui::PopID();
            ImGui::PopItemWidth();

            static const ImU32 colourX = IM_COL32(168, 46, 2, 255);
            static const ImU32 colourY = IM_COL32(112, 162, 22, 255);
            static const ImU32 colourZ = IM_COL32(51, 122, 210, 255);

            const glm::vec2 size   = glm::vec2(ImGui::GetFontSize() / 4.0f, ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y);
            posPostLabel           = posPostLabel + glm::vec2(-1.0f, ImGui::GetStyle().FramePadding.y / 2.0f);
            ImRect axis_color_rect = ImRect(posPostLabel.x, posPostLabel.y, posPostLabel.x + size.x, posPostLabel.y + size.y);
            ImGui::GetWindowDrawList()->AddRectFilled(axis_color_rect.Min, axis_color_rect.Max, axis == 0 ? colourX : axis == 1 ? colourY
                                                                                                                                : colourZ);
        };

        ImGui::BeginGroup();
        ImGui::Indent(labelIndetation);
        ImGui::TextUnformatted(name);
        ImGui::Unindent(labelIndetation);
        showFloat(0, &vector.x);
        showFloat(1, &vector.y);
        showFloat(2, &vector.z);
        ImGui::EndGroup();

        return updated;
    }

    bool ImGuiUtilities::Property(const char* name, glm::quat& value, ImGuiUtilities::PropertyFlag flags)
    {
        LUMOS_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::Text("%.2f , %.2f, %.2f , %.2f", value.x, value.y, value.z, value.w);
        }
        else
        {

            // std::string id = "##" + name;
            if(ImGui::DragFloat4(GenerateID(), glm::value_ptr(value)))
                updated = true;
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    void ImGuiUtilities::Tooltip(const char* text)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

        if(ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(text);
            ImGui::EndTooltip();
        }

        ImGui::PopStyleVar();
    }

    void ImGuiUtilities::Tooltip(Graphics::Texture2D* texture, const glm::vec2& size)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

        if(ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();
            ImGui::Image(texture ? texture->GetHandle() : nullptr, ImVec2(size.x, size.y), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
            ImGui::EndTooltip();
        }

        ImGui::PopStyleVar();
    }

    void ImGuiUtilities::Tooltip(Graphics::Texture2D* texture, const glm::vec2& size, const char* text)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

        if(ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();
            ImGui::Image(texture ? texture->GetHandle() : nullptr, ImVec2(size.x, size.y), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
            ImGui::TextUnformatted(text);
            ImGui::EndTooltip();
        }

        ImGui::PopStyleVar();
    }

    void ImGuiUtilities::Tooltip(Graphics::TextureDepthArray* texture, uint32_t index, const glm::vec2& size)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

        if(ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();

            ImTextureID texID = texture ? texture->GetHandle() : nullptr;
#ifdef LUMOS_RENDER_API_VULKAN
            if(texture && Graphics::GraphicsContext::GetRenderAPI() == Graphics::RenderAPI::VULKAN)
                texID = ((Graphics::VKTextureDepthArray*)texture)->GetImageView(index);
#endif

            bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();
            ImGui::Image(texID, ImVec2(size.x, size.y), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
            ImGui::EndTooltip();
        }

        ImGui::PopStyleVar();
    }

    void ImGuiUtilities::Image(Graphics::Texture2D* texture, const glm::vec2& size)
    {
        LUMOS_PROFILE_FUNCTION();
        bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();
        ImGui::Image(texture ? texture->GetHandle() : nullptr, ImVec2(size.x, size.y), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
    }

    void ImGuiUtilities::Image(Graphics::TextureCube* texture, const glm::vec2& size)
    {
        LUMOS_PROFILE_FUNCTION();
        bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();
        ImGui::Image(texture ? texture->GetHandle() : nullptr, ImVec2(size.x, size.y), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
    }

    void ImGuiUtilities::Image(Graphics::TextureDepthArray* texture, uint32_t index, const glm::vec2& size)
    {
        LUMOS_PROFILE_FUNCTION();
        bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();

        ImTextureID texID = texture ? texture->GetHandle() : nullptr;
#ifdef LUMOS_RENDER_API_VULKAN
        if(texture && Graphics::GraphicsContext::GetRenderAPI() == Graphics::RenderAPI::VULKAN)
            texID = ((Graphics::VKTextureDepthArray*)texture)->GetImageView(index);
#endif
        ImGui::Image(texID, ImVec2(size.x, size.y), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
    }

    bool ImGuiUtilities::BufferingBar(const char* label, float value, const glm::vec2& size_arg, const uint32_t& bg_col, const uint32_t& fg_col)
    {
        LUMOS_PROFILE_FUNCTION();
        auto g                  = ImGui::GetCurrentContext();
        auto drawList           = ImGui::GetWindowDrawList();
        const ImGuiStyle& style = ImGui::GetStyle();
        const ImGuiID id        = ImGui::GetID(label);

        ImVec2 pos  = ImGui::GetCursorPos();
        ImVec2 size = { size_arg.x, size_arg.y };
        size.x -= style.FramePadding.x * 2;

        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb, style.FramePadding.y);
        if(!ImGui::ItemAdd(bb, id))
            return false;

        // Render
        const float circleStart = size.x * 0.7f;
        const float circleEnd   = size.x;
        const float circleWidth = circleEnd - circleStart;

        drawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart, bb.Max.y), bg_col);
        drawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart * value, bb.Max.y), fg_col);

        const float t     = float(g->Time);
        const float r     = size.y * 0.5f;
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

    bool ImGuiUtilities::Spinner(const char* label, float radius, int thickness, const uint32_t& colour)
    {
        LUMOS_PROFILE_FUNCTION();
        auto g                  = ImGui::GetCurrentContext();
        const ImGuiStyle& style = g->Style;
        const ImGuiID id        = ImGui::GetID(label);
        auto drawList           = ImGui::GetWindowDrawList();

        ImVec2 pos = ImGui::GetCursorPos();
        ImVec2 size((radius)*2, (radius + style.FramePadding.y) * 2);

        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb, style.FramePadding.y);
        if(!ImGui::ItemAdd(bb, id))
            return false;

        // Render
        drawList->PathClear();

        int num_segments = 30;
        float start      = abs(ImSin(float(g->Time) * 1.8f) * (num_segments - 5));

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

    void ImGuiUtilities::DrawRowsBackground(int row_count, float line_height, float x1, float x2, float y_offset, ImU32 col_even, ImU32 col_odd)
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        float y0              = ImGui::GetCursorScreenPos().y + (float)(int)y_offset;

        int row_display_start = 0;
        int row_display_end   = 0;
        // ImGui::CalcListClipping(row_count, line_height, &row_display_start, &row_display_end);
        for(int row_n = row_display_start; row_n < row_display_end; row_n++)
        {
            ImU32 col = (row_n & 1) ? col_odd : col_even;
            if((col & IM_COL32_A_MASK) == 0)
                continue;
            float y1 = y0 + (line_height * row_n);
            float y2 = y1 + line_height;
            draw_list->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), col);
        }
    }

    void ImGuiUtilities::TextCentred(const char* text)
    {
        auto windowWidth = ImGui::GetWindowSize().x;
        auto textWidth   = ImGui::CalcTextSize(text).x;

        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::TextUnformatted(text);
    }

    void ImGuiUtilities::SetTheme(Theme theme)
    {
        static const float max = 255.0f;

        auto& style     = ImGui::GetStyle();
        ImVec4* colours = style.Colors;
        SelectedColour  = glm::vec4(0.28f, 0.56f, 0.9f, 1.0f);

        // colours[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        // colours[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);

        if(theme == Black)
        {
            ImGui::StyleColorsDark();
            colours[ImGuiCol_Text]                  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            colours[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
            colours[ImGuiCol_WindowBg]              = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
            colours[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colours[ImGuiCol_PopupBg]               = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
            colours[ImGuiCol_Border]                = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
            colours[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
            colours[ImGuiCol_FrameBg]               = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
            colours[ImGuiCol_FrameBgHovered]        = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
            colours[ImGuiCol_FrameBgActive]         = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
            colours[ImGuiCol_TitleBg]               = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
            colours[ImGuiCol_TitleBgActive]         = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
            colours[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
            colours[ImGuiCol_MenuBarBg]             = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
            colours[ImGuiCol_ScrollbarBg]           = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
            colours[ImGuiCol_ScrollbarGrab]         = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
            colours[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
            colours[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
            colours[ImGuiCol_CheckMark]             = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
            colours[ImGuiCol_SliderGrab]            = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
            colours[ImGuiCol_SliderGrabActive]      = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
            colours[ImGuiCol_Button]                = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
            colours[ImGuiCol_ButtonHovered]         = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
            colours[ImGuiCol_ButtonActive]          = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
            colours[ImGuiCol_Header]                = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
            colours[ImGuiCol_HeaderHovered]         = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
            colours[ImGuiCol_HeaderActive]          = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
            colours[ImGuiCol_Separator]             = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
            colours[ImGuiCol_SeparatorHovered]      = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
            colours[ImGuiCol_SeparatorActive]       = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
            colours[ImGuiCol_ResizeGrip]            = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
            colours[ImGuiCol_ResizeGripHovered]     = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
            colours[ImGuiCol_ResizeGripActive]      = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
            colours[ImGuiCol_Tab]                   = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
            colours[ImGuiCol_TabHovered]            = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
            colours[ImGuiCol_TabActive]             = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
            colours[ImGuiCol_TabUnfocused]          = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
            colours[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
            colours[ImGuiCol_DockingPreview]        = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
            colours[ImGuiCol_DockingEmptyBg]        = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            colours[ImGuiCol_PlotLines]             = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            colours[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            colours[ImGuiCol_PlotHistogram]         = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            colours[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            colours[ImGuiCol_TableHeaderBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
            colours[ImGuiCol_TableBorderStrong]     = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
            colours[ImGuiCol_TableBorderLight]      = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
            colours[ImGuiCol_TextSelectedBg]        = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
            colours[ImGuiCol_DragDropTarget]        = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
            colours[ImGuiCol_NavHighlight]          = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            colours[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
            colours[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
            colours[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
        }
        else if(theme == Dark)
        {
            ImGui::StyleColorsDark();
            ImVec4 Titlebar    = ImVec4(40.0f / max, 42.0f / max, 54.0f / max, 1.0f);
            ImVec4 TabActive   = ImVec4(52.0f / max, 54.0f / max, 64.0f / max, 1.0f);
            ImVec4 TabUnactive = ImVec4(35.0f / max, 43.0f / max, 59.0f / max, 1.0f);

            SelectedColour                 = ImVec4(155.0f / 255.0f, 130.0f / 255.0f, 207.0f / 255.0f, 1.00f);
            colours[ImGuiCol_Text]         = ImVec4(200.0f / 255.0f, 200.0f / 255.0f, 200.0f / 255.0f, 1.00f);
            colours[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);

            IconColour                 = colours[ImGuiCol_Text];
            colours[ImGuiCol_WindowBg] = TabActive;
            colours[ImGuiCol_ChildBg]  = TabActive;

            colours[ImGuiCol_PopupBg]        = ImVec4(42.0f / 255.0f, 38.0f / 255.0f, 47.0f / 255.0f, 1.00f);
            colours[ImGuiCol_Border]         = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
            colours[ImGuiCol_BorderShadow]   = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colours[ImGuiCol_FrameBg]        = ImVec4(65.0f / 255.0f, 79.0f / 255.0f, 92.0f / 255.0f, 1.00f);
            colours[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
            colours[ImGuiCol_FrameBgActive]  = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);

            colours[ImGuiCol_TitleBg]          = Titlebar;
            colours[ImGuiCol_TitleBgActive]    = Titlebar;
            colours[ImGuiCol_TitleBgCollapsed] = Titlebar;
            colours[ImGuiCol_MenuBarBg]        = Titlebar;

            colours[ImGuiCol_ScrollbarBg]          = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
            colours[ImGuiCol_ScrollbarGrab]        = ImVec4(0.6f, 0.6f, 0.6f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.7f, 0.7f, 0.7f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.8f, 0.8f, 0.8f, 1.00f);

            colours[ImGuiCol_CheckMark]        = ImVec4(155.0f / 255.0f, 130.0f / 255.0f, 207.0f / 255.0f, 1.00f);
            colours[ImGuiCol_SliderGrab]       = ImVec4(155.0f / 255.0f, 130.0f / 255.0f, 207.0f / 255.0f, 1.00f);
            colours[ImGuiCol_SliderGrabActive] = ImVec4(185.0f / 255.0f, 160.0f / 255.0f, 237.0f / 255.0f, 1.00f);
            colours[ImGuiCol_Button]           = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
            colours[ImGuiCol_ButtonHovered]    = ImVec4(0.20f, 0.25f, 0.29f, 1.00f) + ImVec4(0.1f, 0.1f, 0.1f, 0.1f);
            colours[ImGuiCol_ButtonActive]     = ImVec4(0.20f, 0.25f, 0.29f, 1.00f) + ImVec4(0.1f, 0.1f, 0.1f, 0.1f);

            colours[ImGuiCol_Separator]        = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
            colours[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
            colours[ImGuiCol_SeparatorActive]  = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);

            colours[ImGuiCol_ResizeGrip]        = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
            colours[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            colours[ImGuiCol_ResizeGripActive]  = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);

            colours[ImGuiCol_PlotLines]             = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
            colours[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
            colours[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            colours[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            colours[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
            colours[ImGuiCol_DragDropTarget]        = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colours[ImGuiCol_NavHighlight]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colours[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
            colours[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
            colours[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

            colours[ImGuiCol_Header]        = TabActive + ImVec4(0.1f, 0.1f, 0.1f, 0.1f);
            colours[ImGuiCol_HeaderHovered] = TabActive + ImVec4(0.1f, 0.1f, 0.1f, 0.1f);
            colours[ImGuiCol_HeaderActive]  = TabActive + ImVec4(0.05f, 0.05f, 0.05f, 0.1f);

#ifdef IMGUI_HAS_DOCK

            colours[ImGuiCol_Tab]                = TabUnactive;
            colours[ImGuiCol_TabHovered]         = TabActive + ImVec4(0.1f, 0.1f, 0.1f, 0.1f);
            colours[ImGuiCol_TabActive]          = TabActive;
            colours[ImGuiCol_TabUnfocused]       = TabUnactive;
            colours[ImGuiCol_TabUnfocusedActive] = TabActive;
            colours[ImGuiCol_DockingEmptyBg]     = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
            colours[ImGuiCol_DockingPreview]     = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);

#endif
        }
        else if(theme == Grey)
        {
            ImGui::StyleColorsDark();
            colours[ImGuiCol_Text]         = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            colours[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
            IconColour                     = colours[ImGuiCol_Text];

            colours[ImGuiCol_ChildBg]               = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colours[ImGuiCol_WindowBg]              = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colours[ImGuiCol_PopupBg]               = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colours[ImGuiCol_Border]                = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
            colours[ImGuiCol_BorderShadow]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
            colours[ImGuiCol_FrameBg]               = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
            colours[ImGuiCol_FrameBgHovered]        = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
            colours[ImGuiCol_FrameBgActive]         = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
            colours[ImGuiCol_TitleBg]               = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
            colours[ImGuiCol_TitleBgActive]         = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
            colours[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
            colours[ImGuiCol_MenuBarBg]             = ImVec4(0.335f, 0.335f, 0.335f, 1.000f);
            colours[ImGuiCol_ScrollbarBg]           = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
            colours[ImGuiCol_ScrollbarGrab]         = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
            colours[ImGuiCol_CheckMark]             = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
            colours[ImGuiCol_SliderGrab]            = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
            colours[ImGuiCol_SliderGrabActive]      = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
            colours[ImGuiCol_Button]                = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
            colours[ImGuiCol_ButtonHovered]         = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
            colours[ImGuiCol_ButtonActive]          = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
            colours[ImGuiCol_Header]                = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
            colours[ImGuiCol_HeaderHovered]         = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
            colours[ImGuiCol_HeaderActive]          = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
            colours[ImGuiCol_Separator]             = ImVec4(0.000f, 0.000f, 0.000f, 0.137f);
            colours[ImGuiCol_SeparatorHovered]      = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
            colours[ImGuiCol_SeparatorActive]       = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
            colours[ImGuiCol_ResizeGrip]            = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
            colours[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            colours[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
            colours[ImGuiCol_PlotLines]             = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
            colours[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
            colours[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            colours[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            colours[ImGuiCol_TextSelectedBg]        = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
            colours[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
            colours[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
            colours[ImGuiCol_NavHighlight]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colours[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
            colours[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

#ifdef IMGUI_HAS_DOCK
            colours[ImGuiCol_DockingEmptyBg]     = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
            colours[ImGuiCol_Tab]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colours[ImGuiCol_TabHovered]         = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
            colours[ImGuiCol_TabActive]          = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
            colours[ImGuiCol_TabUnfocused]       = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colours[ImGuiCol_TabUnfocusedActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
            colours[ImGuiCol_DockingPreview]     = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);
#endif
        }
        else if(theme == Light)
        {
            ImGui::StyleColorsLight();
            colours[ImGuiCol_Text]         = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
            colours[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
            IconColour                     = colours[ImGuiCol_Text];

            colours[ImGuiCol_WindowBg]             = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
            colours[ImGuiCol_PopupBg]              = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
            colours[ImGuiCol_Border]               = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
            colours[ImGuiCol_BorderShadow]         = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
            colours[ImGuiCol_FrameBg]              = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
            colours[ImGuiCol_FrameBgHovered]       = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
            colours[ImGuiCol_FrameBgActive]        = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            colours[ImGuiCol_TitleBg]              = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
            colours[ImGuiCol_TitleBgCollapsed]     = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
            colours[ImGuiCol_TitleBgActive]        = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
            colours[ImGuiCol_MenuBarBg]            = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
            colours[ImGuiCol_ScrollbarBg]          = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
            colours[ImGuiCol_ScrollbarGrab]        = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
            colours[ImGuiCol_CheckMark]            = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colours[ImGuiCol_SliderGrab]           = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
            colours[ImGuiCol_SliderGrabActive]     = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colours[ImGuiCol_Button]               = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
            colours[ImGuiCol_ButtonHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colours[ImGuiCol_ButtonActive]         = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
            colours[ImGuiCol_Header]               = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
            colours[ImGuiCol_HeaderHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
            colours[ImGuiCol_HeaderActive]         = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colours[ImGuiCol_ResizeGrip]           = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
            colours[ImGuiCol_ResizeGripHovered]    = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            colours[ImGuiCol_ResizeGripActive]     = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
            colours[ImGuiCol_PlotLines]            = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
            colours[ImGuiCol_PlotLinesHovered]     = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
            colours[ImGuiCol_PlotHistogram]        = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            colours[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            colours[ImGuiCol_TextSelectedBg]       = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        }
        else if(theme == Cherry)
        {
            ImGui::StyleColorsDark();
#define HI(v) ImVec4(0.502f, 0.075f, 0.256f, v)
#define MED(v) ImVec4(0.455f, 0.198f, 0.301f, v)
#define LOW(v) ImVec4(0.232f, 0.201f, 0.271f, v)
#define BG(v) ImVec4(0.200f, 0.220f, 0.270f, v)
#define TEXTCol(v) ImVec4(0.860f, 0.930f, 0.890f, v)

            colours[ImGuiCol_Text]         = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
            colours[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
            IconColour                     = colours[ImGuiCol_Text];

            colours[ImGuiCol_WindowBg]             = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
            colours[ImGuiCol_PopupBg]              = BG(0.9f);
            colours[ImGuiCol_Border]               = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
            colours[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colours[ImGuiCol_FrameBg]              = BG(1.00f);
            colours[ImGuiCol_FrameBgHovered]       = MED(0.78f);
            colours[ImGuiCol_FrameBgActive]        = MED(1.00f);
            colours[ImGuiCol_TitleBg]              = LOW(1.00f);
            colours[ImGuiCol_TitleBgActive]        = HI(1.00f);
            colours[ImGuiCol_TitleBgCollapsed]     = BG(0.75f);
            colours[ImGuiCol_MenuBarBg]            = BG(0.47f);
            colours[ImGuiCol_ScrollbarBg]          = BG(1.00f);
            colours[ImGuiCol_ScrollbarGrab]        = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabHovered] = MED(0.78f);
            colours[ImGuiCol_ScrollbarGrabActive]  = MED(1.00f);
            colours[ImGuiCol_CheckMark]            = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
            colours[ImGuiCol_SliderGrab]           = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
            colours[ImGuiCol_SliderGrabActive]     = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
            colours[ImGuiCol_Button]               = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
            colours[ImGuiCol_ButtonHovered]        = MED(0.86f);
            colours[ImGuiCol_ButtonActive]         = MED(1.00f);
            colours[ImGuiCol_Header]               = MED(0.76f);
            colours[ImGuiCol_HeaderHovered]        = MED(0.86f);
            colours[ImGuiCol_HeaderActive]         = HI(1.00f);
            colours[ImGuiCol_ResizeGrip]           = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
            colours[ImGuiCol_ResizeGripHovered]    = MED(0.78f);
            colours[ImGuiCol_ResizeGripActive]     = MED(1.00f);
            colours[ImGuiCol_PlotLines]            = TEXTCol(0.63f);
            colours[ImGuiCol_PlotLinesHovered]     = MED(1.00f);
            colours[ImGuiCol_PlotHistogram]        = TEXTCol(0.63f);
            colours[ImGuiCol_PlotHistogramHovered] = MED(1.00f);
            colours[ImGuiCol_TextSelectedBg]       = MED(0.43f);
            colours[ImGuiCol_Border]               = ImVec4(0.539f, 0.479f, 0.255f, 0.162f);
            colours[ImGuiCol_TabHovered]           = colours[ImGuiCol_ButtonHovered];
        }
        else if(theme == Blue)
        {
            ImVec4 colour_for_text         = ImVec4(236.f / 255.f, 240.f / 255.f, 241.f / 255.f, 1.0f);
            ImVec4 colour_for_head         = ImVec4(41.f / 255.f, 128.f / 255.f, 185.f / 255.f, 1.0f);
            ImVec4 colour_for_area         = ImVec4(57.f / 255.f, 79.f / 255.f, 105.f / 255.f, 1.0f);
            ImVec4 colour_for_body         = ImVec4(44.f / 255.f, 62.f / 255.f, 80.f / 255.f, 1.0f);
            ImVec4 colour_for_pops         = ImVec4(33.f / 255.f, 46.f / 255.f, 60.f / 255.f, 1.0f);
            colours[ImGuiCol_Text]         = ImVec4(colour_for_text.x, colour_for_text.y, colour_for_text.z, 1.00f);
            colours[ImGuiCol_TextDisabled] = ImVec4(colour_for_text.x, colour_for_text.y, colour_for_text.z, 0.58f);
            IconColour                     = colours[ImGuiCol_Text];

            colours[ImGuiCol_WindowBg]             = ImVec4(colour_for_body.x, colour_for_body.y, colour_for_body.z, 0.95f);
            colours[ImGuiCol_Border]               = ImVec4(colour_for_body.x, colour_for_body.y, colour_for_body.z, 0.00f);
            colours[ImGuiCol_BorderShadow]         = ImVec4(colour_for_body.x, colour_for_body.y, colour_for_body.z, 0.00f);
            colours[ImGuiCol_FrameBg]              = ImVec4(colour_for_area.x, colour_for_area.y, colour_for_area.z, 1.00f);
            colours[ImGuiCol_FrameBgHovered]       = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.78f);
            colours[ImGuiCol_FrameBgActive]        = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 1.00f);
            colours[ImGuiCol_TitleBg]              = ImVec4(colour_for_area.x, colour_for_area.y, colour_for_area.z, 1.00f);
            colours[ImGuiCol_TitleBgCollapsed]     = ImVec4(colour_for_area.x, colour_for_area.y, colour_for_area.z, 0.75f);
            colours[ImGuiCol_TitleBgActive]        = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 1.00f);
            colours[ImGuiCol_MenuBarBg]            = ImVec4(colour_for_area.x, colour_for_area.y, colour_for_area.z, 0.47f);
            colours[ImGuiCol_ScrollbarBg]          = ImVec4(colour_for_area.x, colour_for_area.y, colour_for_area.z, 1.00f);
            colours[ImGuiCol_ScrollbarGrab]        = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.21f);
            colours[ImGuiCol_ScrollbarGrabHovered] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.78f);
            colours[ImGuiCol_ScrollbarGrabActive]  = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 1.00f);
            colours[ImGuiCol_CheckMark]            = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.80f);
            colours[ImGuiCol_SliderGrab]           = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.50f);
            colours[ImGuiCol_SliderGrabActive]     = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 1.00f);
            colours[ImGuiCol_Button]               = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.50f);
            colours[ImGuiCol_ButtonHovered]        = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.86f);
            colours[ImGuiCol_ButtonActive]         = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 1.00f);
            colours[ImGuiCol_Header]               = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.76f);
            colours[ImGuiCol_HeaderHovered]        = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.86f);
            colours[ImGuiCol_HeaderActive]         = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 1.00f);
            colours[ImGuiCol_ResizeGrip]           = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.15f);
            colours[ImGuiCol_ResizeGripHovered]    = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.78f);
            colours[ImGuiCol_ResizeGripActive]     = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 1.00f);
            colours[ImGuiCol_PlotLines]            = ImVec4(colour_for_text.x, colour_for_text.y, colour_for_text.z, 0.63f);
            colours[ImGuiCol_PlotLinesHovered]     = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 1.00f);
            colours[ImGuiCol_PlotHistogram]        = ImVec4(colour_for_text.x, colour_for_text.y, colour_for_text.z, 0.63f);
            colours[ImGuiCol_PlotHistogramHovered] = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 1.00f);
            colours[ImGuiCol_TextSelectedBg]       = ImVec4(colour_for_head.x, colour_for_head.y, colour_for_head.z, 0.43f);
            colours[ImGuiCol_PopupBg]              = ImVec4(colour_for_pops.x, colour_for_pops.y, colour_for_pops.z, 0.92f);
        }
        else if(theme == Classic)
        {
            ImGui::StyleColorsClassic();
            IconColour = colours[ImGuiCol_Text];
        }
        else if(theme == ClassicDark)
        {
            ImGui::StyleColorsDark();
            IconColour = colours[ImGuiCol_Text];
        }
        else if(theme == ClassicLight)
        {
            ImGui::StyleColorsLight();
            IconColour = colours[ImGuiCol_Text];
        }
        else if(theme == Cinder)
        {
            colours[ImGuiCol_Text]                 = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
            colours[ImGuiCol_TextDisabled]         = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
            IconColour                             = colours[ImGuiCol_Text];
            colours[ImGuiCol_WindowBg]             = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
            colours[ImGuiCol_Border]               = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
            colours[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colours[ImGuiCol_FrameBg]              = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
            colours[ImGuiCol_FrameBgHovered]       = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
            colours[ImGuiCol_FrameBgActive]        = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
            colours[ImGuiCol_TitleBg]              = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
            colours[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.20f, 0.22f, 0.27f, 0.75f);
            colours[ImGuiCol_TitleBgActive]        = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
            colours[ImGuiCol_MenuBarBg]            = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
            colours[ImGuiCol_ScrollbarBg]          = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
            colours[ImGuiCol_ScrollbarGrab]        = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
            colours[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
            colours[ImGuiCol_CheckMark]            = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
            colours[ImGuiCol_SliderGrab]           = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
            colours[ImGuiCol_SliderGrabActive]     = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
            colours[ImGuiCol_Button]               = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
            colours[ImGuiCol_ButtonHovered]        = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
            colours[ImGuiCol_ButtonActive]         = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
            colours[ImGuiCol_Header]               = ImVec4(0.92f, 0.18f, 0.29f, 0.76f);
            colours[ImGuiCol_HeaderHovered]        = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
            colours[ImGuiCol_HeaderActive]         = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
            colours[ImGuiCol_ResizeGrip]           = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
            colours[ImGuiCol_ResizeGripHovered]    = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
            colours[ImGuiCol_ResizeGripActive]     = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
            colours[ImGuiCol_PlotLines]            = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
            colours[ImGuiCol_PlotLinesHovered]     = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
            colours[ImGuiCol_PlotHistogram]        = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
            colours[ImGuiCol_PlotHistogramHovered] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
            colours[ImGuiCol_TextSelectedBg]       = ImVec4(0.92f, 0.18f, 0.29f, 0.43f);
            colours[ImGuiCol_PopupBg]              = ImVec4(0.20f, 0.22f, 0.27f, 0.9f);
        }
        else if(theme == Dracula)
        {
            ImGui::StyleColorsDark();

            ImVec4 Titlebar    = ImVec4(33.0f / max, 34.0f / max, 44.0f / max, 1.0f);
            ImVec4 TabActive   = ImVec4(40.0f / max, 42.0f / max, 54.0f / max, 1.0f);
            ImVec4 TabUnactive = ImVec4(35.0f / max, 43.0f / max, 59.0f / max, 1.0f);

            IconColour                     = ImVec4(183.0f / 255.0f, 158.0f / 255.0f, 220.0f / 255.0f, 1.00f);
            SelectedColour                 = ImVec4(145.0f / 255.0f, 111.0f / 255.0f, 186.0f / 255.0f, 1.00f);
            colours[ImGuiCol_Text]         = ImVec4(244.0f / 255.0f, 244.0f / 255.0f, 244.0f / 255.0f, 1.00f);
            colours[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);

            colours[ImGuiCol_WindowBg] = TabActive;
            colours[ImGuiCol_ChildBg]  = TabActive;

            colours[ImGuiCol_PopupBg]        = ImVec4(42.0f / 255.0f, 38.0f / 255.0f, 47.0f / 255.0f, 1.00f);
            colours[ImGuiCol_Border]         = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
            colours[ImGuiCol_BorderShadow]   = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colours[ImGuiCol_FrameBg]        = ImVec4(65.0f / 255.0f, 79.0f / 255.0f, 92.0f / 255.0f, 1.00f);
            colours[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
            colours[ImGuiCol_FrameBgActive]  = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);

            colours[ImGuiCol_TitleBg]          = Titlebar;
            colours[ImGuiCol_TitleBgActive]    = Titlebar;
            colours[ImGuiCol_TitleBgCollapsed] = Titlebar;
            colours[ImGuiCol_MenuBarBg]        = Titlebar;

            colours[ImGuiCol_ScrollbarBg]          = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
            colours[ImGuiCol_ScrollbarGrab]        = ImVec4(0.6f, 0.6f, 0.6f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.7f, 0.7f, 0.7f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.8f, 0.8f, 0.8f, 1.00f);

            colours[ImGuiCol_CheckMark]        = ImVec4(155.0f / 255.0f, 130.0f / 255.0f, 207.0f / 255.0f, 1.00f);
            colours[ImGuiCol_SliderGrab]       = ImVec4(155.0f / 255.0f, 130.0f / 255.0f, 207.0f / 255.0f, 1.00f);
            colours[ImGuiCol_SliderGrabActive] = ImVec4(185.0f / 255.0f, 160.0f / 255.0f, 237.0f / 255.0f, 1.00f);
            colours[ImGuiCol_Button]           = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
            colours[ImGuiCol_ButtonHovered]    = ImVec4(59.0f / 255.0f, 46.0f / 255.0f, 80.0f / 255.0f, 1.0f);
            colours[ImGuiCol_ButtonActive]     = colours[ImGuiCol_ButtonHovered] + ImVec4(0.1f, 0.1f, 0.1f, 0.1f);

            colours[ImGuiCol_Separator]        = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
            colours[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
            colours[ImGuiCol_SeparatorActive]  = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);

            colours[ImGuiCol_ResizeGrip]        = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
            colours[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            colours[ImGuiCol_ResizeGripActive]  = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);

            colours[ImGuiCol_PlotLines]             = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
            colours[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
            colours[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            colours[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            colours[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
            colours[ImGuiCol_DragDropTarget]        = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colours[ImGuiCol_NavHighlight]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colours[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
            colours[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
            colours[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

            colours[ImGuiCol_Header]        = TabActive + ImVec4(0.1f, 0.1f, 0.1f, 0.1f);
            colours[ImGuiCol_HeaderHovered] = TabActive + ImVec4(0.1f, 0.1f, 0.1f, 0.1f);
            colours[ImGuiCol_HeaderActive]  = TabActive + ImVec4(0.05f, 0.05f, 0.05f, 0.1f);

#ifdef IMGUI_HAS_DOCK

            colours[ImGuiCol_Tab]                = TabUnactive;
            colours[ImGuiCol_TabHovered]         = TabActive + ImVec4(0.1f, 0.1f, 0.1f, 0.1f);
            colours[ImGuiCol_TabActive]          = TabActive;
            colours[ImGuiCol_TabUnfocused]       = TabUnactive;
            colours[ImGuiCol_TabUnfocusedActive] = TabActive;
            colours[ImGuiCol_DockingEmptyBg]     = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
            colours[ImGuiCol_DockingPreview]     = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);

#endif
        }

        colours[ImGuiCol_Separator]        = colours[ImGuiCol_TitleBg];
        colours[ImGuiCol_SeparatorActive]  = colours[ImGuiCol_Separator];
        colours[ImGuiCol_SeparatorHovered] = colours[ImGuiCol_Separator];

        colours[ImGuiCol_Tab]          = colours[ImGuiCol_MenuBarBg];
        colours[ImGuiCol_TabUnfocused] = colours[ImGuiCol_MenuBarBg];

        colours[ImGuiCol_TabUnfocusedActive] = colours[ImGuiCol_WindowBg];
        colours[ImGuiCol_TabActive]          = colours[ImGuiCol_WindowBg];
        colours[ImGuiCol_ChildBg]            = colours[ImGuiCol_TabActive];
        colours[ImGuiCol_ScrollbarBg]        = colours[ImGuiCol_TabActive];
        colours[ImGuiCol_TableHeaderBg]      = colours[ImGuiCol_TabActive];

        colours[ImGuiCol_TitleBgActive]    = colours[ImGuiCol_TitleBg];
        colours[ImGuiCol_TitleBgCollapsed] = colours[ImGuiCol_TitleBg];
        colours[ImGuiCol_MenuBarBg]        = colours[ImGuiCol_TitleBg];
        colours[ImGuiCol_PopupBg]          = colours[ImGuiCol_WindowBg] + ImVec4(0.05f, 0.05f, 0.05f, 0.0f);

        colours[ImGuiCol_Border]            = ImVec4(0.08f, 0.10f, 0.12f, 0.00f);
        colours[ImGuiCol_BorderShadow]      = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colours[ImGuiCol_TableBorderLight]  = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colours[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    }

    glm::vec4 ImGuiUtilities::GetSelectedColour()
    {
        return SelectedColour;
    }

    glm::vec4 ImGuiUtilities::GetIconColour()
    {
        return IconColour;
    }

    bool ImGuiUtilities::PropertyDropdown(const char* label, const char** options, int32_t optionCount, int32_t* selected)
    {
        const char* current = options[*selected];
        ImGui::TextUnformatted(label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        auto drawItemActivityOutline = [](float rounding = 0.0f, bool drawWhenInactive = false)
        {
            auto* drawList = ImGui::GetWindowDrawList();
            if(ImGui::IsItemHovered() && !ImGui::IsItemActive())
            {
                drawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                  ImColor(60, 60, 60), rounding, 0, 1.5f);
            }
            if(ImGui::IsItemActive())
            {
                drawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                  ImColor(80, 80, 80), rounding, 0, 1.0f);
            }
            else if(!ImGui::IsItemHovered() && drawWhenInactive)
            {
                drawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                  ImColor(50, 50, 50), rounding, 0, 1.0f);
            }
        };

        bool changed = false;

        // const std::string id = "##" + std::string(label);

        if(ImGui::BeginCombo(GenerateID(), current))
        {
            for(int i = 0; i < optionCount; i++)
            {
                const bool is_selected = (current == options[i]);
                if(ImGui::Selectable(options[i], is_selected))
                {
                    current   = options[i];
                    *selected = i;
                    changed   = true;
                }
                if(is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        drawItemActivityOutline(2.5f);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return changed;
    }

    void ImGuiUtilities::DrawItemActivityOutline(float rounding, bool drawWhenInactive, ImColor colourWhenActive)
    {
        auto* drawList = ImGui::GetWindowDrawList();

        ImRect expandedRect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        expandedRect.Min.x -= 1.0f;
        expandedRect.Min.y -= 1.0f;
        expandedRect.Max.x += 1.0f;
        expandedRect.Max.y += 1.0f;

        const ImRect rect = expandedRect;
        if(ImGui::IsItemHovered() && !ImGui::IsItemActive())
        {
            drawList->AddRect(rect.Min, rect.Max,
                              ImColor(60, 60, 60), rounding, 0, 1.5f);
        }
        if(ImGui::IsItemActive())
        {
            drawList->AddRect(rect.Min, rect.Max,
                              colourWhenActive, rounding, 0, 1.0f);
        }
        else if(!ImGui::IsItemHovered() && drawWhenInactive)
        {
            drawList->AddRect(rect.Min, rect.Max,
                              ImColor(50, 50, 50), rounding, 0, 1.0f);
        }
    }

    bool ImGuiUtilities::InputText(std::string& currentText)
    {
        ImGuiUtilities::ScopedStyle frameBorder(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGuiUtilities::ScopedColour frameColour(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
        char buffer[256];
        memset(buffer, 0, 256);
        memcpy(buffer, currentText.c_str(), currentText.length());

        bool updated = ImGui::InputText("##SceneName", buffer, 256);

        ImGuiUtilities::DrawItemActivityOutline(2.0f, false);

        if(updated)
            currentText = std::string(buffer);

        return updated;
    }

    // from https://github.com/ocornut/imgui/issues/2668
    void ImGuiUtilities::AlternatingRowsBackground(float lineHeight)
    {
        const ImU32 im_color = ImGui::GetColorU32(ImGuiCol_TableRowBgAlt);

        auto* draw_list   = ImGui::GetWindowDrawList();
        const auto& style = ImGui::GetStyle();

        if(lineHeight < 0)
        {
            lineHeight = ImGui::GetTextLineHeight();
        }

        lineHeight += style.ItemSpacing.y;

        float scroll_offset_h    = ImGui::GetScrollX();
        float scroll_offset_v    = ImGui::GetScrollY();
        float scrolled_out_lines = std::floor(scroll_offset_v / lineHeight);
        scroll_offset_v -= lineHeight * scrolled_out_lines;

        ImVec2 clip_rect_min(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);
        ImVec2 clip_rect_max(clip_rect_min.x + ImGui::GetWindowWidth(), clip_rect_min.y + ImGui::GetWindowHeight());

        if(ImGui::GetScrollMaxX() > 0)
        {
            clip_rect_max.y -= style.ScrollbarSize;
        }

        draw_list->PushClipRect(clip_rect_min, clip_rect_max);

        const float y_min = clip_rect_min.y - scroll_offset_v + ImGui::GetCursorPosY();
        const float y_max = clip_rect_max.y - scroll_offset_v + lineHeight;
        const float x_min = clip_rect_min.x + scroll_offset_h + ImGui::GetWindowContentRegionMin().x;
        const float x_max = clip_rect_min.x + scroll_offset_h + ImGui::GetWindowContentRegionMax().x;

        bool is_odd = (static_cast<int>(scrolled_out_lines) % 2) == 0;
        for(float y = y_min; y < y_max; y += lineHeight, is_odd = !is_odd)
        {
            if(is_odd)
            {
                draw_list->AddRectFilled({ x_min, y - style.ItemSpacing.y }, { x_max, y + lineHeight }, im_color);
            }
        }

        draw_list->PopClipRect();
    }

    ImRect ImGuiUtilities::GetItemRect()
    {
        return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    }

    ImRect ImGuiUtilities::RectExpanded(const ImRect& rect, float x, float y)
    {
        ImRect result = rect;
        result.Min.x -= x;
        result.Min.y -= y;
        result.Max.x += x;
        result.Max.y += y;
        return result;
    }

    ImRect ImGuiUtilities::RectOffset(const ImRect& rect, float x, float y)
    {
        ImRect result = rect;
        result.Min.x += x;
        result.Min.y += y;
        result.Max.x += x;
        result.Max.y += y;
        return result;
    }

    ImRect ImGuiUtilities::RectOffset(const ImRect& rect, ImVec2 xy)
    {
        return RectOffset(rect, xy.x, xy.y);
    }

    void ImGuiUtilities::DrawBorder(ImVec2 rectMin, ImVec2 rectMax, const ImVec4& borderColour, float thickness, float offsetX, float offsetY)
    {
        auto min = rectMin;
        min.x -= thickness;
        min.y -= thickness;
        min.x += offsetX;
        min.y += offsetY;
        auto max = rectMax;
        max.x += thickness;
        max.y += thickness;
        max.x += offsetX;
        max.y += offsetY;

        auto* drawList = ImGui::GetWindowDrawList();
        drawList->AddRect(min, max, ImGui::ColorConvertFloat4ToU32(borderColour), 0.0f, 0, thickness);
    }

    void ImGuiUtilities::DrawBorder(const ImVec4& borderColour, float thickness, float offsetX, float offsetY)
    {
        DrawBorder(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), borderColour, thickness, offsetX, offsetY);
    }

    void ImGuiUtilities::DrawBorder(float thickness, float offsetX, float offsetY)
    {
        DrawBorder(ImGui::GetStyleColorVec4(ImGuiCol_Border), thickness, offsetX, offsetY);
    }

    void ImGuiUtilities::DrawBorder(ImVec2 rectMin, ImVec2 rectMax, float thickness, float offsetX, float offsetY)
    {
        DrawBorder(rectMin, rectMax, ImGui::GetStyleColorVec4(ImGuiCol_Border), thickness, offsetX, offsetY);
    }

    void ImGuiUtilities::DrawBorder(ImRect rect, float thickness, float rounding, float offsetX, float offsetY)
    {
        auto min = rect.Min;
        min.x -= thickness;
        min.y -= thickness;
        min.x += offsetX;
        min.y += offsetY;
        auto max = rect.Max;
        max.x += thickness;
        max.y += thickness;
        max.x += offsetX;
        max.y += offsetY;

        auto* drawList = ImGui::GetWindowDrawList();
        drawList->AddRect(min, max, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Border)), rounding, 0, thickness);
    }
}

namespace ImGui
{
    bool DragFloatN_Coloured(const char* label, float* v, int components, float v_speed, float v_min, float v_max, const char* display_format)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGuiWindow* window = GetCurrentWindow();
        if(window->SkipItems)
            return false;

        ImGuiContext& g    = *GImGui;
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
            value_changed |= DragFloat("##v", &v[i], v_speed, v_min, v_max, display_format);

            const ImVec2 min        = GetItemRectMin();
            const ImVec2 max        = GetItemRectMax();
            const float spacing     = g.Style.FrameRounding;
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
        ImGuiWindow* window     = GetCurrentWindow();
        const ImGuiStyle& style = GImGui->Style;
        if(w_full <= 0.0f)
            w_full = GetContentRegionAvail().x;

        const float w_item_one = ImMax(1.0f, (w_full - (style.ItemInnerSpacing.x * 2.0f) * (components - 1)) / (float)components) - style.ItemInnerSpacing.x;
        for(int i = 0; i < components; i++)
            window->DC.ItemWidthStack.push_back(w_item_one - CalcTextSize(labels[i]).x);
        window->DC.ItemWidth = window->DC.ItemWidthStack.back();
    }

    bool DragFloatNEx(const char* labels[], float* v, int components, float v_speed, float v_min, float v_max,
                      const char* display_format)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if(window->SkipItems)
            return false;

        ImGuiContext& g    = *GImGui;
        bool value_changed = false;
        BeginGroup();

        PushMultiItemsWidthsAndLabels(labels, components, 0.0f);
        for(int i = 0; i < components; i++)
        {
            PushID(labels[i]);
            PushID(i);
            TextUnformatted(labels[i], FindRenderedTextEnd(labels[i]));
            SameLine();
            value_changed |= DragFloat("", &v[i], v_speed, v_min, v_max, display_format);
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
        LUMOS_PROFILE_FUNCTION();
        return DragFloatN_Coloured(label, v, 3, v_speed, v_min, v_max);
    }

    bool DragFloat4Coloured(const char* label, float* v, float v_speed, float v_min, float v_max)
    {
        LUMOS_PROFILE_FUNCTION();
        return DragFloatN_Coloured(label, v, 4, v_speed, v_min, v_max);
    }

    bool DragFloat2Coloured(const char* label, float* v, float v_speed, float v_min, float v_max)
    {
        LUMOS_PROFILE_FUNCTION();
        return DragFloatN_Coloured(label, v, 2, v_speed, v_min, v_max);
    }
}
