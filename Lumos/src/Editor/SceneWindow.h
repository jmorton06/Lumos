
#include "EditorWindow.h"
#include "Maths/Maths.h"
#include "Maths/Frustum.h"
#include "Maths/Transform.h"
#include "Editor/Editor.h"
#include "Graphics/Camera/Camera.h"
#include "Core/StringUtilities.h"

#include "ImGui/ImGuiHelpers.h"

#include <imgui/imgui.h>
#include <entt/entt.hpp>

namespace Lumos
{
	class SceneWindow : public EditorWindow
	{
	public:
		SceneWindow();
		~SceneWindow() = default;

		void OnImGui() override;
		void OnNewScene(Scene* scene) override;
        void ToolBar();
		void DrawGizmos(float width, float height, float xpos, float ypos, Scene* scene);

		void Resize(u32 width, u32 height);
	private:

		template<typename T>
		void ShowComponentGizmo(float width, float height, float xpos, float ypos, const Maths::Matrix4& viewProj, const Maths::Frustum& frustum, entt::registry& registry)
		{
			if (m_ShowComponentGizmoMap[typeid(T).hash_code()])
			{
				auto group = registry.group<T>(entt::get<Maths::Transform>);

				for (auto entity : group)
				{
                    const auto &[component, trans] = group.template get<T, Maths::Transform>(entity);
					
					Maths::Vector3 pos = trans. GetWorldPosition();

					auto inside = frustum.IsInside(pos);

                    if (inside == Maths::Intersection::OUTSIDE)
                        continue;
                    
                    Maths::Vector2 screenPos = Maths::WorldToScreen(pos, viewProj, width, height, xpos, ypos);
                    ImGui::SetCursorPos({ screenPos.x - ImGui::GetFontSize() / 2.0f , screenPos.y - ImGui::GetFontSize() / 2.0f });
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));

                    if(ImGui::Button(m_Editor->GetComponentIconMap()[typeid(T).hash_code()]))
                    {
                        m_Editor->SetSelected(entity);
                    }
                
                    ImGui::PopStyleColor();

                    ImGuiHelpers::Tooltip(StringUtilities::Demangle(typeid(T).name()));
				}
			}
		}

		std::unordered_map<size_t, bool> m_ShowComponentGizmoMap;

		bool m_ShowStats = true;
        Ref<Graphics::Texture2D> m_GameViewTexture = nullptr;
		Scene* m_CurrentScene = nullptr;
		float m_AspectRatio;
		u32 m_Width, m_Height;

	};
}
