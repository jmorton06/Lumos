#include "ConsolePanel.h"
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>
#include <Lumos/ImGui/ImGuiUtilities.h>
#include <Lumos/Core/Profiler.h>

namespace Lumos
{
    uint32_t ConsolePanel::s_MessageBufferRenderFilter = 0;
    uint16_t ConsolePanel::s_MessageBufferCapacity = 200;
    uint16_t ConsolePanel::s_MessageBufferSize = 0;
    uint16_t ConsolePanel::s_MessageBufferBegin = 0;
    std::vector<SharedPtr<ConsolePanel::Message>> ConsolePanel::s_MessageBuffer = std::vector<SharedPtr<ConsolePanel::Message>>(200);
    bool ConsolePanel::s_AllowScrollingToBottom = true;
    bool ConsolePanel::s_RequestScrollToBottom = false;

    ConsolePanel::ConsolePanel()
    {
        LUMOS_PROFILE_FUNCTION();
        m_Name = ICON_MDI_VIEW_LIST " Console###console";
        m_SimpleName = "Console";
        s_MessageBufferRenderFilter = Message::Level::Trace | Message::Level::Info | Message::Level::Debug | Message::Level::Warn | Message::Level::Error | Message::Level::Critical;
    }

    void ConsolePanel::AddMessage(const SharedPtr<Message>& message)
    {
        LUMOS_PROFILE_FUNCTION();
        if(message->m_Level == 0)
            return;

        auto messageStart = s_MessageBuffer.begin() + s_MessageBufferBegin;
        if(*messageStart) // If contains old message here
        {
            for(auto messIt = messageStart; messIt != s_MessageBuffer.end(); messIt++)
            {
                if(message->GetMessageID() == (*messIt)->GetMessageID())
                {
                    (*messIt)->IncreaseCount();
                    return;
                }
            }
        }

        if(s_MessageBufferBegin != 0) // Skipped first messages in vector
        {
            for(auto messIt = s_MessageBuffer.begin(); messIt != messageStart; messIt++)
            {
                if(*messIt)
                {
                    if(message->GetMessageID() == (*messIt)->GetMessageID())
                    {
                        (*messIt)->IncreaseCount();
                        return;
                    }
                }
            }
        }

        *(s_MessageBuffer.begin() + s_MessageBufferBegin) = message;
        if(++s_MessageBufferBegin == s_MessageBufferCapacity)
            s_MessageBufferBegin = 0;
        if(s_MessageBufferSize < s_MessageBufferCapacity)
            s_MessageBufferSize++;

        if(s_AllowScrollingToBottom)
            s_RequestScrollToBottom = true;
    }

    void ConsolePanel::Flush()
    {
        LUMOS_PROFILE_FUNCTION();
        for(auto message = s_MessageBuffer.begin(); message != s_MessageBuffer.end(); message++)
            (*message) = nullptr;
        s_MessageBufferBegin = 0;
    }

    void ConsolePanel::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();
        auto flags = ImGuiWindowFlags_NoCollapse;
        ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_FirstUseEver);
        ImGui::Begin(m_Name.c_str(), &m_Active, flags);
        {
            ImGuiRenderHeader();
            ImGui::Separator();
            ImGuiRenderMessages();
        }
        ImGui::End();
    }

    void ConsolePanel::ImGuiRenderHeader()
    {
        LUMOS_PROFILE_FUNCTION();
        ImGuiStyle& style = ImGui::GetStyle();

        // Button for advanced settings
        {
            ImGuiUtilities::ScopedColour buttonColour(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            if(ImGui::Button(ICON_MDI_COGS))
                ImGui::OpenPopup("SettingsPopup");
        }
        if(ImGui::BeginPopup("SettingsPopup"))
        {
            // Checkbox for scrolling lock
            ImGui::Checkbox("Scroll to bottom", &s_AllowScrollingToBottom);

            // Button to clear the console
            if(ImGui::Button("Clear console"))
                Flush();

            ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::TextUnformatted(ICON_MDI_MAGNIFY);
        ImGui::SameLine();

        float spacing = ImGui::GetStyle().ItemSpacing.x;
        ImGui::GetStyle().ItemSpacing.x = 2;
        float levelButtonWidth = (ImGui::CalcTextSize(Message::GetLevelIcon(Message::Level(1))) + ImGui::GetStyle().FramePadding * 2.0f).x;
        float levelButtonWidths = (levelButtonWidth + ImGui::GetStyle().ItemSpacing.x) * 6;

        {
            ImGuiUtilities::ScopedFont boldFont(ImGui::GetIO().Fonts->Fonts[1]);
            ImGuiUtilities::ScopedStyle frameBorder(ImGuiStyleVar_FrameBorderSize, 0.0f);
            ImGuiUtilities::ScopedColour frameColour(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
            Filter.Draw("###ConsoleFilter", ImGui::GetContentRegionAvail().x - (levelButtonWidths));
            ImGuiUtilities::DrawItemActivityOutline(2.0f, false);
        }

        ImGui::SameLine(); // ImGui::GetWindowWidth() - levelButtonWidths);

        for(int i = 0; i < 6; i++)
        {
            ImGuiUtilities::ScopedColour buttonColour(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::SameLine();
            auto level = Message::Level(Maths::Pow(2, i));

            bool levelEnabled = s_MessageBufferRenderFilter & level;
            if(levelEnabled)
                ImGui::PushStyleColor(ImGuiCol_Text, Message::GetRenderColour(level));
            else
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5, 0.5f, 0.5f));

            if(ImGui::Button(Message::GetLevelIcon(level)))
            {
                s_MessageBufferRenderFilter ^= level;
            }

            if(ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", Message::GetLevelName(level));
            }
            ImGui::PopStyleColor();
        }

        ImGui::GetStyle().ItemSpacing.x = spacing;

        if(!Filter.IsActive())
        {
            ImGui::SameLine();
            ImGuiUtilities::ScopedFont boldFont(ImGui::GetIO().Fonts->Fonts[1]);
            ImGui::SetCursorPosX(ImGui::GetFontSize() * 4.0f);
            ImGuiUtilities::ScopedStyle padding(ImGuiStyleVar_FramePadding, ImVec2(0.0f, ImGui::GetStyle().FramePadding.y));
            ImGui::TextUnformatted("Search...");
        }
    }

    void ConsolePanel::ImGuiRenderMessages()
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::BeginChild("ScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        {
            // ImGuiUtilities::AlternatingRowsBackground();

            auto messageStart = s_MessageBuffer.begin() + s_MessageBufferBegin;
            if(*messageStart) // If contains old message here
            {
                for(auto message = messageStart; message != s_MessageBuffer.end(); message++)
                {
                    if(Filter.IsActive())
                    {
                        if(Filter.PassFilter((*message)->m_Message.c_str()))
                        {
                            (*message)->OnImGUIRender();
                        }
                    }
                    else
                    {
                        (*message)->OnImGUIRender();
                    }
                }
            }

            if(s_MessageBufferBegin != 0) // Skipped first messages in vector
            {
                for(auto message = s_MessageBuffer.begin(); message != messageStart; message++)
                {
                    if(*message)
                    {
                        if(Filter.IsActive())
                        {
                            if(Filter.PassFilter((*message)->m_Message.c_str()))
                            {
                                (*message)->OnImGUIRender();
                            }
                        }
                        else
                        {
                            (*message)->OnImGUIRender();
                        }
                    }
                }
            }

            if(s_RequestScrollToBottom && ImGui::GetScrollMaxY() > 0)
            {
                ImGui::SetScrollHereY(1.0f);
                s_RequestScrollToBottom = false;
            }
        }
        ImGui::EndChild();
    }

    ConsolePanel::Message::Message(const std::string& message, Level level, const std::string& source, int threadID)
        : m_Message(message)
        , m_Level(level)
        , m_Source(source)
        , m_ThreadID(threadID)
        , m_MessageID(std::hash<std::string>()(message))
    {
    }

    void ConsolePanel::Message::OnImGUIRender()
    {
        LUMOS_PROFILE_FUNCTION();
        if(s_MessageBufferRenderFilter & m_Level)
        {
            ImGuiUtilities::ScopedID((int)m_MessageID);
            ImGui::PushStyleColor(ImGuiCol_Text, GetRenderColour(m_Level));
            auto levelIcon = GetLevelIcon(m_Level);
            ImGui::TextUnformatted(levelIcon);
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextUnformatted(m_Message.c_str());
            if(ImGui::BeginPopupContextItem(m_Message.c_str()))
            {
                if(ImGui::MenuItem("Copy"))
                {
                    ImGui::SetClipboardText(m_Message.c_str());
                }

                ImGui::EndPopup();
            }

            if(ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", m_Source.c_str());
            }

            if(m_Count > 1)
            {
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - (m_Count > 99 ? ImGui::GetFontSize() * 1.7f : ImGui::GetFontSize() * 1.5f));
                ImGui::Text("%d", m_Count);
            }
        }
    }

    const char* ConsolePanel::Message::GetLevelIcon(Level level)
    {
        switch(level)
        {
        case ConsolePanel::Message::Level::Trace:
            return ICON_MDI_MESSAGE_TEXT;
        case ConsolePanel::Message::Level::Info:
            return ICON_MDI_INFORMATION;
        case ConsolePanel::Message::Level::Debug:
            return ICON_MDI_BUG;
        case ConsolePanel::Message::Level::Warn:
            return ICON_MDI_ALERT;
        case ConsolePanel::Message::Level::Error:
            return ICON_MDI_CLOSE_OCTAGON;
        case ConsolePanel::Message::Level::Critical:
            return ICON_MDI_ALERT_OCTAGRAM;
        default:
            return "Unknown name";
        }
    }

    const char* ConsolePanel::Message::GetLevelName(Level level)
    {
        switch(level)
        {
        case ConsolePanel::Message::Level::Trace:
            return ICON_MDI_MESSAGE_TEXT " Trace";
        case ConsolePanel::Message::Level::Info:
            return ICON_MDI_INFORMATION " Info";
        case ConsolePanel::Message::Level::Debug:
            return ICON_MDI_BUG " Debug";
        case ConsolePanel::Message::Level::Warn:
            return ICON_MDI_ALERT " Warning";
        case ConsolePanel::Message::Level::Error:
            return ICON_MDI_CLOSE_OCTAGON " Error";
        case ConsolePanel::Message::Level::Critical:
            return ICON_MDI_ALERT_OCTAGRAM " Critical";
        default:
            return "Unknown name";
        }
    }

    glm::vec4 ConsolePanel::Message::GetRenderColour(Level level)
    {
        switch(level)
        {
        case ConsolePanel::Message::Level::Trace:
            return { 0.75f, 0.75f, 0.75f, 1.00f }; // Gray
        case ConsolePanel::Message::Level::Info:
            return { 0.40f, 0.70f, 1.00f, 1.00f }; // Blue
        case ConsolePanel::Message::Level::Debug:
            return { 0.00f, 0.50f, 0.50f, 1.00f }; // Cyan
        case ConsolePanel::Message::Level::Warn:
            return { 1.00f, 1.00f, 0.00f, 1.00f }; // Yellow
        case ConsolePanel::Message::Level::Error:
            return { 1.00f, 0.25f, 0.25f, 1.00f }; // Red
        case ConsolePanel::Message::Level::Critical:
            return { 0.6f, 0.2f, 0.8f, 1.00f }; // Purple
        default:
            return { 1.00f, 1.00f, 1.00f, 1.00f };
        }
    }
}
