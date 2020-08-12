#include "lmpch.h"
#include "InspectorWindow.h"
#include "Editor.h"
#include "Core/Application.h"
#include "Scene/SceneManager.h"
#include "Scene/Component/Components.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Sprite.h"
#include "Graphics/Light.h"
#include "Graphics/Environment.h"
#include "Graphics/API/Texture.h"
#include "Graphics/API/GraphicsContext.h"
#include "Maths/Transform.h"
#include "Scripting/Lua/LuaScriptComponent.h"
#include "ImGui/ImGuiHelpers.h"
#include "FileBrowserWindow.h"
#include "Physics/LumosPhysicsEngine/CuboidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/SphereCollisionShape.h"
#include "Physics/LumosPhysicsEngine/PyramidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CapsuleCollisionShape.h"

#include <imgui/imgui.h>
#include <IconFontCppHeaders/IconsMaterialDesignIcons.h>
#include <sol/sol.hpp>

namespace MM
{
	template<>
	void ComponentEditorWidget<Lumos::LuaScriptComponent>(entt::registry& reg, entt::registry::entity_type e)
	{
		auto& script = reg.get<Lumos::LuaScriptComponent>(e);

		if(!script.Loaded())
		{
			ImGui::Text("Script Failed to Load : %s", script.GetFilePath().c_str());
			return;
		}

		ImGui::TextUnformatted("Loaded Functions : ");

		auto& solEnv = script.GetSolEnvironment();

        ImGui::Indent(12.0f);

		for(auto&& function : solEnv)
		{
			if(function.second.is<sol::function>())
			{
				ImGui::TextUnformatted(function.first.as<std::string>().c_str());
			}
		}
    
        ImGui::Unindent(12.0f);

        ImGui::Indent(12.0f);

        if(ImGui::Button("Reload", ImVec2{ ImGui::GetWindowWidth() - 24.0f, 20.0f }))
			script.Reload();
    
        ImGui::Unindent(12.0f);


		std::string filePath = script.GetFilePath();

		static char filePathBuffer[INPUT_BUF_SIZE];
		strcpy(filePathBuffer, filePath.c_str());

		ImGui::PushItemWidth(-1);
		if(ImGui::InputText("##filePath", filePathBuffer, IM_ARRAYSIZE(filePathBuffer), 0))
			script.SetFilePath(filePathBuffer);

#ifdef LUMOS_EDITOR
		if(ImGui::Button("Edit File"))
			Lumos::Application::Get().GetEditor()->OpenTextFile(script.GetFilePath());
#endif
	}

	template<>
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
		if(ImGui::DragFloat3("##Position", Lumos::Maths::ValuePointer(position)))
		{
			transform.SetLocalPosition(position);
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Rotation");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::DragFloat3("##Rotation", Lumos::Maths::ValuePointer(rotation)))
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
		if(ImGui::DragFloat3("##Scale", Lumos::Maths::ValuePointer(scale), 0.1f))
		{
			transform.SetLocalScale(scale);
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar();
	}

	template<>
	void ComponentEditorWidget<Lumos::MeshComponent>(entt::registry& reg, entt::registry::entity_type e)
	{
		auto& meshComponent = reg.get<Lumos::MeshComponent>(e);
		ImGui::TextUnformatted(meshComponent.GetFilePath().c_str());
		auto primitiveType = meshComponent.GetPrimitiveType();
	}

	static void CuboidCollisionShapeInspector(Lumos::CuboidCollisionShape* shape, const Lumos::Physics3DComponent& phys)
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Half Dimensions");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		Lumos::Maths::Vector3 size = shape->GetHalfDimensions();
		if(ImGui::DragFloat3("##CollisionShapeHalfDims", Lumos::Maths::ValuePointer(size), 1.0f, 0.0f, 10000.0f))
		{
			shape->SetHalfDimensions(size);
			phys.GetRigidBody()->CollisionShapeUpdated();
		}
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
	}

	static void SphereCollisionShapeInspector(Lumos::SphereCollisionShape* shape, const Lumos::Physics3DComponent& phys)
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Radius");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		float radius = shape->GetRadius();
		if(ImGui::DragFloat("##CollisionShapeRadius", &radius, 1.0f, 0.0f, 10000.0f))
		{
			shape->SetRadius(radius);
			phys.GetRigidBody()->CollisionShapeUpdated();
		}
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
	}

	static void PyramidCollisionShapeInspector(Lumos::PyramidCollisionShape* shape, const Lumos::Physics3DComponent& phys)
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Half Dimensions");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		Lumos::Maths::Vector3 size = shape->GetHalfDimensions();
		if(ImGui::DragFloat3("##CollisionShapeHalfDims", Lumos::Maths::ValuePointer(size), 1.0f, 0.0f, 10000.0f))
		{
			shape->SetHalfDimensions(size);
			phys.GetRigidBody()->CollisionShapeUpdated();
		}
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
	}

	static void CapsuleCollisionShapeInspector(Lumos::CapsuleCollisionShape* shape, const Lumos::Physics3DComponent& phys)
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Half Dimensions");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		float radius = shape->GetRadius();
		if(ImGui::DragFloat("##CollisionShapeRadius", &radius, 1.0f, 0.0f, 10000.0f))
		{
			shape->SetRadius(radius);
			phys.GetRigidBody()->CollisionShapeUpdated();
		}
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
	}

	std::string CollisionShape2DTypeToString(Lumos::Shape shape)
	{
		switch (shape)
		{
			case Lumos::Shape::Circle : return "Circle";
			case Lumos::Shape::Square : return "Square";
			case Lumos::Shape::Custom : return "Custom";
		}

		return "Unknown Shape";
	}

	Lumos::Shape StringToCollisionShape2DType(const std::string& type)
	{
		if(type == "Circle")
			return Lumos::Shape::Circle;
		if(type == "Square")
			return Lumos::Shape::Square;
		if(type == "Custom")
			return Lumos::Shape::Custom;

		Lumos::Debug::Log::Error("Unsupported Collision shape {0}", type);
		return Lumos::Shape::Circle;
	}

	std::string CollisionShapeTypeToString(Lumos::CollisionShapeType type)
	{
		switch(type)
		{
		case Lumos::CollisionShapeType::CollisionCuboid:
			return "Cuboid";
		case Lumos::CollisionShapeType::CollisionSphere:
			return "Sphere";
		case Lumos::CollisionShapeType::CollisionPyramid:
			return "Pyramid";
		case Lumos::CollisionShapeType::CollisionCapsule:
			return "Capsule";
		default:
			Lumos::Debug::Log::Error("Unsupported Collision shape");
			break;
		}

		return "Error";
	}

	Lumos::CollisionShapeType StringToCollisionShapeType(const std::string& type)
	{
		if(type == "Sphere")
			return Lumos::CollisionShapeType::CollisionSphere;
		if(type == "Cuboid")
			return Lumos::CollisionShapeType::CollisionCuboid;
		if(type == "Pyramid")
			return Lumos::CollisionShapeType::CollisionPyramid;
		if(type == "Capsule")
			return Lumos::CollisionShapeType::CollisionCapsule;

		Lumos::Debug::Log::Error("Unsupported Collision shape {0}", type);
		return Lumos::CollisionShapeType::CollisionSphere;
	}

	template<>
	void ComponentEditorWidget<Lumos::Physics3DComponent>(entt::registry& reg, entt::registry::entity_type e)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();
		auto& phys = reg.get<Lumos::Physics3DComponent>(e);

		auto pos = phys.GetRigidBody()->GetPosition();
		auto force = phys.GetRigidBody()->GetForce();
		auto torque = phys.GetRigidBody()->GetTorque();
		auto orientation = phys.GetRigidBody()->GetOrientation();
		auto angularVelocity = phys.GetRigidBody()->GetAngularVelocity();
		auto friction = phys.GetRigidBody()->GetFriction();
		auto isStatic = phys.GetRigidBody()->GetIsStatic();
		auto isRest = phys.GetRigidBody()->GetIsAtRest();
		auto mass = 1.0f / phys.GetRigidBody()->GetInverseMass();
		auto velocity = phys.GetRigidBody()->GetLinearVelocity();
		auto elasticity = phys.GetRigidBody()->GetElasticity();

		auto collisionShape = phys.GetRigidBody()->GetCollisionShape();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Position");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::DragFloat3("##Position", Lumos::Maths::ValuePointer(pos)))
			phys.GetRigidBody()->SetPosition(pos);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Velocity");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::DragFloat3("##Velocity", Lumos::Maths::ValuePointer(velocity)))
			phys.GetRigidBody()->SetLinearVelocity(velocity);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Torque");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::DragFloat3("##Torque", Lumos::Maths::ValuePointer(torque)))
			phys.GetRigidBody()->SetTorque(torque);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Orientation");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::DragFloat4("##Orientation", Lumos::Maths::ValuePointer(orientation)))
			phys.GetRigidBody()->SetOrientation(orientation);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Force");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::DragFloat4("##Force", Lumos::Maths::ValuePointer(force)))
			phys.GetRigidBody()->SetForce(force);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Angular Velocity");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::DragFloat3("##Angular Velocity", Lumos::Maths::ValuePointer(angularVelocity)))
			phys.GetRigidBody()->SetAngularVelocity(angularVelocity);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Friction");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::DragFloat("##Friction", &friction))
			phys.GetRigidBody()->SetFriction(friction);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Mass");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::DragFloat("##Mass", &mass))
			phys.GetRigidBody()->SetInverseMass(1.0f / mass);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Elasticity");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::DragFloat("##Elasticity", &elasticity))
			phys.GetRigidBody()->SetElasticity(elasticity);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Static");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::Checkbox("##Static", &isStatic))
			phys.GetRigidBody()->SetIsStatic(isStatic);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("At Rest");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::Checkbox("##At Rest", &isRest))
			phys.GetRigidBody()->SetIsAtRest(isRest);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar();

		ImGui::Separator();
		ImGui::TextUnformatted("Collision Shape");

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Shape Type");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		const char* shapes[] = {"Sphere", "Cuboid", "Pyramid", "Capsule"};
		std::string shape_current = collisionShape ? CollisionShapeTypeToString(collisionShape->GetType()) : "";
		if(ImGui::BeginCombo("", shape_current.c_str(), 0)) // The second parameter is the label previewed before opening the combo.
		{
			for(int n = 0; n < 4; n++)
			{
				bool is_selected = (shape_current.c_str() == shapes[n]);
				if(ImGui::Selectable(shapes[n], shape_current.c_str()))
				{
					phys.GetRigidBody()->SetCollisionShape(StringToCollisionShapeType(shapes[n]));
				}
				if(is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		if(collisionShape)
		{
			switch(collisionShape->GetType())
			{
			case Lumos::CollisionShapeType::CollisionCuboid:
				CuboidCollisionShapeInspector(reinterpret_cast<Lumos::CuboidCollisionShape*>(collisionShape.get()), phys);
				break;
			case Lumos::CollisionShapeType::CollisionSphere:
				SphereCollisionShapeInspector(reinterpret_cast<Lumos::SphereCollisionShape*>(collisionShape.get()), phys);
				break;
			case Lumos::CollisionShapeType::CollisionPyramid:
				PyramidCollisionShapeInspector(reinterpret_cast<Lumos::PyramidCollisionShape*>(collisionShape.get()), phys);
				break;
			case Lumos::CollisionShapeType::CollisionCapsule:
				CapsuleCollisionShapeInspector(reinterpret_cast<Lumos::CapsuleCollisionShape*>(collisionShape.get()), phys);
				break;
			default:
				Lumos::Debug::Log::Error("Unsupported Collision shape");
				break;
			}
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar();
	}

	template<>
	void ComponentEditorWidget<Lumos::Physics2DComponent>(entt::registry& reg, entt::registry::entity_type e)
	{
		auto& phys = reg.get<Lumos::Physics2DComponent>(e);

		auto pos = phys.GetRigidBody()->GetPosition();
		auto angle = phys.GetRigidBody()->GetAngle();
		auto friction = phys.GetRigidBody()->GetFriction();
		auto isStatic = phys.GetRigidBody()->GetIsStatic();
		auto isRest = phys.GetRigidBody()->GetIsAtRest();

		auto elasticity = phys.GetRigidBody()->GetElasticity();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Position");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::DragFloat2("##Position", &pos.x))
			phys.GetRigidBody()->SetPosition(pos);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Orientation");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::DragFloat("##Orientation", &angle))
			phys.GetRigidBody()->SetOrientation(angle);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Friction");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::DragFloat("##Friction", &friction))
			phys.GetRigidBody()->SetFriction(friction);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Elasticity");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::DragFloat("##Elasticity", &elasticity))
			phys.GetRigidBody()->SetElasticity(elasticity);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Static");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::Checkbox("##Static", &isStatic))
			phys.GetRigidBody()->SetIsStatic(isStatic);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("At Rest");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::Checkbox("##At Rest", &isRest))
			phys.GetRigidBody()->SetIsAtRest(isRest);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Shape Type");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		const char* shapes[] = {"Circle", "Square", "Custom"};
		std::string shape_current = CollisionShape2DTypeToString(phys.GetRigidBody()->GetShapeType());
		if(ImGui::BeginCombo("", shape_current.c_str(), 0)) // The second parameter is the label previewed before opening the combo.
		{
			for(int n = 0; n < 3; n++)
			{
				bool is_selected = (shape_current.c_str() == shapes[n]);
				if(ImGui::Selectable(shapes[n], shape_current.c_str()))
				{
					phys.GetRigidBody()->SetShape(StringToCollisionShape2DType(shapes[n]));
				}
				if(is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar();
	}

	template<>
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
		if(ImGui::InputFloat3("##Position", Lumos::Maths::ValuePointer(pos)))
			sound.GetSoundNode()->SetPosition(pos);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Radius");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::InputFloat("##Radius", &radius))
			sound.GetSoundNode()->SetRadius(radius);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Pitch");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::InputFloat("##Pitch", &pitch))
			sound.GetSoundNode()->SetPitch(pitch);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Reference Distance");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::DragFloat("##Reference Distance", &referenceDistance))
			sound.GetSoundNode()->SetReferenceDistance(referenceDistance);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Paused");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::Checkbox("##Paused", &paused))
			sound.GetSoundNode()->SetPaused(paused);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar();
	}

	template<>
	void ComponentEditorWidget<Lumos::Camera>(entt::registry& reg, entt::registry::entity_type e)
	{
		auto& camera = reg.get<Lumos::Camera>(e);
		camera.OnImGui();
	}

	template<>
	void ComponentEditorWidget<Lumos::Graphics::Sprite>(entt::registry& reg, entt::registry::entity_type e)
	{
		auto& sprite = reg.get<Lumos::Graphics::Sprite>(e);

		sprite.OnImGui();
	}

	template<>
	void ComponentEditorWidget<Lumos::Graphics::Light>(entt::registry& reg, entt::registry::entity_type e)
	{
		auto& light = reg.get<Lumos::Graphics::Light>(e);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();

		if(light.Type != 0)
			Lumos::ImGuiHelpers::Property("Position", light.Position);

		if(light.Type != 2)
			Lumos::ImGuiHelpers::Property("Direction", light.Direction);

		if(light.Type != 0)
			Lumos::ImGuiHelpers::Property("Radius", light.Radius, 0.0f, 100.0f);
		Lumos::ImGuiHelpers::Property("Colour", light.Colour, true, Lumos::ImGuiHelpers::PropertyFlag::ColorProperty);
		Lumos::ImGuiHelpers::Property("Intensity", light.Intensity, 0.0f, 4.0f);

		if(light.Type == 1)
			Lumos::ImGuiHelpers::Property("Angle", light.Angle, -1.0f, 1.0f);

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Light Type");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		const char* types[] = {"Directional", "Spot", "Point"};
		std::string light_current = Lumos::Graphics::Light::LightTypeToString(Lumos::Graphics::LightType(int(light.Type)));
		if(ImGui::BeginCombo("", light_current.c_str(), 0)) // The second parameter is the label previewed before opening the combo.
		{
			for(int n = 0; n < 3; n++)
			{
				bool is_selected = (light_current.c_str() == types[n]);
				if(ImGui::Selectable(types[n], light_current.c_str()))
				{
					light.Type = Lumos::Graphics::Light::StringToLightType(types[n]);
				}
				if(is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar();
	}

	template<>
	void ComponentEditorWidget<Lumos::MaterialComponent>(entt::registry& reg, entt::registry::entity_type e)
	{
		using namespace Lumos;
		auto& materialComponent = reg.get<Lumos::MaterialComponent>(e);
		auto material = materialComponent.GetMaterial();
		bool flipImage = Graphics::GraphicsContext::GetContext()->FlipImGUITexture();

		MaterialProperties* prop = material->GetProperties();

		if(ImGui::TreeNodeEx("Albedo", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			auto tex = material->GetTextures().albedo;

			if(tex)
			{
				if(ImGui::ImageButton(tex->GetHandle(), ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
				{
#ifdef LUMOS_EDITOR
					Application::Get().GetEditor()->GetFileBrowserWindow().Open();
					Application::Get().GetEditor()->GetFileBrowserWindow().SetCallback(std::bind(&MaterialComponent::SetAlbedoTexture, &materialComponent, std::placeholders::_1));
#endif
				}

				if(ImGui::IsItemHovered() && tex)
				{
					ImGui::BeginTooltip();
					ImGui::Image(tex->GetHandle(), ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
					ImGui::EndTooltip();
				}
			}
			else
			{
				if(ImGui::Button("Empty", ImVec2(64, 64)))
				{
#ifdef LUMOS_EDITOR
					Application::Get().GetEditor()->GetFileBrowserWindow().Open();
					Application::Get().GetEditor()->GetFileBrowserWindow().SetCallback(std::bind(&MaterialComponent::SetAlbedoTexture, &materialComponent, std::placeholders::_1));
#endif
				}
			}

			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::TextUnformatted(tex ? tex->GetFilepath().c_str() : "No Texture");

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGuiHelpers::Property("Use Albedo Map", prop->usingAlbedoMap, 0.0f, 1.0f);
			ImGuiHelpers::Property("Albedo", prop->albedoColour, 0.0f, 1.0f, false, Lumos::ImGuiHelpers::PropertyFlag::ColorProperty);

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();

			ImGui::TreePop();
		}

		ImGui::Separator();

		if(ImGui::TreeNode("Normal"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			auto tex = material->GetTextures().normal;

			if(tex)
			{
				if(ImGui::ImageButton(tex->GetHandle(), ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
				{
#ifdef LUMOS_EDITOR
					Application::Get().GetEditor()->GetFileBrowserWindow().Open();
					Application::Get().GetEditor()->GetFileBrowserWindow().SetCallback(std::bind(&MaterialComponent::SetNormalTexture, &materialComponent, std::placeholders::_1));
#endif
				}

				if(ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::Image(tex->GetHandle(), ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
					ImGui::EndTooltip();
				}
			}
			else
			{
				if(ImGui::Button("Empty", ImVec2(64, 64)))
				{
#ifdef LUMOS_EDITOR
					Application::Get().GetEditor()->GetFileBrowserWindow().Open();
					Application::Get().GetEditor()->GetFileBrowserWindow().SetCallback(std::bind(&MaterialComponent::SetNormalTexture, &materialComponent, std::placeholders::_1));
#endif
				}
			}

			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::TextUnformatted(tex ? tex->GetFilepath().c_str() : "No Texture");

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGuiHelpers::Property("Use Normal Map", prop->usingNormalMap, 0.0f, 1.0f);

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();
			ImGui::TreePop();
		}

		ImGui::Separator();

		if(ImGui::TreeNode("Metallic"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			auto tex = material->GetTextures().metallic;

			if(tex)
			{
				if(ImGui::ImageButton(tex->GetHandle(), ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
				{
#ifdef LUMOS_EDITOR
					Application::Get().GetEditor()->GetFileBrowserWindow().Open();
					Application::Get().GetEditor()->GetFileBrowserWindow().SetCallback(std::bind(&MaterialComponent::SetMetallicTexture, &materialComponent, std::placeholders::_1));
#endif
				}

				if(ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::Image(tex->GetHandle(), ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
					ImGui::EndTooltip();
				}
			}
			else
			{
				if(ImGui::Button("Empty", ImVec2(64, 64)))
				{
#ifdef LUMOS_EDITOR
					Application::Get().GetEditor()->GetFileBrowserWindow().Open();
					Application::Get().GetEditor()->GetFileBrowserWindow().SetCallback(std::bind(&MaterialComponent::SetMetallicTexture, &materialComponent, std::placeholders::_1));
#endif
				}
			}

			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::TextUnformatted(tex ? tex->GetFilepath().c_str() : "No Texture");

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGuiHelpers::Property("Use Metallic Map", prop->usingMetallicMap, 0.0f, 1.0f);
			ImGuiHelpers::Property("Metallic", prop->metallicColour, 0.0f, 1.0f, false);

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();
			ImGui::TreePop();
		}

		ImGui::Separator();

		if(ImGui::TreeNode("Roughness"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			auto tex = material->GetTextures().roughness;
			if(tex)
			{
				if(ImGui::ImageButton(tex->GetHandle(), ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
				{
#ifdef LUMOS_EDITOR
					Application::Get().GetEditor()->GetFileBrowserWindow().Open();
					Application::Get().GetEditor()->GetFileBrowserWindow().SetCallback(std::bind(&MaterialComponent::SetRoughnessTexture, &materialComponent, std::placeholders::_1));
#endif
				}

				if(ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
					ImGui::EndTooltip();
				}
			}
			else
			{
				if(ImGui::Button("Empty", ImVec2(64, 64)))
				{
#ifdef LUMOS_EDITOR
					Application::Get().GetEditor()->GetFileBrowserWindow().Open();
					Application::Get().GetEditor()->GetFileBrowserWindow().SetCallback(std::bind(&MaterialComponent::SetRoughnessTexture, &materialComponent, std::placeholders::_1));
#endif
				}
			}

			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::TextUnformatted(tex ? tex->GetFilepath().c_str() : "No Texture");

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGuiHelpers::Property("Use Roughness Map", prop->usingRoughnessMap, 0.0f, 1.0f);
			ImGuiHelpers::Property("Roughness", prop->roughnessColour, 0.0f, 1.0f, false);

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();
			ImGui::TreePop();
		}

		ImGui::Separator();

		if(ImGui::TreeNode("Ao"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			auto tex = material->GetTextures().ao;
			if(tex)
			{
				if(ImGui::ImageButton(tex->GetHandle(), ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
				{
#ifdef LUMOS_EDITOR
					Application::Get().GetEditor()->GetFileBrowserWindow().Open();
					Application::Get().GetEditor()->GetFileBrowserWindow().SetCallback(std::bind(&MaterialComponent::SetAOTexture, &materialComponent, std::placeholders::_1));
#endif
				}

				if(ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::Image(tex->GetHandle(), ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
					ImGui::EndTooltip();
				}
			}
			else
			{
				if(ImGui::Button("Empty", ImVec2(64, 64)))
				{
#ifdef LUMOS_EDITOR
					Application::Get().GetEditor()->GetFileBrowserWindow().Open();
					Application::Get().GetEditor()->GetFileBrowserWindow().SetCallback(std::bind(&MaterialComponent::SetAOTexture, &materialComponent, std::placeholders::_1));
#endif
				}
			}

			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::TextUnformatted(tex ? tex->GetFilepath().c_str() : "No Texture");
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGuiHelpers::Property("Use AO Map", prop->usingAOMap, 0.0f, 1.0f);

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();
			ImGui::TreePop();
		}

		ImGui::Separator();

		if(ImGui::TreeNode("Emissive"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			auto tex = material->GetTextures().emissive;
			if(tex)
			{
				if(ImGui::ImageButton(tex->GetHandle(), ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
				{
#ifdef LUMOS_EDITOR
					Application::Get().GetEditor()->GetFileBrowserWindow().Open();
					Application::Get().GetEditor()->GetFileBrowserWindow().SetCallback(std::bind(&MaterialComponent::SetEmissiveTexture, &materialComponent, std::placeholders::_1));
#endif
				}

				if(ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::Image(tex->GetHandle(), ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
					ImGui::EndTooltip();
				}
			}
			else
			{
				if(ImGui::Button("Empty", ImVec2(64, 64)))
				{
#ifdef LUMOS_EDITOR
					Application::Get().GetEditor()->GetFileBrowserWindow().Open();
					Application::Get().GetEditor()->GetFileBrowserWindow().SetCallback(std::bind(&MaterialComponent::SetEmissiveTexture, &materialComponent, std::placeholders::_1));
#endif
				}
			}

			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::TextUnformatted(tex ? tex->GetFilepath().c_str() : "No Texture");

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Use Emissive Map");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::SliderFloat("##UseEmissiveMap", &prop->usingEmissiveMap, 0.0f, 1.0f);

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Emissive");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::SliderFloat3("##Emissive", Maths::ValuePointer(prop->emissiveColour), 0.0f, 1.0f);

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();
			ImGui::TreePop();
		}

		ImGui::Separator();

		material->SetMaterialProperites(*prop);
	}

	template<>
	void ComponentEditorWidget<Lumos::Graphics::Environment>(entt::registry& reg, entt::registry::entity_type e)
	{
		auto& environment = reg.get<Lumos::Graphics::Environment>(e);
		Lumos::ImGuiHelpers::Image(environment.GetEnvironmentMap(), Lumos::Maths::Vector2(200, 200));
	}

	template<>
	void ComponentEditorWidget<Lumos::TextureMatrixComponent>(entt::registry& reg, entt::registry::entity_type e)
	{
		auto& textureMatrix = reg.get<Lumos::TextureMatrixComponent>(e);
		Lumos::Maths::Matrix4& mat = textureMatrix.GetMatrix();
		auto rotation = textureMatrix.GetMatrix().Rotation();
		auto position = textureMatrix.GetMatrix().Translation();
		auto scale = textureMatrix.GetMatrix().Scale();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Position");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::DragFloat3("##Position", Lumos::Maths::ValuePointer(position)))
		{
			mat.SetTranslation(position);
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Rotation");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::DragFloat3("##Rotation", Lumos::Maths::ValuePointer(rotation)))
		{
			float pitch = Lumos::Maths::Min(rotation.x, 89.9f);
			pitch = Lumos::Maths::Max(pitch, -89.9f);
			mat.SetRotation(Lumos::Maths::Quaternion::EulerAnglesToQuaternion(pitch, rotation.y, rotation.z).RotationMatrix());
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Scale");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if(ImGui::DragFloat3("##Scale", Lumos::Maths::ValuePointer(scale), 0.1f))
		{
			mat.SetScale(scale);
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar();
	}

	template<>
	void ComponentEditorWidget<Lumos::DefaultCameraController>(entt::registry& reg, entt::registry::entity_type e)
	{
		auto& controllerComp = reg.get<Lumos::DefaultCameraController>(e);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Controller Type");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		const char* controllerTypes[] = {"Editor", "FPS", "ThirdPerson", "2D", "Custom"};
		std::string currentController = Lumos::DefaultCameraController::CameraControllerTypeToString(controllerComp.GetType());
		if(ImGui::BeginCombo("", currentController.c_str(), 0)) // The second parameter is the label previewed before opening the combo.
		{
			for(int n = 0; n < 5; n++)
			{
				bool is_selected = (currentController.c_str() == controllerTypes[n]);
				if(ImGui::Selectable(controllerTypes[n], currentController.c_str()))
				{
					controllerComp.SetControllerType(Lumos::DefaultCameraController::StringToControllerType(controllerTypes[n]));
				}
				if(is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

        if(controllerComp.GetController())
            controllerComp.GetController()->OnImGui();

		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar();
	}
}

namespace Lumos
{
	InspectorWindow::InspectorWindow()
	{
		m_Name = ICON_MDI_INFORMATION " Inspector###inspector";
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
		std::string Name; \
		if(iconMap.find(typeid(ComponentType).hash_code()) != iconMap.end()) \
			Name += iconMap[typeid(ComponentType).hash_code()]; \
		Name += "\t"; \
		Name += (ComponentName); \
		m_EnttEditor.registerComponent<ComponentType>(Name.c_str()); \
	}
		TRIVIAL_COMPONENT(Maths::Transform, "Transform");
		TRIVIAL_COMPONENT(MeshComponent, "Mesh");
		TRIVIAL_COMPONENT(Camera, "Camera");
		TRIVIAL_COMPONENT(Physics3DComponent, "Physics3D");
		TRIVIAL_COMPONENT(Physics2DComponent, "Physics2D");
		TRIVIAL_COMPONENT(SoundComponent, "Sound");
		TRIVIAL_COMPONENT(Graphics::Sprite, "Sprite");
		TRIVIAL_COMPONENT(MaterialComponent, "Material");
		TRIVIAL_COMPONENT(Graphics::Light, "Light");
		TRIVIAL_COMPONENT(LuaScriptComponent, "LuaScript");
		TRIVIAL_COMPONENT(Graphics::Environment, "Environment");
		TRIVIAL_COMPONENT(TextureMatrixComponent, "Texture Matrix");
		TRIVIAL_COMPONENT(DefaultCameraController, "Default Camera Controller");
	}

	void InspectorWindow::OnImGui()
	{
		auto& registry = Application::Get().GetSceneManager()->GetCurrentScene()->GetRegistry();
		auto selected = m_Editor->GetSelected();

		if(ImGui::Begin(m_Name.c_str(), &m_Active))
		{
			if(selected == entt::null)
			{
				ImGui::End();
				return;
			}

			//active checkbox
			auto activeComponent = registry.try_get<ActiveComponent>(selected);
			bool active = activeComponent ? activeComponent->active : true;
			if(ImGui::Checkbox("##ActiveCheckbox", &active))
			{
				if(!activeComponent)
					registry.emplace<ActiveComponent>(selected, active);
				else
					activeComponent->active = active;
			}
			ImGui::SameLine();
			ImGui::TextUnformatted(ICON_MDI_CUBE);
			ImGui::SameLine();

			bool hasName = registry.has<NameComponent>(selected);
			std::string name;
			if(hasName)
				name = registry.get<NameComponent>(selected).name;
			else
				name = StringFormat::ToString(entt::to_integral(selected));

			static char objName[INPUT_BUF_SIZE];
			strcpy(objName, name.c_str());

			if(m_DebugMode)
			{
				if(registry.valid(selected))
				{
					ImGui::Text("ID: %d, Version: %d", static_cast<int>(registry.entity(selected)), registry.version(selected));
				}
				else
				{
					ImGui::TextUnformatted("INVALID ENTITY");
				}
			}

            ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - 16.0f);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));

            if (ImGui::Button(ICON_MDI_TUNE))
                ImGui::OpenPopup("SetDebugMode");
            ImGui::PopStyleColor();

            if (ImGui::BeginPopup("SetDebugMode", 3))
            {
                if (ImGui::Selectable("Debug Mode", m_DebugMode))
                {
                    m_DebugMode = !m_DebugMode;
                }
                ImGui::EndPopup();
            }

			ImGui::PushItemWidth(-1);
			if(ImGui::InputText("##Name", objName, IM_ARRAYSIZE(objName), 0))
				registry.get_or_emplace<NameComponent>(selected).name = objName;

			ImGui::Separator();

			if(m_DebugMode)
            {
                auto hierarchyComp = registry.try_get<Hierarchy>(selected);

                if(hierarchyComp)
                {
                    if(registry.valid(hierarchyComp->parent()))
                    {
                        ImGui::Text("Parent : ID: %d", static_cast<int>(registry.entity(hierarchyComp->parent())));
                    }
                    else
                    {
                        ImGui::TextUnformatted("Parent : null");
                    }

                    entt::entity child = hierarchyComp->first();
                    ImGui::TextUnformatted("Children : ");
                    ImGui::Indent(24.0f);

                    while (child != entt::null)
                    {
                        ImGui::Text("ID: %d", static_cast<int>(registry.entity(child)));

                        auto hierarchy = registry.try_get<Hierarchy>(child);

                        if (hierarchy)
                        {
                            child = hierarchy->next();
                        }
                    }

                    ImGui::Unindent(24.0f);

                }


                ImGui::Separator();
            }


			m_EnttEditor.RenderImGui(registry, selected);
		}
		ImGui::End();
	}

    void InspectorWindow::SetDebugMode(bool mode)
    {
		m_DebugMode = mode;
    }
}
