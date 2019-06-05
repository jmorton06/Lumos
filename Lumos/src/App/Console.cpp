#include "LM.h"
#include "Console.h"

namespace Lumos 
{
	Console::Console()
	{
		AutoScroll = true;
		ScrollToBottom = false;
		Clear();
	}

	void Console::Clear()
	{
		Buf.clear();
		LineOffsets.clear();
		LineOffsets.push_back(0);
	}

	void Console::AddLog(const char* fmt, ...)
	{
		int old_size = Buf.size();
		va_list args;
		va_start(args, fmt);
		Buf.appendfv(fmt, args);
		va_end(args);
		for (int new_size = Buf.size(); old_size < new_size; old_size++)
			if (Buf[old_size] == '\n')
				LineOffsets.push_back(old_size + 1);
		if (AutoScroll)
			ScrollToBottom = true;
	}

	void Console::Draw(const char* title, bool* p_open)
	{
		if (!ImGui::Begin(title, p_open))
		{
			ImGui::End();
			return;
		}

		// Options menu
		if (ImGui::BeginPopup("Options"))
		{
			if (ImGui::Checkbox("Auto-scroll", &AutoScroll))
				if (AutoScroll)
					ScrollToBottom = true;
			ImGui::EndPopup();
		}

		// Main window
		if (ImGui::Button("Options"))
			ImGui::OpenPopup("Options");
		ImGui::SameLine();
		bool clear = ImGui::Button("Clear");
		ImGui::SameLine();
		bool copy = ImGui::Button("Copy");
		ImGui::SameLine();
		Filter.Draw("Filter", -100.0f);

		ImGui::Separator();
		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

		if (clear)
			Clear();
		if (copy)
			ImGui::LogToClipboard();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		const char* buf = Buf.begin();
		const char* buf_end = Buf.end();
		if (Filter.IsActive())
		{
			for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
			{
				const char* line_start = buf + LineOffsets[line_no];
				const char* line_end = (line_no + 1 < LineOffsets.Size)
					                       ? (buf + LineOffsets[line_no + 1] - 1)
					                       : buf_end;
				if (Filter.PassFilter(line_start, line_end))
					ImGui::TextUnformatted(line_start, line_end);
			}
		}
		else
		{
			ImGuiListClipper clipper;
			clipper.Begin(LineOffsets.Size);
			while (clipper.Step())
			{
				for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
				{
					const char* line_start = buf + LineOffsets[line_no];
					const char* line_end = (line_no + 1 < LineOffsets.Size)
						                       ? (buf + LineOffsets[line_no + 1] - 1)
						                       : buf_end;
					ImGui::TextUnformatted(line_start, line_end);
				}
			}
			clipper.End();
		}
		ImGui::PopStyleVar();

		if (ScrollToBottom)
			ImGui::SetScrollHereY(1.0f);
		ScrollToBottom = false;
		ImGui::EndChild();
		ImGui::End();
	}

	void ConsoleSink::log(const spdlog::details::log_msg& msg)
	{
		fmt::memory_buffer formatted;
		sink::formatter_->format(msg, formatted);
		std::string str = fmt::to_string(formatted);
		ImVec4 color(255, 255, 255, 255);
		switch (msg.level) {
		case spdlog::level::trace:
			break;
		case spdlog::level::debug:
			break;
		case spdlog::level::warn:
			color = ImVec4(255, 48, 0, 255);
			break;
		case spdlog::level::err:
			color = ImVec4(255, 0, 0, 255);
			break;
		case spdlog::level::critical:
			color = ImVec4(255, 0, 0, 255);
			break;
		case spdlog::level::info:
		default:
			;
		}
		console.AddLog(str.c_str(), color);
	}

}