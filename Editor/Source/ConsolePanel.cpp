#include "ConsolePanel.h"
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>
#include <Lumos/ImGui/ImGuiUtilities.h>
#include <Lumos/Core/Profiler.h>
#include <Lumos/Maths/MathsUtilities.h>
#include <Lumos/Core/Mutex.h>

namespace Lumos
{
    uint32_t ConsolePanel::s_MessageBufferRenderFilter    = 0;
    bool ConsolePanel::s_AllowScrollingToBottom           = true;
    bool ConsolePanel::s_RequestScrollToBottom            = false;
    Mutex* ConsolePanel::m_MessageBufferMutex             = nullptr;
    TDArray<ConsoleMessage> ConsolePanel::m_MessageBuffer = TDArray<ConsoleMessage>();

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
        case ConsoleLogLevel::Fatal:
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
        case ConsoleLogLevel::Fatal:
            return ICON_MDI_ALERT_OCTAGRAM " FATAL";
        default:
            return "Unknown name";
        }
    }

    Vec4 GetRenderColour(ConsoleLogLevel level)
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
        case ConsoleLogLevel::Fatal:
            return { 0.6f, 0.2f, 0.8f, 1.00f }; // Purple
        default:
            return { 1.00f, 1.00f, 1.00f, 1.00f };
        }
    }

    ConsolePanel::ConsolePanel()
    {
        LUMOS_PROFILE_FUNCTION();
        m_Name                      = "Console###console";
        m_SimpleName                = "Console";
        s_MessageBufferRenderFilter = (int16_t)ConsoleLogLevel::Trace | (int16_t)ConsoleLogLevel::Info | (int16_t)ConsoleLogLevel::Debug | (int16_t)ConsoleLogLevel::Warn | (int16_t)ConsoleLogLevel::Error | (int16_t)ConsoleLogLevel::Fatal;
        m_MessageBuffer.Reserve(2000);

        if(!m_MessageBufferMutex)
        {
            m_MessageBufferMutex = new Mutex();
            MutexInit(m_MessageBufferMutex);
        }
    }

    ConsolePanel::~ConsolePanel()
    {
        MutexDestroy(m_MessageBufferMutex);
        delete m_MessageBufferMutex;
        m_MessageBufferMutex = nullptr;
    }

    void ConsolePanel::AddMessage(const ConsoleMessage& message)
    {
        LUMOS_PROFILE_FUNCTION();
        if(!m_MessageBufferMutex)
        {
            m_MessageBufferMutex = new Mutex();
            MutexInit(m_MessageBufferMutex);
        }

        if(m_MessageBuffer.Size() > 0)
        {
            if(m_MessageBuffer.Back().m_MessageID == message.m_MessageID)
            {
                ScopedMutex lock(m_MessageBufferMutex);
                m_MessageBuffer.Back().m_Count++;
                return;
            }
        }
        {
            ScopedMutex lock(m_MessageBufferMutex);
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
        ImGuiUtilities::BeginPanel(m_Name.c_str(), nullptr, flags);
        {
#ifdef LUMOS_PLATFORM_IOS
            // Pin filter/action row to the bottom so it stays in thumb reach.
            const float filterRowH = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y * 2.0f;
            ImGui::BeginChild("##ConsoleMessages", ImVec2(0.0f, -filterRowH), false);
            ImGuiRenderMessages();
            ImGui::EndChild();
            ImGui::Separator();
            ImGuiRenderHeader();
#else
            ImGuiRenderHeader();
            ImGui::Separator();
            ImGuiRenderMessages();
#endif
        }
        ImGui::End();
    }

    void ConsolePanel::ImGuiRenderHeader()
    {
        LUMOS_PROFILE_FUNCTION();
        ImGuiStyle& style = ImGui::GetStyle();
        ImGui::AlignTextToFramePadding();

        // Clear button
        {
            ImGuiUtilities::ScopedColour buttonColour(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            if(ImGui::Button(ICON_MDI_DELETE))
            {
                ScopedMutex lock(m_MessageBufferMutex);
                m_MessageBuffer.Clear();
            }
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Clear console");
        }

        // Auto-scroll lock toggle (one-tap) — only on touch platforms.
#ifdef LUMOS_PLATFORM_IOS
        ImGui::SameLine();
        {
            ImGuiUtilities::ScopedColour buttonColour(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            const char* lockIcon = s_AllowScrollingToBottom ? ICON_MDI_ARROW_DOWN : ICON_MDI_PAUSE;
            if(ImGui::Button(lockIcon))
                s_AllowScrollingToBottom = !s_AllowScrollingToBottom;
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip(s_AllowScrollingToBottom ? "Pause auto-scroll" : "Resume auto-scroll");
        }
#endif

        // Settings button
        ImGui::SameLine();
        {
            ImGuiUtilities::ScopedColour buttonColour(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            if(ImGui::Button(ICON_MDI_COGS))
                ImGui::OpenPopup("SettingsPopup");
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Settings");
        }
        if(ImGui::BeginPopup("SettingsPopup"))
        {
            // Checkbox for scrolling lock
            ImGui::Checkbox("Auto-scroll to bottom", &s_AllowScrollingToBottom);

            ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        float spacing                   = ImGui::GetStyle().ItemSpacing.x;
        ImGui::GetStyle().ItemSpacing.x = 2;

        // Count messages first so button widths can account for the count labels
        int counts[6] = {0, 0, 0, 0, 0, 0};
        for(uint32_t i = 0; i < m_MessageBuffer.Size(); i++)
        {
            auto& msg = m_MessageBuffer[i];
            int levelIndex = 0;
            switch(msg.m_Level)
            {
                case ConsoleLogLevel::Trace: levelIndex = 0; break;
                case ConsoleLogLevel::Debug: levelIndex = 1; break;
                case ConsoleLogLevel::Info:  levelIndex = 2; break;
                case ConsoleLogLevel::Warn:  levelIndex = 3; break;
                case ConsoleLogLevel::Error: levelIndex = 4; break;
                case ConsoleLogLevel::Fatal: levelIndex = 5; break;
                default: break;
            }
            counts[levelIndex] += msg.m_Count;
        }

        // Build labels and measure total button width before drawing the filter
        char buttonLabels[6][32];
        float levelButtonWidths = 0.0f;
        for(int i = 0; i < 6; i++)
        {
            auto level = ConsoleLogLevel(Maths::Pow(2, i));
            if(counts[i] > 0)
                snprintf(buttonLabels[i], sizeof(buttonLabels[i]), "%s %d", GetLevelIcon(level), counts[i]);
            else
                snprintf(buttonLabels[i], sizeof(buttonLabels[i]), "%s", GetLevelIcon(level));
            float w = (ImGui::CalcTextSize(buttonLabels[i]) + ImGui::GetStyle().FramePadding * 2.0f).x;
            levelButtonWidths += w + ImGui::GetStyle().ItemSpacing.x;
        }

        // Store cursor position for placeholder text
        ImVec2 filterPos  = ImGui::GetCursorScreenPos();
        float filterWidth = ImGui::GetContentRegionAvail().x - levelButtonWidths;
        if(filterWidth < 40.0f) filterWidth = 40.0f;

        {
            ImGuiUtilities::ScopedFont boldFont(ImGui::GetIO().Fonts->Fonts[1]);
            ImGuiUtilities::ScopedStyle frameBorder(ImGuiStyleVar_FrameBorderSize, 0.0f);
            ImGuiUtilities::ScopedColour frameColour(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
            Filter.Draw("###ConsoleFilter", filterWidth);
            ImGuiUtilities::DrawItemActivityOutline(2.0f, false);
        }

        // Draw placeholder when filter is empty
        if(!Filter.IsActive())
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            float yOffset = (ImGui::GetFrameHeight() - ImGui::GetFontSize()) * 0.5f;

            ImVec2 iconPos = ImVec2(filterPos.x + ImGui::GetStyle().FramePadding.x, filterPos.y + yOffset);
            drawList->AddText(iconPos, ImGui::GetColorU32(ImGuiCol_TextDisabled), ICON_MDI_MAGNIFY);

            float iconWidth = ImGui::CalcTextSize(ICON_MDI_MAGNIFY).x;
            ImVec2 textPos  = ImVec2(filterPos.x + ImGui::GetStyle().FramePadding.x + iconWidth + ImGui::GetStyle().ItemInnerSpacing.x, filterPos.y + yOffset);

            ImGuiUtilities::ScopedFont boldFont(ImGui::GetIO().Fonts->Fonts[1]);
            drawList->AddText(textPos, ImGui::GetColorU32(ImGuiCol_TextDisabled), "Search...");
        }

        ImGui::AlignTextToFramePadding();

        for(int i = 0; i < 6; i++)
        {
            ImGuiUtilities::ScopedColour buttonColour(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::SameLine();
            auto level = ConsoleLogLevel(Maths::Pow(2, i));

            bool levelEnabled = s_MessageBufferRenderFilter & (int16_t)level;
            if(levelEnabled)
                ImGui::PushStyleColor(ImGuiCol_Text, GetRenderColour(level));
            else
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));

            if(ImGui::Button(buttonLabels[i]))
                s_MessageBufferRenderFilter ^= (int16_t)level;

            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", GetLevelName(level));

            ImGui::PopStyleColor();
        }

        ImGui::GetStyle().ItemSpacing.x = spacing;
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

        // Show auto-scroll status indicator when disabled
        if(!s_AllowScrollingToBottom)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
            ImGui::TextUnformatted(ICON_MDI_PAUSE " Auto-scroll paused");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            if(ImGui::SmallButton("Resume"))
                s_AllowScrollingToBottom = true;
            ImGui::SameLine();
            if(ImGui::SmallButton(ICON_MDI_ARROW_DOWN " Jump to Bottom"))
                s_RequestScrollToBottom = true;
        }

        // ImGui::BeginChild("ScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        if(ImGui::BeginTable("Messages", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg))
        {

            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, MyItemColumnID_Type);
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
                    ImGui::TextUnformatted(levelIcon);

                    if(ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("%s", GetLevelName(message->m_Level));
                    }

                    ImGui::TableNextColumn();
                    message->OnImGUIRender();
                    ImGui::PopStyleColor();
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
        if(ImGui::IsItemClicked() && ImGui::IsItemHovered())
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
            // Use BeginPopup instead of BeginPopupModal to allow closing on outside click
            if(ImGui::BeginPopup("Message", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
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
            else
            {
                // Popup was closed (clicked outside)
                m_DetailedPanelOpen = false;
            }
        }

        if(ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", m_Source.c_str());
        }

        // Show count badge if message repeated
        if(m_Count > 1)
        {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            char countLabel[32];
            snprintf(countLabel, sizeof(countLabel), " %d ", m_Count);
            ImGui::SmallButton(countLabel);
            ImGui::PopStyleColor(2);
            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Message repeated %d times", m_Count);
        }
    }

}
