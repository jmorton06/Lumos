#include "LM.h"
#include "JMLog.h"

#include "spdlog/sinks/stdout_color_sinks.h"

namespace Lumos
{
    std::shared_ptr<spdlog::logger> JMLog::s_CoreLogger;
    std::shared_ptr<spdlog::logger> JMLog::s_ClientLogger;
    
    void JMLog::Init()
    {
        spdlog::set_pattern("%^[%T] %n: %v%$");
        s_CoreLogger = spdlog::stdout_color_mt("JM ");
        s_CoreLogger->set_level(spdlog::level::trace);
        
        s_ClientLogger = spdlog::stdout_color_mt("APP ");
        s_ClientLogger->set_level(spdlog::level::trace);
    }
}
