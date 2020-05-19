#pragma once
#include "lmpch.h"
#include "ConsoleWindow.h"
#include <spdlog/sinks/base_sink.h>

namespace Lumos
{
	template<typename Mutex>
	class ImGuiConsoleSink : public spdlog::sinks::base_sink<Mutex>
	{
	public:
		explicit ImGuiConsoleSink()
        {
        };
        
		ImGuiConsoleSink(const ImGuiConsoleSink&) = delete;
		ImGuiConsoleSink& operator=(const ImGuiConsoleSink&) = delete;
		virtual ~ImGuiConsoleSink() = default;

		// SPDLog sink interface
        void sink_it_(const spdlog::details::log_msg& msg) override
		{
			fmt::memory_buffer formatted;
            spdlog::sinks::sink::formatter_->format(msg, formatted);
			String source = fmt::format("File : {0} | Function : {1} | Line : {2}", msg.source.filename, msg.source.funcname, msg.source.line);
            ConsoleWindow::AddMessage(CreateRef<ConsoleWindow::Message>(fmt::to_string(formatted), GetMessageLevel(msg.level), source, static_cast<int>(msg.thread_id)));
		}

        static ConsoleWindow::Message::Level GetMessageLevel(const spdlog::level::level_enum level)
		{
			switch (level)
			{
				case spdlog::level::level_enum::trace   : return ConsoleWindow::Message::Level::Trace;
				case spdlog::level::level_enum::debug   : return ConsoleWindow::Message::Level::Debug;
				case spdlog::level::level_enum::info    : return ConsoleWindow::Message::Level::Info;
				case spdlog::level::level_enum::warn    : return ConsoleWindow::Message::Level::Warn;
				case spdlog::level::level_enum::err     : return ConsoleWindow::Message::Level::Error;
				case spdlog::level::level_enum::critical: return ConsoleWindow::Message::Level::Critical;
				case spdlog::level::level_enum::off     : return ConsoleWindow::Message::Level::Off;
			}
			return ConsoleWindow::Message::Level::Invalid;
		}

		void flush_() override { ConsoleWindow::Flush(); };
	};
}

#include "spdlog/details/null_mutex.h"
#include <mutex>
namespace Lumos
{
	using ImGuiConsoleSink_mt = ImGuiConsoleSink<std::mutex>;                  // multi-threaded
	using ImGuiConsoleSink_st = ImGuiConsoleSink<spdlog::details::null_mutex>; // single threaded
}
