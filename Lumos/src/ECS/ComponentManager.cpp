#include "lmpch.h"
#include "ComponentManager.h"
#include "ECS/Component/Components.h"
#include "Graphics/Sprite.h"
#include "Graphics/Light.h"
#include "Maths/Transform.h"

#include <imgui/imgui.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>

namespace Lumos
{
	ComponentManager::ComponentManager()
	{
		m_NextComponentType = 0;
		RegisterComponent<Maths::Transform>();
		RegisterComponent<Graphics::Sprite>();
		RegisterComponent<Graphics::Light>();
		RegisterComponent<MeshComponent>();
		RegisterComponent<SoundComponent>();
		RegisterComponent<TextureMatrixComponent>();
		RegisterComponent<Physics3DComponent>();
		RegisterComponent<Physics2DComponent>();
		RegisterComponent<AIComponent>();
		RegisterComponent<ParticleComponent>();
		RegisterComponent<CameraComponent>();
		RegisterComponent<MaterialComponent>();
	}

	ComponentManager::~ComponentManager()
	{
	}

	void ComponentManager::OnUpdate()
	{
		LUMOS_PROFILE_FUNC;
		for (auto& componentArray : m_ComponentArrays)
			componentArray.second->OnUpdate();
	}

	void IComponentArray::ImGuiComponentHeader(const String& componentName, size_t typeID, Entity* entity, bool& open,bool hasActive, bool& active)
	{
		ImGui::Separator();

		String name = componentName.substr(componentName.find_last_of(':') + 1);

		open = ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap);

		if (typeID != typeid(Maths::Transform).hash_code())
		{
			const float ItemSpacing = ImGui::GetStyle().ItemSpacing.x;

			const float HostButtonWidth = 42.0f;
			float pos = HostButtonWidth + ItemSpacing;
			ImGui::SameLine(ImGui::GetWindowWidth() - pos);

			if (hasActive)
			{
				ImGui::Checkbox(("##Active" + componentName).c_str(), &active);
				ImGui::SameLine();
			}
	
			if (ImGui::Button((ICON_FA_COG"##" + componentName).c_str()))
				ImGui::OpenPopup(("Remove Component" + componentName).c_str());

			if (ImGui::BeginPopup(("Remove Component" + componentName).c_str(), 3))
			{
				if (ImGui::Selectable(("Remove##" + componentName).c_str())) RemoveData(entity);
				ImGui::EndPopup();
			}
		}
	}
}
