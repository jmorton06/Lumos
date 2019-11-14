#include "lmpch.h"
#include "Profiler.h"

namespace Lumos
{
#ifdef LUMOS_PROFILER_ENABLED
	const char* ProfilerRecord::s_CurrentProfilerName = "";
#endif

    ProfilerRecord::ProfilerRecord(const char* name)
        : m_Name(name)
    {
#ifdef LUMOS_PROFILER_ENABLED
		m_Parent = s_CurrentProfilerName;
		s_CurrentProfilerName = name;
#endif

        m_EndTime = 0.0f;
    }
    
    ProfilerRecord::~ProfilerRecord()
    {
        m_EndTime = m_Timer.GetTimedMS();
        Profiler::Instance()->Save(*this);

#ifdef LUMOS_PROFILER_ENABLED
		s_CurrentProfilerName = m_Parent;
#endif
    }
    
    Profiler::Profiler()
    {
        m_ElapsedFrames = 0;
        m_Enabled = false;
    }
    
    Profiler::~Profiler()
    {
        
    }
    
    void Profiler::Update(float deltaTime)
    {
        if (IsEnabled())
        {
            ++m_ElapsedFrames;
        }
    }
    
    void Profiler::ClearHistory()
    {
        m_ElapsedHistory.clear();
        m_CallsCounter.clear();
        m_WorkingThreads.clear();
        m_ElapsedFrames = 0;
        
        m_Timer.GetMS();
    }
    
    bool& Profiler::IsEnabled()
    {
        return m_Enabled;
    }
    
    void Profiler::Enable()
    {
        LUMOS_LOG_INFO("Profiler Enabled");
        m_Enabled = true;
    }
    
    void Profiler::Disable()
    {
        LUMOS_LOG_INFO("Profiler Disabled");
        m_Enabled = false;
    }
    
    void Profiler::ToggleEnable()
    {
        m_Enabled = !m_Enabled;
        LUMOS_LOG_INFO(m_Enabled ? "Profiler Enabled" : "Profiler Disabled");
    }
    
    void Profiler::Save(const ProfilerRecord& record)
    {
        m_SaveMutex.lock();
        
        if (std::find(m_WorkingThreads.begin(), m_WorkingThreads.end(), std::this_thread::get_id()) == m_WorkingThreads.end())
            m_WorkingThreads.push_back(std::this_thread::get_id());
        
        if (m_ElapsedHistory.find(record.Name()) != m_ElapsedHistory.end())
        {
            m_ElapsedHistory[record.Name()] += record.EndTime();
        }
        else
        {
            m_ElapsedHistory[record.Name()] = record.EndTime();
        }
        
        if (m_CallsCounter.find(record.Name()) != m_CallsCounter.end())
        {
            ++m_CallsCounter[record.Name()];
        }
        else
        {
            m_CallsCounter[record.Name()] = 1;
        }
        
        m_SaveMutex.unlock();
    }

    ProfilerReport Profiler::GenerateReport()
    {
        ProfilerReport report;

        double time = m_Timer.GetTimedMS();
        
		if (m_ElapsedFrames == 0)
			return report;

        report.workingThreads = static_cast<uint16_t>((m_WorkingThreads.size() - 1) / m_ElapsedFrames);
        report.elapsedFrames = m_ElapsedFrames;
        report.elaspedTime = time;

        std::multimap<double, const char*> sortedHistory;

        for (auto& data : m_ElapsedHistory)
            sortedHistory.insert(std::pair<float, const char*>(data.second, data.first));

        for (auto& data : sortedHistory)
            report.actions.push_back({ data.second, data.first, (data.first / time) * 100.0f, m_CallsCounter[data.second] });

        return report;
    }
}
