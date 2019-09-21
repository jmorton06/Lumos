#include "lmpch.h"
#include "Profiler.h"

#include <imgui/imgui.h>

namespace Lumos
{
    ProfilerRecord::ProfilerRecord(const String& name)
    : m_Name(name)
    {
        m_Timer = CreateScope<Timer>();
    }
    
    ProfilerRecord::~ProfilerRecord()
    {
        m_EndTime = m_Timer->GetMS();
        Profiler::Instance()->Save(*this);
    }
    
    Profiler::Profiler()
    {
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
        
        m_lastTime = 0.0;
    }
    
    bool Profiler::IsEnabled()
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
    
    void Profiler::OnImGui()
    {
        ImGui::Begin("Profiler");
        {
            for(auto& data : m_ElapsedHistory)
            {
                ImGui::Text("%s %lf ms", data.first.c_str() , data.second);
            }
        }
        ImGui::End();
    }
}
