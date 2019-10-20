#include "lmpch.h"
#include "EditorWindow.h"
#include "Maths/Maths.h"
#include "Maths/Frustum.h"
#include "ECS/ComponentManager.h"

#include "ImGui/ImGuiHelpers.h"

#include <imgui/imgui.h>

namespace Lumos
{
	class SceneWindow : public EditorWindow
	{
	public:
		SceneWindow();
		~SceneWindow() = default;

		void OnImGui() override;
        void ToolBar();
		void DrawGizmos(float width, float height, float xpos, float ypos);

	private:

		template<typename T>
		void ShowComponentGizmo(float width, float height, float xpos, float ypos, const Maths::Matrix4& viewProj, const Maths::Frustum& frustum)
		{
			if (m_ShowComponentGizmoMap[typeid(T).hash_code()])
			{
				auto& components = *ComponentManager::Instance()->GetComponentArray<T>();
				auto size = components.GetSize();
				for (int i = 0; i < size; i++)
				{
					auto component = components[i];
					auto entity = components.GetEntity(component);
					if (component && entity)
					{
						Maths::Vector3 pos = entity->GetTransformComponent()->GetWorldPosition();

						if (frustum.InsideFrustum(pos, 0.1f))
						{
							Maths::Vector2 screenPos = Maths::WorldToScreen(pos, viewProj, width, height);
							ImGui::SetCursorPos({ screenPos.x - ImGui::GetFontSize() / 2.0f , screenPos.y - ImGui::GetFontSize() / 2.0f });
                            ImGui::Text("%s", m_ComponentIconMap[typeid(T).hash_code()]);

							ImGuiHelpers::Tooltip(components.GetName().c_str());
						}
					}
				}
			}
		}

		std::unordered_map<size_t, const char*> m_ComponentIconMap;
		std::unordered_map<size_t, bool> m_ShowComponentGizmoMap;
	};
}
