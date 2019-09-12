#include "lmpch.h"
#include "Console.h"

namespace Lumos 
{
	Console::Message::Level Console::s_MessageBufferRenderFilter = Console::Message::Level::Trace;

    Console::Console()
	{
	    m_MessageBufferCapacity = 200;
		m_MessageBufferSize = 0;
		m_MessageBufferBegin = 0;
		m_MessageBuffer.resize(m_MessageBufferCapacity);
		m_AllowScrollingToBottom = true;
		m_RequestScrollToBottom = false;
	}

	void Console::AddMessage(const Ref<Message>& message)
	{
		if (message->m_Level == Message::Level::Invalid)
			return;

		*(m_MessageBuffer.begin() + m_MessageBufferBegin) = message;
		if (++m_MessageBufferBegin == m_MessageBufferCapacity)
			m_MessageBufferBegin = 0;
		if (m_MessageBufferSize < m_MessageBufferCapacity)
			m_MessageBufferSize++;

		if (m_AllowScrollingToBottom)
			m_RequestScrollToBottom = true;
	}

	void Console::Flush()
	{
		for (auto message = m_MessageBuffer.begin(); message != m_MessageBuffer.end(); message++)
			(*message) = CreateRef<Message>();
		m_MessageBufferBegin = 0;
	}

	void Console::OnImGuiRender(bool* show)
	{
		auto flags = ImGuiWindowFlags_NoCollapse;
		ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_FirstUseEver);
		ImGui::Begin("Console", show, flags);
		{
            ImGuiRenderHeader();
			ImGui::Separator();
			ImGuiRenderMessages();
		}
		ImGui::End();
	}

	void Console::ImGuiRenderHeader()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		const float spacing = style.ItemInnerSpacing.x;

		// Text change level
		ImGui::AlignFirstTextHeightToWidgets();
		ImGui::Text("Display");

		ImGui::SameLine(0.0f, 2.0f * spacing);

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

		// Buttons to quickly change level
		if (ImGui::ArrowButton("##MessageRenderFilter_L", ImGuiDir_Left))
		{
			s_MessageBufferRenderFilter = Message::GetLowerLevel(s_MessageBufferRenderFilter);
		}

		ImGui::SameLine(0.0f, spacing);

		if (ImGui::ArrowButton("##MessageRenderFilter_R", ImGuiDir_Right))
		{
			s_MessageBufferRenderFilter = Message::GetHigherLevel(s_MessageBufferRenderFilter);
		}

		ImGui::SameLine(0.0f, spacing);

		// Button for advanced settings
		if (ImGui::Button("Settings"))
			ImGui::OpenPopup("SettingsPopup");
		if (ImGui::BeginPopup("SettingsPopup"))
		{
			// Checkbox for scrolling lock
			ImGui::Checkbox("Scroll to bottom", &m_AllowScrollingToBottom);

			// Button to clear the console
			if (ImGui::Button("Clear console"))
				Flush();

			ImGui::EndPopup();
		}

		ImGui::SameLine();
		Filter.Draw("Filter", -100.0f);

	}

	void Console::ImGuiRenderMessages()
	{
		ImGui::BeginChild("ScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
		{
			auto messageStart = m_MessageBuffer.begin() + m_MessageBufferBegin;
			if (*messageStart) // If contains old message here
			{
				for (auto message = messageStart; message != m_MessageBuffer.end(); message++)
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
					
			if (m_MessageBufferBegin != 0) // Skipped first messages in vector
			{
				for (auto message = m_MessageBuffer.begin(); message != messageStart; message++)
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
					
			if (m_RequestScrollToBottom && ImGui::GetScrollMaxY() > 0)
			{
				ImGui::SetScrollY(ImGui::GetScrollMaxY());
				m_RequestScrollToBottom = false;
			}
		}
		ImGui::EndChild();
	}

	std::vector<Console::Message::Level> Console::Message::s_Levels{
		Console::Message::Level::Trace,
		//Console::Message::Level::Debug,
		Console::Message::Level::Info,
		Console::Message::Level::Warn,
		Console::Message::Level::Error,
		Console::Message::Level::Critical,
		Console::Message::Level::Off
	};

	Console::Message::Message(const String& message, Level level, const String& source, int threadID)
		: m_Message(message), m_Level(level), m_Source(source), m_ThreadID(threadID)
	{
	}

	void Console::Message::OnImGUIRender()
	{
		if (m_Level != Level::Invalid && m_Level >= s_MessageBufferRenderFilter)
		{
			Colour colour = GetRenderColour(m_Level);
			ImGui::PushStyleColor(ImGuiCol_Text, { colour.r, colour.g, colour.b, colour.a });
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

	Console::Message::Level Console::Message::GetLowerLevel(Level level)
	{
		switch (level)
		{
			case Console::Message::Level::Off     : return Console::Message::Level::Critical;
			case Console::Message::Level::Critical: return Console::Message::Level::Error;
			case Console::Message::Level::Error   : return Console::Message::Level::Warn;
			case Console::Message::Level::Warn    : //return Console::Message::Level::Debug;
			case Console::Message::Level::Debug   : return Console::Message::Level::Info;
			case Console::Message::Level::Info    :
			case Console::Message::Level::Trace   : return Console::Message::Level::Trace;
            default: return Console::Message::Level::Invalid;
		}
	}

	Console::Message::Level Console::Message::GetHigherLevel(Level level)
	{
		switch (level)
		{
            case Console::Message::Level::Trace   : return Console::Message::Level::Info;
			case Console::Message::Level::Info    : //return Console::Message::Level::Debug;
            case Console::Message::Level::Debug   : return Console::Message::Level::Warn;
            case Console::Message::Level::Warn    : return Console::Message::Level::Error;
            case Console::Message::Level::Error   : return Console::Message::Level::Critical;
			case Console::Message::Level::Critical:
            case Console::Message::Level::Off     : return Console::Message::Level::Off;
            default: return Console::Message::Level::Invalid;
		}
	}


	const char* Console::Message::GetLevelName(Level level)
	{
		switch (level)
		{
			case Console::Message::Level::Trace   : return "Trace";
			case Console::Message::Level::Info    : return "Info";
			case Console::Message::Level::Debug   : return "Debug";
			case Console::Message::Level::Warn    : return "Warning";
			case Console::Message::Level::Error   : return "Error";
			case Console::Message::Level::Critical: return "Critical";
			case Console::Message::Level::Off     : return "None";
            default: return "Unknown name";
        }
	}

	Console::Message::Colour Console::Message::GetRenderColour(Level level)
	{
		switch (level)
		{
			case Console::Message::Level::Trace   : return { 0.75f, 0.75f, 0.75f, 1.00f }; // White-ish gray
			case Console::Message::Level::Info    : return { 0.00f, 0.50f, 0.00f, 1.00f }; // Green
			case Console::Message::Level::Debug   : return { 0.00f, 0.50f, 0.50f, 1.00f }; // Cyan
			case Console::Message::Level::Warn    : return { 1.00f, 1.00f, 0.00f, 1.00f }; // Yellow
			case Console::Message::Level::Error   : return { 1.00f, 0.00f, 0.00f, 1.00f }; // Red
			case Console::Message::Level::Critical: return { 1.00f, 1.00f, 1.00f, 1.00f }; // White-white
            default: return { 1.00f, 1.00f, 1.00f, 1.00f };
		}
	}
}
