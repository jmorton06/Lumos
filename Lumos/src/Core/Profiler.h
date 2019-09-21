#pragma once
#include "lmpch.h"
#include "Utilities/Timer.h"
#include "Utilities/TSingleton.h"

#include <chrono>

#define PROFILERRECORD(name) \
Scope<Lumos::ProfilerRecord> profilerData = \
Lumos::Profiler::Instance()->IsEnabled() ? CreateScope<Lumos::ProfilerRecord>(name) : nullptr

namespace Lumos
{
    class ProfilerRecord
    {
    public:
        ProfilerRecord(const String& name);
        ~ProfilerRecord();
        
        const double EndTime() const { return m_EndTime; }
        const String& Name() const { return m_Name; }
    private:
        String m_Name;
        Scope<Timer> m_Timer;
        double m_EndTime;
    };
    
    class Profiler : public TSingleton<Profiler>
    {
        friend class TSingleton<Profiler>;
    public:
        Profiler();
        ~Profiler();
        
        void ClearHistory();
        void Update(float deltaTime);
        
        bool IsEnabled();
        void Enable();
        void Disable();
        void ToggleEnable();
        void Save(const ProfilerRecord& record);
        void OnImGui();
        
    private:
        bool m_Enabled;
        
        double m_lastTime;
        
        std::mutex m_SaveMutex;
        std::unordered_map<String, double> m_ElapsedHistory;
        std::unordered_map<String, uint64_t> m_CallsCounter;
        std::vector<std::thread::id> m_WorkingThreads;
        uint32_t m_ElapsedFrames;
    };
}
