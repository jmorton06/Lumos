#include "Precompiled.h"
#include "ConsoleWindow.h"
#include "ImGui/IconsMaterialDesignIcons.h"
namespace Lumos
{
	u32 ConsoleWindow::s_MessageBufferRenderFilter = 0;
	uint16_t ConsoleWindow::s_MessageBufferCapacity = 200;
	uint16_t ConsoleWindow::s_MessageBufferSize = 0;
	uint16_t ConsoleWindow::s_MessageBufferBegin = 0;
	std::vector<Ref<ConsoleWindow::Message>> ConsoleWindow::s_MessageBuffer = std::vector<Ref<ConsoleWindow::Message>>(200);
	bool ConsoleWindow::s_AllowScrollingToBottom = true;
	bool ConsoleWindow::s_RequestScrollToBottom = false;

	ConsoleWindow::ConsoleWindow()
	{
		LUMOS_PROFILE_FUNCTION();
		m_Name = ICON_MDI_VIEW_LIST " Console###console";
		m_SimpleName = "Console";
        s_MessageBufferRenderFilter = Message::Level::Trace | Message::Level::Info | Message::Level::Debug | Message::Level::Warn | Message::Level::Error | Message::Level::Critical;
	}

	void ConsoleWindow::AddMessage(const Ref<Message>& message)
	{
		LUMOS_PROFILE_FUNCTION();
		if(message->m_Level == 0)
			return;
		
		auto messageStart = s_MessageBuffer.begin() + s_MessageBufferBegin;
			if(*messageStart) // If contains old message here
			{
				for(auto messIt = messageStart; messIt != s_MessageBuffer.end(); messIt++)
				{
					if (message->GetMessageID() == (*messIt)->GetMessageID())
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
					if (message->GetMessageID() == (*messIt)->GetMessageID())
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

	void ConsoleWindow::Flush()
	{
		LUMOS_PROFILE_FUNCTION();
		for(auto message = s_MessageBuffer.begin(); message != s_MessageBuffer.end(); message++)
			(*message) = nullptr;
		s_MessageBufferBegin = 0;
	}

	void ConsoleWindow::OnImGui()
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

	void ConsoleWindow::ImGuiRenderHeader()
	{
		LUMOS_PROFILE_FUNCTION();
		ImGuiStyle& style = ImGui::GetStyle();
	
		// Button for advanced settings
		if(ImGui::Button(ICON_MDI_COGS))
			ImGui::OpenPopup("SettingsPopup");
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
        
        float levelButtonWidths = 7 * 22.0f;
        Filter.Draw("###ConsoleFilter", ImGui::GetWindowWidth() - levelButtonWidths * 1.5f);
        
        ImGui::SameLine(ImGui::GetWindowWidth() - levelButtonWidths);
        
        for(int i = 0; i < 6; i++)
        {
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
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(Message::GetLevelName(level));
                ImGui::EndTooltip();
            }
            ImGui::PopStyleColor();
        }
	}

	void ConsoleWindow::ImGuiRenderMessages()
	{
		LUMOS_PROFILE_FUNCTION();
		ImGui::BeginChild("ScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
		{
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
				ImGui::SetScrollY(ImGui::GetScrollMaxY());
				s_RequestScrollToBottom = false;
			}
		}
		ImGui::EndChild();
	}

	ConsoleWindow::Message::Message(const std::string& message, Level level, const std::string& source, int threadID)
		: m_Message(message)
		, m_Level(level)
		, m_Source(source)
		, m_ThreadID(threadID)
		, m_MessageID(std::hash<std::string>()(message))
	{
	}

	void ConsoleWindow::Message::OnImGUIRender()
	{
		LUMOS_PROFILE_FUNCTION();
        if(s_MessageBufferRenderFilter & m_Level)
		{
			ImGui::PushID(this);
			ImGui::PushStyleColor(ImGuiCol_Text, GetRenderColour(m_Level));
			auto levelIcon = GetLevelIcon(m_Level);
			ImGui::Text("%s %s", levelIcon,  m_Message.c_str());
			
			if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::MenuItem("Copy"))
				{
					ImGui::SetClipboardText(m_Message.c_str());
				}
				
				ImGui::EndPopup();
			}
			
			if(ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::TextUnformatted(m_Source.c_str());
				ImGui::EndTooltip();
			}
			
			if (m_Count > 1)
			{
				ImGui::SameLine(ImGui::GetContentRegionAvail().x - (m_Count > 99 ? 35 : 28));
				ImGui::Text("%d", m_Count);
			}
			
			ImGui::PopStyleColor();
			ImGui::PopID();
		}
	}
	
	const char* ConsoleWindow::Message::GetLevelIcon(Level level)
	{
		switch(level)
		{
			case ConsoleWindow::Message::Level::Trace:
			return ICON_MDI_MESSAGE_TEXT;
			case ConsoleWindow::Message::Level::Info:
			return ICON_MDI_INFORMATION;
			case ConsoleWindow::Message::Level::Debug:
			return ICON_MDI_BUG;
			case ConsoleWindow::Message::Level::Warn:
			return ICON_MDI_ALERT;
			case ConsoleWindow::Message::Level::Error:
			return ICON_MDI_CLOSE_OCTAGON;
			case ConsoleWindow::Message::Level::Critical:
			return ICON_MDI_ALERT_OCTAGRAM;
			default:
			return "Unknown name";
		}
	}

	const char* ConsoleWindow::Message::GetLevelName(Level level)
	{
		switch(level)
		{
		case ConsoleWindow::Message::Level::Trace:
			return ICON_MDI_MESSAGE_TEXT" Trace";
		case ConsoleWindow::Message::Level::Info:
			return ICON_MDI_INFORMATION" Info";
		case ConsoleWindow::Message::Level::Debug:
			return ICON_MDI_BUG" Debug";
		case ConsoleWindow::Message::Level::Warn:
			return ICON_MDI_ALERT" Warning";
		case ConsoleWindow::Message::Level::Error:
			return ICON_MDI_CLOSE_OCTAGON" Error";
		case ConsoleWindow::Message::Level::Critical:
			return ICON_MDI_ALERT_OCTAGRAM" Critical";
		default:
			return "Unknown name";
		}
	}

	Maths::Colour ConsoleWindow::Message::GetRenderColour(Level level)
	{
		switch(level)
		{
		case ConsoleWindow::Message::Level::Trace:
			return {0.75f, 0.75f, 0.75f, 1.00f}; // Gray
		case ConsoleWindow::Message::Level::Info:
			return {0.40f, 0.70f, 1.00f, 1.00f}; // Blue
		case ConsoleWindow::Message::Level::Debug:
			return {0.00f, 0.50f, 0.50f, 1.00f}; // Cyan
		case ConsoleWindow::Message::Level::Warn:
			return {1.00f, 1.00f, 0.00f, 1.00f}; // Yellow
		case ConsoleWindow::Message::Level::Error:
			return {1.00f, 0.25f, 0.25f, 1.00f}; // Red
		case ConsoleWindow::Message::Level::Critical:
			return {0.6f, 0.2f, 0.8f, 1.00f}; // Purple
		default:
			return {1.00f, 1.00f, 1.00f, 1.00f};
		}
	}
}
