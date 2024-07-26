#include "AssetManagerPanel.h"

#include <Lumos/Core/Application.h>
#include <Lumos/Scene/SceneManager.h>
#include <Lumos/Scene/Scene.h>
#include <Lumos/Core/Asset/AssetRegistry.h>
#include <Lumos/Core/Asset/AssetManager.h>

#include "Editor.h"

#include <Lumos/Core/Engine.h>
#include <Lumos/Core/OS/Window.h>
#include <Lumos/Graphics/Renderers/SceneRenderer.h>
#include <Lumos/ImGui/ImGuiUtilities.h>
#include <Lumos/Utilities/StringUtilities.h>
#include <imgui/imgui.h>
#include <imgui/Plugins/implot/implot.h>

namespace Lumos
{
    AssetManagerPanel::AssetManagerPanel()
    {
        m_Name       = "AssetManagerPanel###AssetManagerPanel";
        m_SimpleName = "Asset Manager";
    }

    void AssetManagerPanel::OnImGui()
    {
        auto flags = ImGuiWindowFlags_NoCollapse;
        if(ImGui::Begin(m_Name.c_str(), &m_Active, flags))
        {
            ImGuiUtilities::PushID();

            enum MyItemColumnID
            {
                MyItemColumnID_ID,
                MyItemColumnID_Name,
                MyItemColumnID_Type,
                MyItemColumnID_Accessed
            };

            if(ImGui::BeginTable("Asset Registry", 4, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg))
            {
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, MyItemColumnID_ID);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, MyItemColumnID_Name);
                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoSort, 0.0f, MyItemColumnID_Type);
                ImGui::TableSetupColumn("Last Accessed", ImGuiTableColumnFlags_NoSort, 0.0f, MyItemColumnID_Accessed);

                ImGui::TableSetupScrollFreeze(0, 1);

                ImGui::TableHeadersRow();
                ImGui::TableNextRow();
                Lumos::AssetRegistry& registry = *m_Editor->GetAssetManager()->GetAssetRegistry();

                auto DrawEntry = [&registry](AssetMetaData& metaData, uint64_t ID)
                {
                    {
                        ImGui::TableNextColumn();
                        // ImGui::TextUnformatted(fmt::format("{0}", ID).c_str()); //"%lld", ID);

                        ImGui::TableNextColumn();
                        std::string name = "Unnamed";
#ifndef LUMOS_PRODUCTION
                        registry.GetName(ID, name);
#endif
                        ImGui::TextUnformatted(name.c_str());

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(AssetTypeToString(metaData.Type));

                        ImGui::TableNextColumn();
                        ImGui::Text("%.2f", metaData.lastAccessed);

                        ImGui::TableNextRow();
                    }
                };

                for(auto& current : registry)
                {
                    DrawEntry(current.second, current.first);
                }

                ImGui::EndTable();
            }
            ImGuiUtilities::PopID();
        }
        ImGui::End();
    }
}
