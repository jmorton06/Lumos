
#include "EditorPanel.h"
#include "Editor.h"
#include <Lumos/Maths/Frustum.h>
#include <Lumos/Maths/Transform.h>
#include <Lumos/Utilities/StringUtilities.h>
#include <Lumos/ImGui/ImGuiUtilities.h>
#include <Lumos/Maths/MathsUtilities.h>

#include <imgui/imgui.h>
DISABLE_WARNING_PUSH
DISABLE_WARNING_CONVERSION_TO_SMALLER_TYPE
#include <entt/entt.hpp>
DISABLE_WARNING_POP

namespace Lumos
{
    class SceneViewPanel : public EditorPanel
    {
    public:
        SceneViewPanel();
        ~SceneViewPanel() = default;

        void OnImGui() override;
        void OnNewScene(Scene* scene) override;
        void ToolBar();
        void DrawGizmos(float width, float height, float xpos, float ypos, Scene* scene);

        void Resize(uint32_t width, uint32_t height);

    private:
        template <typename T>
        void ShowComponentGizmo(float width, float height, float xpos, float ypos, const glm::mat4& viewProj, const Maths::Frustum& frustum, entt::registry& registry)
        {
            if(m_ShowComponentGizmoMap[typeid(T).hash_code()])
            {
                auto group = registry.group<T>(entt::get<Maths::Transform>);

                for(auto entity : group)
                {
                    const auto& [component, trans] = group.template get<T, Maths::Transform>(entity);

                    glm::vec3 pos = trans.GetWorldPosition();

                    auto inside = frustum.IsInside(pos);

                    if(inside == Maths::Intersection::OUTSIDE)
                        continue;

                    glm::vec2 screenPos = Maths::WorldToScreen(pos, viewProj, width, height, xpos, ypos);
                    ImGui::SetCursorPos({ screenPos.x - ImGui::GetFontSize() * 0.5f, screenPos.y - ImGui::GetFontSize() * 0.5f });
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));

                    // if(ImGui::Button(m_Editor->GetComponentIconMap()[typeid(T).hash_code()]))
                    //{
                    //     m_Editor->SetSelected(entity);
                    // }

                    ImGui::TextUnformatted(m_Editor->GetComponentIconMap()[typeid(T).hash_code()]);
                    ImGui::PopStyleColor();

                    const char* demangledName = nullptr;
                    auto found                = m_DemangledNames.find(typeid(T).hash_code());
                    if(found == m_DemangledNames.end())
                    {
                        m_DemangledNames[typeid(T).hash_code()] = StringUtilities::Demangle(typeid(T).name());
                        demangledName                           = m_DemangledNames[typeid(T).hash_code()].c_str();
                    }
                    else
                        demangledName = found->second.c_str();

                    ImGuiUtilities::Tooltip(demangledName);
                }
            }
        }

        std::unordered_map<size_t, bool> m_ShowComponentGizmoMap;
        std::unordered_map<size_t, std::string> m_DemangledNames;

        bool m_ShowStats                                 = false;
        SharedPtr<Graphics::Texture2D> m_GameViewTexture = nullptr;
        Scene* m_CurrentScene                            = nullptr;
        uint32_t m_Width, m_Height;
    };
}
