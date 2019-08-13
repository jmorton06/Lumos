#include "LM.h"
#include "LMLog.h"

#include "Editor/Console.h"
#include "Editor/ImGUIConsoleSink.h"
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Lumos
{
    std::shared_ptr<spdlog::logger> LMLog::s_CoreLogger;
    std::shared_ptr<spdlog::logger> LMLog::s_ClientLogger;
    
    void LMLog::OnInit()
    {
		Console::Instance();
		std::vector<spdlog::sink_ptr> sinks;
        sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>()); // debug console
		sinks.emplace_back(std::make_shared<ImGuiConsoleSink_mt>()); // ImGuiConsole
        
        auto logFileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("LumosLog.txt", 1048576 * 5, 3);
        sinks.emplace_back(logFileSink); // Log file

		// create the loggers
		s_CoreLogger = CreateRef<spdlog::logger>("LUMOS", begin(sinks), end(sinks));
		spdlog::register_logger(s_CoreLogger);
		s_ClientLogger = CreateRef<spdlog::logger>("APP", begin(sinks), end(sinks));
		spdlog::register_logger(s_ClientLogger);

		// configure the loggers
		spdlog::set_pattern("%^[%T] %n: %v%$");
		s_CoreLogger->set_level(spdlog::level::trace);
		s_ClientLogger->set_level(spdlog::level::trace);
    }

	void LMLog::OnRelease()
	{
		Console::Release();
		spdlog::shutdown();
	}
}
