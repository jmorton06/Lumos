#include "LM.h"
#include "LMLog.h"

#include "spdlog/sinks/stdout_color_sinks.h"

namespace Lumos
{
    std::shared_ptr<spdlog::logger> LMLog::s_CoreLogger;
    std::shared_ptr<spdlog::logger> LMLog::s_ClientLogger;
    
    void LMLog::OnInit()
    {
        spdlog::set_pattern("%^[%T] %n: %v%$");
        s_CoreLogger = spdlog::stdout_color_mt("LUMOS");
        s_CoreLogger->set_level(spdlog::level::trace);
        
        s_ClientLogger = spdlog::stdout_color_mt("APP   ");
        s_ClientLogger->set_level(spdlog::level::trace);
    }
}
