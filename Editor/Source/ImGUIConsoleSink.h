#pragma once

#include "ConsolePanel.h"
#include <spdlog/sinks/base_sink.h>
#include <spdlog/fmt/chrono.h>

namespace Lumos
{
    template <typename Mutex>
    class ImGuiConsoleSink : public spdlog::sinks::base_sink<Mutex>
    {
    public:
        explicit ImGuiConsoleSink()
            : m_MessageBufferCapacity(1)
        {
            m_MessageBuffer.PushBack({});
        };

        ImGuiConsoleSink(const ImGuiConsoleSink&)            = delete;
        ImGuiConsoleSink& operator=(const ImGuiConsoleSink&) = delete;
        virtual ~ImGuiConsoleSink()                          = default;

        // SPDLog sink interface
        void sink_it_(const spdlog::details::log_msg& msg) override
        {
            spdlog::memory_buf_t formatted;
            spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
            std::string source = fmt::format("File : {0} | Function : {1} | Line : {2}", msg.source.filename, msg.source.funcname, msg.source.line);
            const auto time    = fmt::localtime(std::chrono::system_clock::to_time_t(msg.time));
            auto processed     = fmt::format("{:%H:%M:%S}", time);

            m_MessageBuffer[m_MessageCount++] = ConsoleMessage(fmt::to_string(formatted), GetMessageLevel(msg.level), source, static_cast<int>(msg.thread_id), processed);

            if(m_MessageCount == m_MessageBufferCapacity)
                flush_();
        }

        static ConsoleLogLevel GetMessageLevel(const spdlog::level::level_enum level)
        {
            switch(level)
            {
            case spdlog::level::level_enum::trace:
                return ConsoleLogLevel::Trace;
            case spdlog::level::level_enum::debug:
                return ConsoleLogLevel::Debug;
            case spdlog::level::level_enum::info:
                return ConsoleLogLevel::Info;
            case spdlog::level::level_enum::warn:
                return ConsoleLogLevel::Warn;
            case spdlog::level::level_enum::err:
                return ConsoleLogLevel::Error;
            case spdlog::level::level_enum::critical:
                return ConsoleLogLevel::Critical;
            }
            return ConsoleLogLevel::Trace;
        }

        void flush_() override
        {
            for(const auto& message : m_MessageBuffer)
                ConsolePanel::AddMessage(message);

            m_MessageCount = 0;
        };

    private:
        uint32_t m_MessageBufferCapacity;
        Vector<ConsoleMessage> m_MessageBuffer;
        uint32_t m_MessageCount = 0;
    };
}

#include "spdlog/details/null_mutex.h"
#include <mutex>
namespace Lumos
{
    using ImGuiConsoleSink_mt = ImGuiConsoleSink<std::mutex>;                  // multi-threaded
    using ImGuiConsoleSink_st = ImGuiConsoleSink<spdlog::details::null_mutex>; // single threaded
}
