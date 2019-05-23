#pragma once
#include "LM.h"
#include <deque>

#include <imgui/imgui.h>
#include <spdlog/sinks/sink.h>

namespace lumos
{
	class Console 
	{
	public:
		ImGuiTextBuffer     Buf;
		ImGuiTextFilter     Filter;
		ImVector<int>       LineOffsets;
		bool                AutoScroll;
		bool                ScrollToBottom;

		Console();
		void Clear();
		void AddLog(const char* fmt, ...) IM_FMTARGS(2);
		void Draw(const char* title, bool* p_open = NULL);
	};

	class ConsoleSink :
		public spdlog::sinks::sink 
	{
	public:
		ConsoleSink(Console& c) : console(c) {};

		// SPDLog sink interface
		void log(const spdlog::details::log_msg& msg) override;

		void flush() override { console.Clear(); };
		void set_pattern(const std::string& pattern) override {};
		void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override {}
	private:
		Console& console;
	};

}