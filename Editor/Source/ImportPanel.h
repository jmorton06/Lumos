#pragma once
#include <string>
#include <Lumos/Core/Asset/AssetImporter.h>

namespace Lumos
{
    class Editor;

    class ImportPanel
    {
    public:
        ImportPanel() = default;

        void Show(const std::string& sourcePath);
        void OnImGui();
        bool IsActive() const { return m_Active; }

    private:
        bool m_Active = false;
        std::string m_SourcePath;
        ImportSettings m_Settings;
    };
}
