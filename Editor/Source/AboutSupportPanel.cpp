#include "AboutSupportPanel.h"
#include <Lumos/Core/Version.h>
#include <Lumos/Core/OS/OS.h>
#include <Lumos/Utilities/StringUtilities.h>
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>
#include <Lumos/Core/Thread.h>

#include <imgui/imgui.h>
#include <cereal/version.hpp>
#include <entt/entt.hpp>

namespace Lumos
{
    AboutSupportPanel::AboutSupportPanel()
    {
        m_Name       = "About & Support###aboutsupport";
        m_SimpleName = "About & Support";
    }

    void AboutSupportPanel::OnImGui()
    {
        ImGui::Begin(m_Name.c_str(), &m_Active, ImGuiWindowFlags_NoCollapse);

        auto& version = LumosVersion;

        ImGui::TextUnformatted("Lumos Engine");
        ImGui::Text("Version %d.%d.%d", version.major, version.minor, version.patch);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // GitHub
        {
            std::string githubText = std::string(ICON_MDI_GITHUB_BOX) + " GitHub Repository";
            if(ImGui::Button(githubText.c_str(), ImVec2(-1, 0)))
                OS::Get().OpenURL("https://www.github.com/jmorton06/Lumos");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Third-party
        if(ImGui::TreeNodeEx("Third-Party Libraries", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ArenaTemp scratch = ScratchBegin(nullptr, 0);

            if(ImGui::Selectable((const char*)PushStr8F(scratch.arena, "ImGui - %s", IMGUI_VERSION).str))
                OS::Get().OpenURL("https://github.com/ocornut/imgui");
            if(ImGui::Selectable((const char*)PushStr8F(scratch.arena, "EnTT - %s", ENTT_VERSION).str))
                OS::Get().OpenURL("https://github.com/skypjack/entt");
            if(ImGui::Selectable((const char*)PushStr8F(scratch.arena, "Cereal - %i.%i.%i", CEREAL_VERSION_MAJOR, CEREAL_VERSION_MINOR, CEREAL_VERSION_PATCH).str))
                OS::Get().OpenURL("https://github.com/USCiLab/cereal");
            if(ImGui::Selectable((const char*)PushStr8F(scratch.arena, "Box2D - 3.0").str))
                OS::Get().OpenURL("https://github.com/erincatto/box2d");

            ScratchEnd(scratch);
            ImGui::TreePop();
        }

        // Contributors
        if(ImGui::TreeNodeEx("Contributors"))
        {
            if(ImGui::Selectable("Joe Morton"))
                OS::Get().OpenURL("https://github.com/jmorton06");
            if(ImGui::Selectable("RuanLucasGD"))
                OS::Get().OpenURL("https://github.com/RuanLucasGD");
            if(ImGui::Selectable("adriengivry"))
                OS::Get().OpenURL("https://github.com/adriengivry");
            ImGui::TreePop();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Support Development
        ImGui::TextUnformatted("Support Development");
        ImGui::Spacing();

        {
            std::string sponsorText = std::string(ICON_MDI_HEART) + " GitHub Sponsors";
            if(ImGui::Button(sponsorText.c_str(), ImVec2(-1, 0)))
                OS::Get().OpenURL("https://github.com/sponsors/jmorton06");
        }


        ImGui::End();
    }
}
