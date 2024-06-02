#include "ConsolePanel.h"
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>
#include <Lumos/ImGui/ImGuiUtilities.h>
#include <Lumos/Core/Profiler.h>
#include <Lumos/Maths/MathsUtilities.h>

namespace Lumos
{
    uint32_t ConsolePanel::s_MessageBufferRenderFilter = 0;
    bool ConsolePanel::s_AllowScrollingToBottom        = true;
    bool ConsolePanel::s_RequestScrollToBottom         = false;
    std::mutex ConsolePanel::m_MessageBufferMutex;
    TDArray<ConsoleMessage> ConsolePanel::m_MessageBuffer = TDArray<ConsoleMessage>(2000);

    const char* GetLevelIcon(ConsoleLogLevel level)
    {
        switch(level)
        {
        case ConsoleLogLevel::Trace:
            return ICON_MDI_MESSAGE_TEXT;
        case ConsoleLogLevel::Info:
            return ICON_MDI_INFORMATION;
        case ConsoleLogLevel::Debug:
            return ICON_MDI_BUG;
        case ConsoleLogLevel::Warn:
            return ICON_MDI_ALERT;
        case ConsoleLogLevel::Error:
            return ICON_MDI_CLOSE_OCTAGON;
        case ConsoleLogLevel::Critical:
            return ICON_MDI_ALERT_OCTAGRAM;
        default:
            return "Unknown name";
        }
    }

    const char* GetLevelName(ConsoleLogLevel level)
    {
        switch(level)
        {
        case ConsoleLogLevel::Trace:
            return ICON_MDI_MESSAGE_TEXT " Trace";
        case ConsoleLogLevel::Info:
            return ICON_MDI_INFORMATION " Info";
        case ConsoleLogLevel::Debug:
            return ICON_MDI_BUG " Debug";
        case ConsoleLogLevel::Warn:
            return ICON_MDI_ALERT " Warning";
        case ConsoleLogLevel::Error:
            return ICON_MDI_CLOSE_OCTAGON " Error";
        case ConsoleLogLevel::Critical:
            return ICON_MDI_ALERT_OCTAGRAM " Critical";
        default:
            return "Unknown name";
        }
    }

    glm::vec4 GetRenderColour(ConsoleLogLevel level)
    {
        switch(level)
        {
        case ConsoleLogLevel::Trace:
            return { 0.75f, 0.75f, 0.75f, 1.00f }; // Gray
        case ConsoleLogLevel::Info:
            return { 0.40f, 0.70f, 1.00f, 1.00f }; // Blue
        case ConsoleLogLevel::Debug:
            return { 0.00f, 0.50f, 0.50f, 1.00f }; // Cyan
        case ConsoleLogLevel::Warn:
            return { 1.00f, 1.00f, 0.00f, 1.00f }; // Yellow
        case ConsoleLogLevel::Error:
            return { 1.00f, 0.25f, 0.25f, 1.00f }; // Red
        case ConsoleLogLevel::Critical:
            return { 0.6f, 0.2f, 0.8f, 1.00f }; // Purple
        default:
            return { 1.00f, 1.00f, 1.00f, 1.00f };
        }
    }

    ConsolePanel::ConsolePanel()
    {
        LUMOS_PROFILE_FUNCTION();
        m_Name                      = ICON_MDI_VIEW_LIST " Console###console";
        m_SimpleName                = "Console";
        s_MessageBufferRenderFilter = (int16_t)ConsoleLogLevel::Trace | (int16_t)ConsoleLogLevel::Info | (int16_t)ConsoleLogLevel::Debug | (int16_t)ConsoleLogLevel::Warn | (int16_t)ConsoleLogLevel::Error | (int16_t)ConsoleLogLevel::Critical;
    }

    void ConsolePanel::AddMessage(const ConsoleMessage& message)
    {
        LUMOS_PROFILE_FUNCTION();
        // if(message.m_Level == ConsoleLogLevel::None)
        //   return;

        if(m_MessageBuffer.Size() > 0)
        {
            if(m_MessageBuffer.Back().m_MessageID == message.m_MessageID)
            {
                m_MessageBuffer.Back().m_Count++;
                return;
            }
        }
        {
            std::scoped_lock<std::mutex> lock(m_MessageBufferMutex);
            m_MessageBuffer.EmplaceBack(message);
        }

        if(s_AllowScrollingToBottom)
            s_RequestScrollToBottom = true;
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
        ImGui::AlignTextToFramePadding();
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
            {
                std::scoped_lock<std::mutex> lock(m_MessageBufferMutex);
                m_MessageBuffer.Clear();
            }

            ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::TextUnformatted(ICON_MDI_MAGNIFY);
        ImGui::SameLine();

        float spacing                   = ImGui::GetStyle().ItemSpacing.x;
        ImGui::GetStyle().ItemSpacing.x = 2;
        float levelButtonWidth          = (ImGui::CalcTextSize(GetLevelIcon(ConsoleLogLevel(1))) + ImGui::GetStyle().FramePadding * 2.0f).x;
        float levelButtonWidths         = (levelButtonWidth + ImGui::GetStyle().ItemSpacing.x) * 6;

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
            auto level = ConsoleLogLevel(Maths::Pow(2, i));

            bool levelEnabled = s_MessageBufferRenderFilter & (int16_t)level;
            if(levelEnabled)
                ImGui::PushStyleColor(ImGuiCol_Text, GetRenderColour(level));
            else
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5, 0.5f, 0.5f));

            if(ImGui::Button(GetLevelIcon(level)))
            {
                s_MessageBufferRenderFilter ^= (int16_t)level;
            }

            if(ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", GetLevelName(level));
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

    enum MyItemColumnID
    {
        MyItemColumnID_Time,
        MyItemColumnID_Message,
        MyItemColumnID_Type
    };

    void ConsolePanel::ImGuiRenderMessages()
    {
        LUMOS_PROFILE_FUNCTION();
        // ImGui::BeginChild("ScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        if(ImGui::BeginTable("Messages", 3, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg))
        {

            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, MyItemColumnID_Type);
            ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, MyItemColumnID_Time);
            ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_NoSort, 0.0f, MyItemColumnID_Message);
            ImGui::TableSetupScrollFreeze(0, 1);

            ImGui::TableHeadersRow();
            // ImGuiUtilities::AlternatingRowsBackground();

            ImGui::TableNextRow();

            auto DrawMessage = [](ConsoleMessage* message)
            {
                if(s_MessageBufferRenderFilter & (int16_t)message->m_Level)
                {
                    ImGui::TableNextColumn();
                    ImGui::PushStyleColor(ImGuiCol_Text, GetRenderColour(message->m_Level));
                    auto levelIcon = GetLevelIcon(message->m_Level);
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(levelIcon).x
                                         - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
                    ImGui::TextUnformatted(levelIcon);

                    if(ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("%s", GetLevelName(message->m_Level));
                    }
                    ImGui::PopStyleColor();

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(message->m_Time.c_str());

                    ImGui::TableNextColumn();
                    message->OnImGUIRender();
                    ImGui::TableNextRow();
                }
            };

            for(uint32_t i = 0; i < m_MessageBuffer.Size(); i++)
            {
                auto& msg = m_MessageBuffer[i];

				if(Filter.IsActive() && !Filter.PassFilter(msg.m_Message.c_str()))
					continue;
                DrawMessage(&msg);
            }

            if(s_RequestScrollToBottom && ImGui::GetScrollMaxY() > 0)
            {
                ImGui::SetScrollHereY(1.0f);
                s_RequestScrollToBottom = false;
            }
            ImGui::EndTable();
        }
    }

    ConsoleMessage::ConsoleMessage(const std::string& message, ConsoleLogLevel level, const std::string& source, int threadID, const std::string& time)
        : m_Message(message)
        , m_Level(level)
        , m_Source(source)
        , m_ThreadID(threadID)
        , m_MessageID(std::hash<std::string>()(message) + std::hash<std::string>()(time))
        , m_Time(time)
    {
    }

    void ConsoleMessage::OnImGUIRender()
    {
        LUMOS_PROFILE_FUNCTION();

        ImGuiUtilities::ScopedID scopedID((int)m_MessageID);
        ImGui::TextUnformatted(m_Message.c_str());

        bool clicked = false;
        if(ImGui::IsItemClicked())
            clicked = true;

        if(ImGui::BeginPopupContextItem(m_Message.c_str()))
        {
            if(ImGui::MenuItem("Copy"))
            {
                ImGui::SetClipboardText(m_Message.c_str());
            }

            ImGui::EndPopup();
        }
        static bool m_DetailedPanelOpen = false;
        if(clicked)
        {
            ImGui::OpenPopup("Message");
            ImVec2 size = ImGui::GetMainViewport()->Size;
            ImGui::SetNextWindowSize({ size.x * 0.5f, size.y * 0.5f });
            ImGui::SetNextWindowPos({ size.x / 2.0f, size.y / 2.5f }, 0, { 0.5, 0.5 });
            m_DetailedPanelOpen = true;
        }

        if(m_DetailedPanelOpen)
        {
            if(ImGui::BeginPopupModal("Message", &m_DetailedPanelOpen, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
            {
                ImGui::TextWrapped("Message : %s", m_Message.c_str());

                if(ImGui::BeginPopupContextItem(m_Message.c_str()))
                {
                    if(ImGui::MenuItem("Copy"))
                    {
                        ImGui::SetClipboardText(m_Message.c_str());
                    }

                    ImGui::EndPopup();
                }

                ImGui::TextWrapped("Source : %s", m_Source.c_str());

                ImGui::Text("Time : %s", m_Time.c_str());
                ImGui::Text("Type : %s", GetLevelName(m_Level));

                ImGui::EndPopup();
            }
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
