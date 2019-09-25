#include "lmpch.h"
#include "Profiler.h"

#include <imgui/imgui.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>

namespace Lumos
{
    ProfilerRecord::ProfilerRecord(const String& name)
        : m_Name(name)
    {
        m_EndTime = 0.0f;
        m_Timer = CreateScope<Timer>();
    }
    
    ProfilerRecord::~ProfilerRecord()
    {
        m_EndTime = m_Timer->GetTimedMS();
        Profiler::Instance()->Save(*this);
    }
    
    Profiler::Profiler()
    {
        m_Timer = CreateScope<Timer>();
        m_UpdateTimer = 0.0f;
        m_UpdateFrequency = 0.1f;
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
            m_UpdateTimer += deltaTime;
            ++m_ElapsedFrames;
        }
    }
    
    void Profiler::ClearHistory()
    {
        m_ElapsedHistory.clear();
        m_CallsCounter.clear();
        m_WorkingThreads.clear();
        m_ElapsedFrames = 0;
        
        m_Timer->GetMS();
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

    ProfilerReport Profiler::GenerateReport()
    {
        ProfilerReport report;

        double time = m_Timer->GetMS();
        
        report.workingThreads = static_cast<uint16_t>((m_WorkingThreads.size() - 1) / m_ElapsedFrames);
        report.elapsedFrames = m_ElapsedFrames;
        report.elaspedTime = time;

        std::multimap<double, std::string> sortedHistory;

        for (auto& data : m_ElapsedHistory)
            sortedHistory.insert(std::pair<float, std::string>(data.second, data.first));

        for (auto& data : sortedHistory)
            report.actions.push_back({ data.second, data.first, (data.first / time), m_CallsCounter[data.second] });

        return report;
    }
    
    void Profiler::OnImGui()
    {
        ImGui::Begin(ICON_FA_STOPWATCH" Profiler###profiler");
        {
            ImGui::Checkbox("Profiler Enabled", &m_Enabled);
            ImGui::InputFloat("Update Frequency", &m_UpdateFrequency);
            ImGui::Text("Report period duration : %f ms", m_Report.elaspedTime);
            ImGui::Text("Threads : %i", m_Report.workingThreads);
            ImGui::Text("Frames : %i", m_Report.elapsedFrames);
            
            if (IsEnabled())
            {
                if ( m_UpdateTimer >= m_UpdateFrequency)
                {
                    m_Report = GenerateReport();
                    ClearHistory();
                    m_UpdateTimer = 0.0f;
                }
                
                if (m_Report.actions.empty())
                {
                    ImGui::Text("Collecting Data");
                }
                else
                {
                    for (const auto& action : m_Report.actions)
                    {
                        ImVec4 colour;
                        
                        if (action.percentage <= 25.0f)
                            colour = {0.2f,0.8f,0.2f,1.0f};
                        else if (action.percentage <= 50.0f)
                            colour = {0.2f,0.6f,0.6f,1.0f};
                        else if (action.percentage <= 75.0f)
                            colour = {0.7f,0.2f,0.3f,1.0f};
                        else
                            colour = {0.8f,0.2f,0.2f,1.0f};
                        
                        ImGui::TextColored(colour, "%s %f ms | %f ms per call | %f percent | %llu calls", action.name.c_str(), action.duration, action.duration / action.calls, action.percentage, action.calls);
                    }
                }
            }
            else
            {
                ImGui::Text("Profiler Disabled");
            }
        }
        ImGui::End();
    }
}
