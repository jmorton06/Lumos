#include "lmpch.h"
#include "ConsoleWindow.h"
#include <IconFontCppHeaders/IconsFontAwesome5.h>
namespace Lumos 
{
	ConsoleWindow::Message::Level ConsoleWindow::s_MessageBufferRenderFilter = ConsoleWindow::Message::Level::Trace;
	uint16_t ConsoleWindow::s_MessageBufferCapacity = 200;
	uint16_t ConsoleWindow::s_MessageBufferSize = 0;
	uint16_t ConsoleWindow::s_MessageBufferBegin = 0;
	std::vector<Ref<ConsoleWindow::Message>> ConsoleWindow::s_MessageBuffer = std::vector<Ref<ConsoleWindow::Message>>(200);
	bool ConsoleWindow::s_AllowScrollingToBottom = true;
	bool ConsoleWindow::s_RequestScrollToBottom = false;

	ConsoleWindow::ConsoleWindow()
	{
		m_Name = ICON_FA_LIST" Console###console";
		m_SimpleName = "Console";
	}

	void ConsoleWindow::AddMessage(const Ref<Message>& message)
	{
		if (message->m_Level == Message::Level::Invalid)
			return;

		*(s_MessageBuffer.begin() + s_MessageBufferBegin) = message;
		if (++s_MessageBufferBegin == s_MessageBufferCapacity)
			s_MessageBufferBegin = 0;
		if (s_MessageBufferSize < s_MessageBufferCapacity)
			s_MessageBufferSize++;

		if (s_AllowScrollingToBottom)
			s_RequestScrollToBottom = true;
	}

	void ConsoleWindow::Flush()
	{
		for (auto message = s_MessageBuffer.begin(); message != s_MessageBuffer.end(); message++)
			(*message) = CreateRef<Message>();
		s_MessageBufferBegin = 0;
	}

	void ConsoleWindow::OnImGui()
	{
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
		ImGuiStyle& style = ImGui::GetStyle();
		const float spacing = style.ItemInnerSpacing.x;

		// Dropdown with levels
		ImGui::PushItemWidth(ImGui::CalcTextSize("Critical").x * 1.36f);
		if (ImGui::BeginCombo(
			"##MessageRenderFilter",
			Message::GetLevelName(s_MessageBufferRenderFilter),
			ImGuiComboFlags_NoArrowButton))
		{
			for (auto level = Message::s_Levels.begin(); level != Message::s_Levels.end(); level++)
			{
				bool is_selected = (s_MessageBufferRenderFilter == *level);
				if (ImGui::Selectable(Message::GetLevelName(*level), is_selected))
					s_MessageBufferRenderFilter = *level;
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();

		ImGui::SameLine(0.0f, spacing);

		// Button for advanced settings
		if (ImGui::Button("Settings"))
			ImGui::OpenPopup("SettingsPopup");
		if (ImGui::BeginPopup("SettingsPopup"))
		{
			// Checkbox for scrolling lock
			ImGui::Checkbox("Scroll to bottom", &s_AllowScrollingToBottom);

			// Button to clear the console
			if (ImGui::Button("Clear console"))
				Flush();

			ImGui::EndPopup();
		}

		ImGui::SameLine();
		Filter.Draw("###ConsoleFilter", -100.0f);

	}

	void ConsoleWindow::ImGuiRenderMessages()
	{
		ImGui::BeginChild("ScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
		{
			auto messageStart = s_MessageBuffer.begin() + s_MessageBufferBegin;
			if (*messageStart) // If contains old message here
			{
				for (auto message = messageStart; message != s_MessageBuffer.end(); message++)
				{
					if (Filter.IsActive())
					{
						if(Filter.PassFilter((*message)->m_Message.c_str()))
							(*message)->OnImGUIRender();
					}
					else
					{
						(*message)->OnImGUIRender();
					}
				}
			}
					
			if (s_MessageBufferBegin != 0) // Skipped first messages in vector
			{
				for (auto message = s_MessageBuffer.begin(); message != messageStart; message++)
				{
					if (*message)
					{
						if (Filter.IsActive())
						{
							if (Filter.PassFilter((*message)->m_Message.c_str()))
								(*message)->OnImGUIRender();
						}
						else
						{
							(*message)->OnImGUIRender();
						}
					}
				}
			}
					
			if (s_RequestScrollToBottom && ImGui::GetScrollMaxY() > 0)
			{
				ImGui::SetScrollY(ImGui::GetScrollMaxY());
				s_RequestScrollToBottom = false;
			}
		}
		ImGui::EndChild();
	}

	std::vector<ConsoleWindow::Message::Level> ConsoleWindow::Message::s_Levels{
		ConsoleWindow::Message::Level::Trace,
		//ConsoleWindow::Message::Level::Debug,
		ConsoleWindow::Message::Level::Info,
		ConsoleWindow::Message::Level::Warn,
		ConsoleWindow::Message::Level::Error,
		ConsoleWindow::Message::Level::Critical,
		ConsoleWindow::Message::Level::Off
	};

	ConsoleWindow::Message::Message(const String& message, Level level, const String& source, int threadID)
		: m_Message(message), m_Level(level), m_Source(source), m_ThreadID(threadID)
	{
	}

	void ConsoleWindow::Message::OnImGUIRender()
	{
		if (m_Level != Level::Invalid && m_Level >= s_MessageBufferRenderFilter)
		{
			Maths::Colour colour = GetRenderColour(m_Level);
			ImGui::PushStyleColor(ImGuiCol_Text, { colour.r_, colour.g_, colour.b_, colour.a_ });
			ImGui::TextUnformatted(m_Message.c_str());
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::TextUnformatted(m_Source.c_str());
				ImGui::EndTooltip();
			}
			ImGui::PopStyleColor();
		}
	}

	ConsoleWindow::Message::Level ConsoleWindow::Message::GetLowerLevel(Level level)
	{
		switch (level)
		{
			case ConsoleWindow::Message::Level::Off     : return ConsoleWindow::Message::Level::Critical;
			case ConsoleWindow::Message::Level::Critical: return ConsoleWindow::Message::Level::Error;
			case ConsoleWindow::Message::Level::Error   : return ConsoleWindow::Message::Level::Warn;
			case ConsoleWindow::Message::Level::Warn    : //return ConsoleWindow::Message::Level::Debug;
			case ConsoleWindow::Message::Level::Debug   : return ConsoleWindow::Message::Level::Info;
			case ConsoleWindow::Message::Level::Info    :
			case ConsoleWindow::Message::Level::Trace   : return ConsoleWindow::Message::Level::Trace;
            default: return ConsoleWindow::Message::Level::Invalid;
		}
	}

	ConsoleWindow::Message::Level ConsoleWindow::Message::GetHigherLevel(Level level)
	{
		switch (level)
		{
            case ConsoleWindow::Message::Level::Trace   : return ConsoleWindow::Message::Level::Info;
			case ConsoleWindow::Message::Level::Info    : //return ConsoleWindow::Message::Level::Debug;
            case ConsoleWindow::Message::Level::Debug   : return ConsoleWindow::Message::Level::Warn;
            case ConsoleWindow::Message::Level::Warn    : return ConsoleWindow::Message::Level::Error;
            case ConsoleWindow::Message::Level::Error   : return ConsoleWindow::Message::Level::Critical;
			case ConsoleWindow::Message::Level::Critical:
            case ConsoleWindow::Message::Level::Off     : return ConsoleWindow::Message::Level::Off;
            default: return ConsoleWindow::Message::Level::Invalid;
		}
	}


	const char* ConsoleWindow::Message::GetLevelName(Level level)
	{
		switch (level)
		{
			case ConsoleWindow::Message::Level::Trace   : return "Trace";
			case ConsoleWindow::Message::Level::Info    : return "Info";
			case ConsoleWindow::Message::Level::Debug   : return "Debug";
			case ConsoleWindow::Message::Level::Warn    : return "Warning";
			case ConsoleWindow::Message::Level::Error   : return "Error";
			case ConsoleWindow::Message::Level::Critical: return "Critical";
			case ConsoleWindow::Message::Level::Off     : return "None";
            default: return "Unknown name";
        }
	}

	Maths::Colour ConsoleWindow::Message::GetRenderColour(Level level)
	{
		switch (level)
		{
			case ConsoleWindow::Message::Level::Trace   : return { 0.75f, 0.75f, 0.75f, 1.00f }; // White-ish gray
			case ConsoleWindow::Message::Level::Info    : return { 0.20f, 0.70f, 0.20f, 1.00f }; // Green
			case ConsoleWindow::Message::Level::Debug   : return { 0.00f, 0.50f, 0.50f, 1.00f }; // Cyan
			case ConsoleWindow::Message::Level::Warn    : return { 1.00f, 1.00f, 0.00f, 1.00f }; // Yellow
			case ConsoleWindow::Message::Level::Error   : return { 1.00f, 0.00f, 0.00f, 1.00f }; // Red
			case ConsoleWindow::Message::Level::Critical: return { 1.00f, 1.00f, 1.00f, 1.00f }; // White-white
            default: return { 1.00f, 1.00f, 1.00f, 1.00f };
		}
	}
}
