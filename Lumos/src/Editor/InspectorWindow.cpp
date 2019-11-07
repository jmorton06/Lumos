#include "lmpch.h"
#include "InspectorWindow.h"
#include "Editor.h"
#include "ECS/EntityManager.h"
#include "Graphics/API/GraphicsContext.h"
#include "App/Application.h"
#include "App/SceneManager.h"
#include "ECS/Component/Components.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Sprite.h"
#include "Graphics/Material.h"
#include "Graphics/Light.h"

#include <imgui/imgui.h>

#include <IconFontCppHeaders/IconsFontAwesome5.h>

namespace Lumos
{
	static void TransformWidget(Maths::Transform& transform)
	{
		auto rotation = transform.GetLocalOrientation().ToEuler();
		auto position = transform.GetLocalPosition();
		auto scale = transform.GetLocalScale();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Position");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::DragFloat3("##Position", Maths::ValuePointer(position)))
		{
			transform.SetLocalPosition(position);
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Rotation");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::DragFloat3("##Rotation", Maths::ValuePointer(rotation)))
		{
			float pitch = Maths::Min(rotation.GetX(), 89.9f);
			pitch = Maths::Max(pitch, -89.9f);
			transform.SetLocalOrientation(Maths::Quaternion::EulerAnglesToQuaternion(pitch, rotation.GetY(), rotation.GetZ()));
		
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Scale");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::DragFloat3("##Scale", Maths::ValuePointer(scale), 0.1f))
		{
			transform.SetLocalScale(scale);
		}

		transform.SetWorldMatrix(Maths::Matrix4());

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar();
	}

	static void MeshWidget(MeshComponent& mesh)
	{
	}

	static void Physics3DWidget(Physics3DComponent& phys)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();

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
		ImGui::Text("Position");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::DragFloat3("##Position", Maths::ValuePointer(pos)))
			phys.GetPhysicsObject()->SetPosition(pos);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Velocity");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::DragFloat3("##Velocity", Maths::ValuePointer(velocity)))
			phys.GetPhysicsObject()->SetLinearVelocity(velocity);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Torque");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::DragFloat3("##Torque", Maths::ValuePointer(torque)))
			phys.GetPhysicsObject()->SetTorque(torque);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Orientation");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::DragFloat4("##Orientation", Maths::ValuePointer(orientation)))
			phys.GetPhysicsObject()->SetOrientation(orientation);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Angular Velocity");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::DragFloat3("##Angular Velocity", Maths::ValuePointer(angularVelocity)))
			phys.GetPhysicsObject()->SetAngularVelocity(angularVelocity);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Friction");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::DragFloat("##Friction", &friction))
			phys.GetPhysicsObject()->SetFriction(friction);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Mass");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::DragFloat("##Mass", &mass))
			phys.GetPhysicsObject()->SetInverseMass(1.0f / mass);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Elasticity");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::DragFloat("##Elasticity", &elasticity))
			phys.GetPhysicsObject()->SetElasticity(elasticity);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Static");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::Checkbox("##Static", &isStatic))
			phys.GetPhysicsObject()->SetIsStatic(isStatic);

		ImGui::PopItemWidth();
		ImGui::NextColumn();


		ImGui::AlignTextToFramePadding();
		ImGui::Text("At Rest");
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

	static void Physics2DWidget(Physics2DComponent& phys)
	{
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
		ImGui::Text("Position");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::DragFloat2("##Position", &pos.x))
			phys.GetPhysicsObject()->SetPosition(pos);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Orientation");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::DragFloat("##Orientation", &angle))
			phys.GetPhysicsObject()->SetOrientation(angle);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Friction");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::DragFloat("##Friction", &friction))
			phys.GetPhysicsObject()->SetFriction(friction);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Elasticity");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::DragFloat("##Elasticity", &elasticity))
			phys.GetPhysicsObject()->SetElasticity(elasticity);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Static");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::Checkbox("##Static", &isStatic))
			phys.GetPhysicsObject()->SetIsStatic(isStatic);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("At Rest");
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

	static void SoundWidget(SoundComponent& sound)
	{
		auto pos = sound.GetSoundNode()->GetPosition();
		auto radius = sound.GetSoundNode()->GetRadius();
		auto paused = sound.GetSoundNode()->GetPaused();
		auto pitch = sound.GetSoundNode()->GetPitch();
		auto referenceDistance = sound.GetSoundNode()->GetReferenceDistance();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Position");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::InputFloat3("##Position", Maths::ValuePointer(pos)))
			sound.GetSoundNode()->SetPosition(pos);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Radius");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::InputFloat("##Radius", &radius))
			sound.GetSoundNode()->SetRadius(radius);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Pitch");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::InputFloat("##Pitch", &pitch))
			sound.GetSoundNode()->SetPitch(pitch);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Reference Distance");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::DragFloat("##Reference Distance", &referenceDistance))
			sound.GetSoundNode()->SetReferenceDistance(referenceDistance);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Paused");
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

	static void CameraWidget(CameraComponent& camera)
	{
		if (camera.GetCamera() && ImGui::Button("Set Active"))
			camera.SetAsMainCamera();

		if(camera.GetCamera())
			camera.GetCamera()->OnImGui();
	}

	static void SpriteWidget(Graphics::Sprite& sprite)
	{
		sprite.OnImGui();
	}

	static void LightWidget(Graphics::Light& light)
	{
		light.OnImGui();
	}

	static void MaterialWidget(MaterialComponent& material)
	{
		material.OnImGui();
	}

	InspectorWindow::InspectorWindow()
	{
		m_Name = ICON_FA_INFO_CIRCLE" Inspector###inspector";
		m_SimpleName = "Inspector";
	}

	void InspectorWindow::OnNewScene(Scene* scene)
	{
		auto& registry = scene->GetRegistry();
		auto& iconMap = m_Editor->GetComponentIconMap();

#define TrivialComponent(ComponentType, ComponentName, func) \
				{ \
				String Name; \
				if(iconMap.find(typeid(ComponentType).hash_code()) != iconMap.end()) \
				Name += iconMap[typeid(ComponentType).hash_code()]; Name += "\t"; \
				Name += ComponentName; \
				m_EnttEditor.registerTrivial<ComponentType>(registry, Name.c_str()); \
				} \
				m_EnttEditor.registerComponentWidgetFn( \
					registry.type<ComponentType>(), \
					[](entt::registry& reg, auto e) { \
						auto& comp = reg.get<ComponentType>(e); \
						func(comp); });

		TrivialComponent(Maths::Transform, "Transform", TransformWidget);
		TrivialComponent(MeshComponent, "Mesh", MeshWidget);
		TrivialComponent(CameraComponent, "Camera", CameraWidget);
		TrivialComponent(Physics3DComponent, "Physics3D", Physics3DWidget);
		TrivialComponent(Physics2DComponent, "Physics2D", Physics2DWidget);
		TrivialComponent(SoundComponent, "Sound", SoundWidget);
		TrivialComponent(Graphics::Sprite, "Sprite", SpriteWidget);
		TrivialComponent(MaterialComponent, "Material", MaterialWidget);
		TrivialComponent(Graphics::Light, "Light", LightWidget);
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
				name = StringFormat::ToString(entt::to_integer(selected));

			static char objName[INPUT_BUF_SIZE];
			strcpy(objName, name.c_str());

			if (true)
			{
				if (registry.valid(selected))
				{
					ImGui::Text("ID: %d, Version: %d", registry.entity(selected), registry.version(selected));
				}
				else
				{
					ImGui::Text("INVALID ENTITY");
				}
			}

			ImGui::Indent();
			ImGui::TextUnformatted(ICON_FA_CUBE);
			ImGui::SameLine();

			//active checkbox
			//ImGui::SameLine();

			ImGui::PushItemWidth(-1);
			if (!hasName)
				registry.assign<NameComponent>(selected, name);
			ImGui::PushItemWidth(-1);
			if (ImGui::InputText("##Name", objName, IM_ARRAYSIZE(objName), 0))
				registry.get<NameComponent>(selected).name = objName;

			ImGui::Unindent();
			ImGui::Separator();

			m_EnttEditor.RenderImGui(registry, selected);

			ImGui::End();
		}
	}
}
