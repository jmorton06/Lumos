#include "lmpch.h"
#include "InspectorWindow.h"
#include "Editor.h"
#include "App/Application.h"
#include "App/SceneManager.h"
#include "ECS/Component/Components.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Sprite.h"
#include "Graphics/Light.h"
#include "Maths/Transform.h"
#include "Scripting/ScriptComponent.h"

#include <imgui/imgui.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>

namespace MM {
    template <>
    void ComponentEditorWidget<Lumos::ScriptComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        ImGui::TextUnformatted("Loaded Functions : ");
        auto& script = reg.get<Lumos::ScriptComponent>(e);

        auto& solEnv = script.GetSolEnvironment();
        for (auto&& function : solEnv)
        {
            if (function.second.is<sol::function>())
            {
                ImGui::TextUnformatted(function.first.as<String>().c_str());
            }
        }

        if(ImGui::Button("Reload"))
            script.Reload();

        String filePath = script.GetFilePath();

        static char filePathBuffer[INPUT_BUF_SIZE];
        strcpy(filePathBuffer, filePath.c_str());

        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("##filePath", filePathBuffer, IM_ARRAYSIZE(filePathBuffer), 0))
            script.SetFilePath(filePathBuffer);
    }

    template <>
    void ComponentEditorWidget<Lumos::Maths::Transform>(entt::registry& reg, entt::registry::entity_type e)
    {
        auto& transform = reg.get<Lumos::Maths::Transform>(e);

        auto rotation = transform.GetLocalOrientation().EulerAngles();
        auto position = transform.GetLocalPosition();
        auto scale = transform.GetLocalScale();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Position");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::DragFloat3("##Position", Lumos::Maths::ValuePointer(position)))
        {
            transform.SetLocalPosition(position);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Rotation");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::DragFloat3("##Rotation", Lumos::Maths::ValuePointer(rotation)))
        {
            float pitch = Lumos::Maths::Min(rotation.x, 89.9f);
            pitch = Lumos::Maths::Max(pitch, -89.9f);
            transform.SetLocalOrientation(Lumos::Maths::Quaternion::EulerAnglesToQuaternion(pitch, rotation.y, rotation.z));
        
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Scale");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::DragFloat3("##Scale", Lumos::Maths::ValuePointer(scale), 0.1f))
        {
            transform.SetLocalScale(scale);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    template <>
    void ComponentEditorWidget<Lumos::MeshComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
    }

    template <>
    void ComponentEditorWidget<Lumos::Physics3DComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();
        auto& phys = reg.get<Lumos::Physics3DComponent>(e);

        auto pos = phys.GetPhysicsObject()->GetPosition();
        auto force = phys.GetPhysicsObject()->GetForce();
        auto torque = phys.GetPhysicsObject()->GetTorque();
        auto orientation = phys.GetPhysicsObject()->GetOrientation();
        auto angularVelocity = phys.GetPhysicsObject()->GetAngularVelocity();
        auto friction = phys.GetPhysicsObject()->GetFriction();
        auto isStatic = phys.GetPhysicsObject()->GetIsStatic();
        auto isRest = phys.GetPhysicsObject()->GetIsAtRest();
        auto mass = 1.0f / phys.GetPhysicsObject()->GetInverseMass();
        auto velocity = phys.GetPhysicsObject()->GetLinearVelocity();
        auto elasticity = phys.GetPhysicsObject()->GetElasticity();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Position");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::DragFloat3("##Position", Lumos::Maths::ValuePointer(pos)))
            phys.GetPhysicsObject()->SetPosition(pos);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Velocity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::DragFloat3("##Velocity", Lumos::Maths::ValuePointer(velocity)))
            phys.GetPhysicsObject()->SetLinearVelocity(velocity);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Torque");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::DragFloat3("##Torque", Lumos::Maths::ValuePointer(torque)))
            phys.GetPhysicsObject()->SetTorque(torque);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Orientation");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::DragFloat4("##Orientation", Lumos::Maths::ValuePointer(orientation)))
            phys.GetPhysicsObject()->SetOrientation(orientation);

        ImGui::PopItemWidth();
        ImGui::NextColumn();
        
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Force");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::DragFloat4("##Force", Lumos::Maths::ValuePointer(force)))
            phys.GetPhysicsObject()->SetForce(force);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Angular Velocity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::DragFloat3("##Angular Velocity", Lumos::Maths::ValuePointer(angularVelocity)))
            phys.GetPhysicsObject()->SetAngularVelocity(angularVelocity);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Friction");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::DragFloat("##Friction", &friction))
            phys.GetPhysicsObject()->SetFriction(friction);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Mass");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::DragFloat("##Mass", &mass))
            phys.GetPhysicsObject()->SetInverseMass(1.0f / mass);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Elasticity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::DragFloat("##Elasticity", &elasticity))
            phys.GetPhysicsObject()->SetElasticity(elasticity);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Static");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::Checkbox("##Static", &isStatic))
            phys.GetPhysicsObject()->SetIsStatic(isStatic);

        ImGui::PopItemWidth();
        ImGui::NextColumn();


        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("At Rest");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::Checkbox("##At Rest", &isRest))
            phys.GetPhysicsObject()->SetIsAtRest(isRest);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    template <>
    void ComponentEditorWidget<Lumos::Physics2DComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        auto& phys = reg.get<Lumos::Physics2DComponent>(e);

        auto pos = phys.GetPhysicsObject()->GetPosition();
        auto angle = phys.GetPhysicsObject()->GetAngle();
        auto friction = phys.GetPhysicsObject()->GetFriction();
        auto isStatic = phys.GetPhysicsObject()->GetIsStatic();
        auto isRest = phys.GetPhysicsObject()->GetIsAtRest();

        auto elasticity = phys.GetPhysicsObject()->GetElasticity();
        
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Position");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::DragFloat2("##Position", &pos.x))
            phys.GetPhysicsObject()->SetPosition(pos);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Orientation");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::DragFloat("##Orientation", &angle))
            phys.GetPhysicsObject()->SetOrientation(angle);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Friction");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::DragFloat("##Friction", &friction))
            phys.GetPhysicsObject()->SetFriction(friction);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Elasticity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::DragFloat("##Elasticity", &elasticity))
            phys.GetPhysicsObject()->SetElasticity(elasticity);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Static");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::Checkbox("##Static", &isStatic))
            phys.GetPhysicsObject()->SetIsStatic(isStatic);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("At Rest");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::Checkbox("##At Rest", &isRest))
            phys.GetPhysicsObject()->SetIsAtRest(isRest);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    template <>
    void ComponentEditorWidget<Lumos::SoundComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        auto& sound = reg.get<Lumos::SoundComponent>(e);

        auto pos = sound.GetSoundNode()->GetPosition();
        auto radius = sound.GetSoundNode()->GetRadius();
        auto paused = sound.GetSoundNode()->GetPaused();
        auto pitch = sound.GetSoundNode()->GetPitch();
        auto referenceDistance = sound.GetSoundNode()->GetReferenceDistance();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Position");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::InputFloat3("##Position", Lumos::Maths::ValuePointer(pos)))
            sound.GetSoundNode()->SetPosition(pos);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Radius");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::InputFloat("##Radius", &radius))
            sound.GetSoundNode()->SetRadius(radius);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Pitch");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::InputFloat("##Pitch", &pitch))
            sound.GetSoundNode()->SetPitch(pitch);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Reference Distance");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::DragFloat("##Reference Distance", &referenceDistance))
            sound.GetSoundNode()->SetReferenceDistance(referenceDistance);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Paused");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::Checkbox("##Paused", &paused))
            sound.GetSoundNode()->SetPaused(paused);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    template <>
    void ComponentEditorWidget<Lumos::CameraComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        auto& camera = reg.get<Lumos::CameraComponent>(e);

        if (camera.GetCamera() && ImGui::Button("Set Active"))
            camera.SetAsMainCamera();

        if(camera.GetCamera())
            camera.GetCamera()->OnImGui();
    }

    template <>
    void ComponentEditorWidget<Lumos::Graphics::Sprite>(entt::registry& reg, entt::registry::entity_type e)
    {
        auto& sprite = reg.get<Lumos::Graphics::Sprite>(e);

        sprite.OnImGui();
    }

    template <>
    void ComponentEditorWidget<Lumos::Graphics::Light>(entt::registry& reg, entt::registry::entity_type e)
    {
        auto& light = reg.get<Lumos::Graphics::Light>(e);

        light.OnImGui();
    }

    template <>
    void ComponentEditorWidget<Lumos::MaterialComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        auto& material = reg.get<Lumos::MaterialComponent>(e);

        material.OnImGui();
    }
}

namespace Lumos
{
	InspectorWindow::InspectorWindow()
	{
		m_Name = ICON_FA_INFO_CIRCLE" Inspector###inspector";
		m_SimpleName = "Inspector";
	}

    static bool init = false;
	void InspectorWindow::OnNewScene(Scene* scene)
	{
        if(init)
            return;
        
        init = true;
        
		auto& registry = scene->GetRegistry();
		auto& iconMap = m_Editor->GetComponentIconMap();

#define TRIVIAL_COMPONENT(ComponentType, ComponentName) \
				{ \
				String Name; \
				if(iconMap.find(typeid(ComponentType).hash_code()) != iconMap.end()) \
				Name += iconMap[typeid(ComponentType).hash_code()]; Name += "\t"; \
				Name += (ComponentName); \
				m_EnttEditor.registerComponent<ComponentType>(Name.c_str()); \
				}
		TRIVIAL_COMPONENT(Maths::Transform, "Transform");
		TRIVIAL_COMPONENT(MeshComponent, "Mesh");
		TRIVIAL_COMPONENT(CameraComponent, "Camera");
		TRIVIAL_COMPONENT(Physics3DComponent, "Physics3D");
		TRIVIAL_COMPONENT(Physics2DComponent, "Physics2D");
		TRIVIAL_COMPONENT(SoundComponent, "Sound");
		TRIVIAL_COMPONENT(Graphics::Sprite, "Sprite");
		TRIVIAL_COMPONENT(MaterialComponent, "Material");
		TRIVIAL_COMPONENT(Graphics::Light, "Light");
        TRIVIAL_COMPONENT(ScriptComponent, "LuaScript");
	}

	void InspectorWindow::OnImGui()
	{
        auto& registry = Application::Instance()->GetSceneManager()->GetCurrentScene()->GetRegistry();
		auto selected = m_Editor->GetSelected();

		if (ImGui::Begin(m_Name.c_str(), &m_Active))
		{
			if (selected == entt::null)
			{
				ImGui::End();
				return;
			}

			bool hasName = registry.has<NameComponent>(selected);
			String name;
			if (hasName)
				name = registry.get<NameComponent>(selected).name;
			else
				name = StringFormat::ToString(entt::to_integral(selected));

			static char objName[INPUT_BUF_SIZE];
			strcpy(objName, name.c_str());

			{
				if (registry.valid(selected))
				{
					ImGui::Text("ID: %d, Version: %d", static_cast<int>(registry.entity(selected)), registry.version(selected));
				}
				else
				{
					ImGui::TextUnformatted("INVALID ENTITY");
				}
			}

			ImGui::Indent();
			ImGui::TextUnformatted(ICON_FA_CUBE);
			ImGui::SameLine();

			//active checkbox
			//ImGui::SameLine();

			ImGui::PushItemWidth(-1);
			if (ImGui::InputText("##Name", objName, IM_ARRAYSIZE(objName), 0))
				registry.get_or_emplace<NameComponent>(selected).name = objName;

			ImGui::Unindent();
			ImGui::Separator();

			m_EnttEditor.RenderImGui(registry, selected);

		}
		ImGui::End();
	}
}
