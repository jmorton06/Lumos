#pragma once
#include "LM.h"
#include "Console.h"
#include <imgui/imgui.h>
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
            Console::Instance()->AddMessage(CreateRef<Console::Message>(fmt::to_string(formatted), GetMessageLevel(msg.level), source, static_cast<int>(msg.thread_id)));
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

		void flush_() override { Console::Instance()->Flush(); };
	};
}

#include "spdlog/details/null_mutex.h"
#include <mutex>
namespace Lumos
{
	using ImGuiConsoleSink_mt = ImGuiConsoleSink<std::mutex>;                  // multi-threaded
	using ImGuiConsoleSink_st = ImGuiConsoleSink<spdlog::details::null_mutex>; // single threaded
}
