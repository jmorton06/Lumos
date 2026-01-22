#pragma once
#include "EditorPanel.h"
#include <string>
#include <vector>

namespace Lumos
{
    class LuaDebugPanel : public EditorPanel
    {
    public:
        LuaDebugPanel();
        ~LuaDebugPanel() = default;

        void OnImGui() override;
        void OnNewScene(Scene* scene) override;

    private:
        void DrawGlobalsTab();
        void DrawWatchTab();
        void DrawScriptsTab();
        void DrawMemoryTab();

        std::string EvaluateExpression(const std::string& expr);
        void RefreshGlobals();

        struct WatchEntry
        {
            std::string Expression;
            std::string Result;
            bool Error = false;
        };

        std::vector<WatchEntry> m_WatchList;
        std::vector<std::pair<std::string, std::string>> m_Globals;
        char m_WatchInput[256] = {0};
        char m_GlobalFilter[128] = {0};
        Scene* m_CurrentScene = nullptr;
        bool m_AutoRefresh = true;
        float m_RefreshTimer = 0.0f;
        float m_RefreshInterval = 0.5f;
    };
}
