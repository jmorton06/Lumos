#pragma once
#include <string>
#include <Lumos/Core/Asset/AssetImporter.h>

namespace Lumos
{
    struct UIDockPanelContext;
    class Editor;

    class ImportPanel
    {
    public:
        ImportPanel() = default;

        void Show(const std::string& sourcePath);
        // The same form as a dock panel on the engine UI.
        void OnEditorUI(const UIDockPanelContext& context);
        bool IsActive() const { return m_Active; }
        void Close() { m_Active = false; }

    private:
        void Import();

        bool m_Active = false;
        std::string m_SourcePath;
        ImportSettings m_Settings;
    };
}
