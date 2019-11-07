#include "lmpch.h"
#include "EditorWindow.h"
#include "Maths/Maths.h"
#include "Maths/Frustum.h"
#include "Maths/Transform.h"
#include "Editor/Editor.h"

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
        void ToolBar();
		void DrawGizmos(float width, float height, float xpos, float ypos, Scene* scene);

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

					if (frustum.InsideFrustum(pos, 0.1f))
					{
						Maths::Vector2 screenPos = Maths::WorldToScreen(pos, viewProj, width, height, xpos, ypos);
						ImGui::SetCursorPos({ screenPos.x - ImGui::GetFontSize() / 2.0f , screenPos.y - ImGui::GetFontSize() / 2.0f });
                        ImGui::TextUnformatted(m_Editor->GetComponentIconMap()[typeid(T).hash_code()]);

						ImGuiHelpers::Tooltip(typeid(T).name());
					}
				}
			}
		}

		std::unordered_map<size_t, bool> m_ShowComponentGizmoMap;

		bool m_ShowStats = true;
	};
}
