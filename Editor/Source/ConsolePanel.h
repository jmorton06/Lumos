#pragma once

#include "EditorPanel.h"

#include <imgui/imgui.h>
#include <Lumos/Core/Reference.h>
#include <Lumos/Core/DataStructures/TDArray.h>

namespace Lumos
{
    struct Mutex;
    enum class ConsoleLogLevel : int16_t
    {
        None  = -1,
        Trace = 1,
        Debug = 2,
        Info  = 4,
        Warn  = 8,
        Error = 16,
        Fatal = 32,
    };

    struct ConsoleMessage
    {
        ConsoleMessage(const std::string& message = "", ConsoleLogLevel level = ConsoleLogLevel::Trace, const std::string& source = "", int threadID = 0, const std::string& time = "");
        void OnImGUIRender();
        void IncreaseCount() { m_Count++; };
        size_t GetMessageID() const { return m_MessageID; }

        std::string m_Message;
        ConsoleLogLevel m_Level = ConsoleLogLevel::None;
        std::string m_Source;
        int m_ThreadID;
        std::string m_Time;
        int m_Count = 1;
        size_t m_MessageID;
    };

    class ConsolePanel : public EditorPanel
    {
    public:
        ConsolePanel();
        ~ConsolePanel();
        void OnImGui() override;

        static void AddMessage(const ConsoleMessage& message);

    private:
        void ImGuiRenderHeader();
        void ImGuiRenderMessages();

    private:
        static Mutex* m_MessageBufferMutex;
        static TDArray<ConsoleMessage> m_MessageBuffer;

        static bool s_AllowScrollingToBottom;
        static bool s_RequestScrollToBottom;
        static uint32_t s_MessageBufferRenderFilter;
        ImGuiTextFilter Filter;
    };
}
