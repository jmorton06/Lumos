#pragma once
#include "LM.h"
#include "Console.h"
#include <imgui/imgui.h>
#include <spdlog/sinks/sink.h>

namespace Lumos
{
	class ImGuiConsoleSink : public spdlog::sinks::sink 
	{
	public:
		explicit ImGuiConsoleSink(Console& console) : m_Console(console)
        {
        };
        
		ImGuiConsoleSink(const ImGuiConsoleSink&) = delete;
		ImGuiConsoleSink& operator=(const ImGuiConsoleSink&) = delete;
		virtual ~ImGuiConsoleSink() = default;

		// SPDLog sink interface
        void log(const spdlog::details::log_msg& msg) override
		{
			fmt::memory_buffer formatted;
			sink::formatter_->format(msg, formatted);
            m_Console.AddMessage(std::make_shared<Console::Message>(fmt::to_string(formatted), GetMessageLevel(msg.level)));
		}

        static Console::Message::Level GetMessageLevel(const spdlog::level::level_enum level)
		{
			switch (level)
			{
				case spdlog::level::level_enum::trace   : return Console::Message::Level::Trace;
				case spdlog::level::level_enum::debug   : return Console::Message::Level::Debug;
				case spdlog::level::level_enum::info    : return Console::Message::Level::Info;
				case spdlog::level::level_enum::warn    : return Console::Message::Level::Warn;
				case spdlog::level::level_enum::err     : return Console::Message::Level::Error;
				case spdlog::level::level_enum::critical: return Console::Message::Level::Critical;
				case spdlog::level::level_enum::off     : return Console::Message::Level::Off;
			}
			return Console::Message::Level::Invalid;
		}

		void flush() override { m_Console.Flush(); };
		void set_pattern(const std::string& pattern) override {};
		void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override {}
	private:
		Console& m_Console;
	};
}
