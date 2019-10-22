#pragma once
#include "lmpch.h"
#include "Utilities/Timer.h"
#include "Utilities/TSingleton.h"

#define LUMOS_PROFILE_BLOCK(name) \
Scope<Lumos::ProfilerRecord> profilerData = \
Lumos::Profiler::Instance()->IsEnabled() ? CreateScope<Lumos::ProfilerRecord>(name) : nullptr

#define LUMOS_PROFILE_FUNC LUMOS_PROFILE_BLOCK(__FUNCTION__)

namespace Lumos
{
    class LUMOS_EXPORT ProfilerRecord
    {
    public:
        ProfilerRecord(const String& name);
        ~ProfilerRecord();
        
        const float EndTime() const { return m_EndTime; }
        const String& Name() const { return m_Name; }
    private:
        String m_Name;
        Scope<Timer> m_Timer;
        float m_EndTime;
    };

    struct ProfilerReport
    {
        struct Action
        {
            String name;
            double duration;
            double percentage;
            uint64_t calls;
        };

        double elaspedTime;
        uint16_t workingThreads;
        uint32_t elapsedFrames;
        std::vector<Action> actions;
		std::vector<size_t> taskStatsIndex;
    };
    
    class LUMOS_EXPORT Profiler : public TSingleton<Profiler>
    {
        friend class TSingleton<Profiler>;
    public:
        Profiler();
        ~Profiler();
        
        void ClearHistory();
        void Update(float deltaTime);
        
        bool& IsEnabled();
        void Enable();
        void Disable();
        void ToggleEnable();
        void Save(const ProfilerRecord& record);
        
        ProfilerReport GenerateReport();
        
    private:
        bool m_Enabled;
        
        Scope<Timer> m_Timer;
        
        std::mutex m_SaveMutex;
        std::unordered_map<String, float> m_ElapsedHistory;
        std::unordered_map<String, uint64_t> m_CallsCounter;
        std::vector<std::thread::id> m_WorkingThreads;
        uint32_t m_ElapsedFrames;
    };
}
