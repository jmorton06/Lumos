#pragma once
#include "lmpch.h"
#include "Utilities/Timer.h"
#include "Utilities/TSingleton.h"

#ifdef LUMOS_PROFILER_ENABLED
#define LUMOS_PROFILE_BLOCK(name) \
UniqueRef<Lumos::ProfilerRecord> profilerData \
 = Lumos::Profiler::Get().IsEnabled() ? CreateUniqueRef<Lumos::ProfilerRecord>(name) : nullptr;

#define LUMOS_PROFILE_FUNC LUMOS_PROFILE_BLOCK(__FUNCTION__)
#else
#define LUMOS_PROFILE_BLOCK(x)
#define LUMOS_PROFILE_FUNC
#endif

namespace Lumos
{
    class LUMOS_EXPORT ProfilerRecord
    {
#ifdef LUMOS_PROFILER_ENABLED
		static const char* s_CurrentProfilerName;
#endif
    public:
		explicit ProfilerRecord(const char* name);
        ~ProfilerRecord();
        
        const float EndTime() const { return m_EndTime; }
		const char* Name() const { return m_Name; }
    private:
        const char* m_Name;
		const char* m_Parent;
        Timer m_Timer;
        float m_EndTime;
    };

    struct ProfilerReport
    {
        struct Action
        {
			const char* name;
            double duration;
            double percentage;
            uint64_t calls;
        };

        double elaspedTime = 0.0;
        uint16_t workingThreads;
        uint32_t elapsedFrames;
        std::vector<Action> actions;
		std::vector<size_t> taskStatsIndex;
    };
    
    class LUMOS_EXPORT Profiler : public ThreadSafeSingleton<Profiler>
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
        
        Timer m_Timer;
        
        std::mutex m_SaveMutex;
        std::unordered_map<const char*, float> m_ElapsedHistory;
        std::unordered_map<const char*, uint64_t> m_CallsCounter;
        std::vector<std::thread::id> m_WorkingThreads;
        uint32_t m_ElapsedFrames;
    };
}
