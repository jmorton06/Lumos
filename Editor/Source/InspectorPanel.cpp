#include "InspectorPanel.h"
#include "Editor.h"
#include "FileBrowserPanel.h"
#include "TextEditPanel.h"
#include <Lumos/Audio/AudioManager.h>
#include <Lumos/Core/Application.h>
#include <Lumos/Core/OS/FileSystem.h>
#include <Lumos/Scene/SceneManager.h>
#include <Lumos/Scene/Component/Components.h>
#include <Lumos/Scene/Component/ModelComponent.h>
#include <Lumos/Graphics/Camera/Camera.h>
#include <Lumos/Graphics/Sprite.h>
#include <Lumos/Graphics/AnimatedSprite.h>
#include <Lumos/Graphics/Model.h>
#include <Lumos/Graphics/Mesh.h>
#include <Lumos/Graphics/MeshFactory.h>
#include <Lumos/Graphics/Light.h>
#include <Lumos/Graphics/Material.h>
#include <Lumos/Graphics/Environment.h>
#include <Lumos/Graphics/RHI/Texture.h>
#include <Lumos/Graphics/RHI/Renderer.h>
#include <Lumos/Graphics/RHI/GraphicsContext.h>
#include <Lumos/Maths/Transform.h>
#include <Lumos/Scripting/Lua/LuaScriptComponent.h>
#include <Lumos/ImGui/ImGuiHelpers.h>
#include <Lumos/Physics/LumosPhysicsEngine/CuboidCollisionShape.h>
#include <Lumos/Physics/LumosPhysicsEngine/SphereCollisionShape.h>
#include <Lumos/Physics/LumosPhysicsEngine/PyramidCollisionShape.h>
#include <Lumos/Physics/LumosPhysicsEngine/CapsuleCollisionShape.h>
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <sol/sol.hpp>
#include <inttypes.h>

namespace MM
{
    template <>
    void ComponentEditorWidget<Lumos::LuaScriptComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& script = reg.get<Lumos::LuaScriptComponent>(e);
        bool loaded = false;
        if(!script.Loaded())
        {
            ImGui::Text("Script Failed to Load : %s", script.GetFilePath().c_str());
            loaded = false;
        }
        else if(!script.Loaded() && script.GetFilePath().empty())
        {
            ImGui::Text("FilePath empty : %s", script.GetFilePath().c_str());
            loaded = false;
        }
        else
            loaded = true;

        auto& solEnv = script.GetSolEnvironment();
        std::string filePath = script.GetFilePath();

        static char objName[INPUT_BUF_SIZE];
        strcpy(objName, filePath.c_str());
        ImGui::PushItemWidth(-1);
        if(ImGui::InputText("##Name", objName, IM_ARRAYSIZE(objName), 0))
            script.SetFilePath(objName);

        bool hasReloaded = false;

        if(ImGui::Button("New File", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
        {
            std::string newFilePath = "//Scripts";
            std::string physicalPath;
            if(!Lumos::VFS::Get().ResolvePhysicalPath(newFilePath, physicalPath, true))
            {
                LUMOS_LOG_ERROR("Failed to Create Lua script {0}", physicalPath);
            }
            else
            {
                std::string defaultScript =
                    R"(--Default Lua Script
                
function OnInit()
end

function OnUpdate(dt)
end

function OnCleanUp()
end
)";
                std::string newScriptFileName = "Script";
                int fileIndex = 0;
                while(Lumos::FileSystem::FileExists(physicalPath + "/" + newScriptFileName + ".lua"))
                {
                    fileIndex++;
                    newScriptFileName = fmt::format("Script({0})", fileIndex);
                }

                Lumos::FileSystem::WriteTextFile(physicalPath + "/" + newScriptFileName + ".lua", defaultScript);
                script.SetFilePath(newFilePath + "/" + newScriptFileName + ".lua");
                script.Reload();
                hasReloaded = true;
            }
        }

        if(loaded)
        {
            if(ImGui::Button("Edit File", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
            {
                Lumos::Editor::GetEditor()->OpenTextFile(script.GetFilePath(), [&]
                    {
                        script.Reload();
                        hasReloaded = true;

                        auto textEditPanel = Lumos::Editor::GetEditor()->GetTextEditPanel();
                        if(textEditPanel)
                            ((Lumos::TextEditPanel*)textEditPanel)->SetErrors(script.GetErrors());
                    });

                auto textEditPanel = Lumos::Editor::GetEditor()->GetTextEditPanel();
                if(textEditPanel)
                    ((Lumos::TextEditPanel*)textEditPanel)->SetErrors(script.GetErrors());
            }

            if(ImGui::Button("Open File", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
            {
                Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(std::bind(&Lumos::LuaScriptComponent::LoadScript, &script, std::placeholders::_1));
            }

            if(ImGui::Button("Reload", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
            {
                script.Reload();
                hasReloaded = true;
            }
        }
        else
        {
            if(ImGui::Button("Load", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
            {
                script.Reload();
                hasReloaded = true;
            }
        }

        if(!script.Loaded() || hasReloaded || !loaded)
        {
            return;
        }

        ImGui::TextUnformatted("Loaded Functions : ");

        ImGui::Indent();
        for(auto&& function : solEnv)
        {
            if(function.second.is<sol::function>())
            {
                ImGui::TextUnformatted(function.first.as<std::string>().c_str());
            }
        }
        ImGui::Unindent();
    }

    template <>
    void ComponentEditorWidget<Lumos::Maths::Transform>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& transform = reg.get<Lumos::Maths::Transform>(e);

        auto rotation = transform.GetLocalOrientation().EulerAngles();
        auto position = transform.GetLocalPosition();
        auto scale = transform.GetLocalScale();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::TextUnformatted("Position");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        if(ImGui::DragFloat3("##Position", Lumos::Maths::ValuePointer(position), 3, 0.05f))
        {
            transform.SetLocalPosition(position);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::TextUnformatted("Rotation");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Rotation", Lumos::Maths::ValuePointer(rotation), 3, 0.05f))
        {
            float pitch = Lumos::Maths::Min(rotation.x, 89.9f);
            pitch = Lumos::Maths::Max(pitch, -89.9f);
            transform.SetLocalOrientation(Lumos::Maths::Quaternion::EulerAnglesToQuaternion(pitch, rotation.y, rotation.z));
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::TextUnformatted("Scale");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Scale", Lumos::Maths::ValuePointer(scale), 3, 0.05f))
        {
            transform.SetLocalScale(scale);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    static void CuboidCollisionShapeInspector(Lumos::CuboidCollisionShape* shape, const Lumos::Physics3DComponent& phys)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Half Dimensions");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        Lumos::Maths::Vector3 size = shape->GetHalfDimensions();
        if(ImGui::DragFloat3("##CollisionShapeHalfDims", Lumos::Maths::ValuePointer(size), 1.0f, 0.0f, 10000.0f, "%.2f"))
        {
            shape->SetHalfDimensions(size);
            phys.GetRigidBody()->CollisionShapeUpdated();
        }
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
    }

    static void SphereCollisionShapeInspector(Lumos::SphereCollisionShape* shape, const Lumos::Physics3DComponent& phys)
    {
        LUMOS_PROFILE_FUNCTION();
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
        LUMOS_PROFILE_FUNCTION();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Half Dimensions");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        Lumos::Maths::Vector3 size = shape->GetHalfDimensions();
        if(ImGui::DragFloat3("##CollisionShapeHalfDims", Lumos::Maths::ValuePointer(size), 1.0f, 0.0f, 10000.0f, "%.2f"))
        {
            shape->SetHalfDimensions(size);
            phys.GetRigidBody()->CollisionShapeUpdated();
        }
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
    }

    static void CapsuleCollisionShapeInspector(Lumos::CapsuleCollisionShape* shape, const Lumos::Physics3DComponent& phys)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Half Dimensions");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        float radius = shape->GetRadius();
        if(ImGui::DragFloat("##CollisionShapeRadius", &radius, 1.0f, 0.0f, 10000.0f, "%.2f"))
        {
            shape->SetRadius(radius);
            phys.GetRigidBody()->CollisionShapeUpdated();
        }
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
    }

    static void HullCollisionShapeInspector(Lumos::HullCollisionShape* shape, const Lumos::Physics3DComponent& phys)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::TextUnformatted("Hull Collision Shape");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
    }

    std::string CollisionShape2DTypeToString(Lumos::Shape shape)
    {
        LUMOS_PROFILE_FUNCTION();
        switch(shape)
        {
        case Lumos::Shape::Circle:
            return "Circle";
        case Lumos::Shape::Square:
            return "Square";
        case Lumos::Shape::Custom:
            return "Custom";
        }

        return "Unknown Shape";
    }

    Lumos::Shape StringToCollisionShape2DType(const std::string& type)
    {
        LUMOS_PROFILE_FUNCTION();
        if(type == "Circle")
            return Lumos::Shape::Circle;
        if(type == "Square")
            return Lumos::Shape::Square;
        if(type == "Custom")
            return Lumos::Shape::Custom;

        LUMOS_LOG_ERROR("Unsupported Collision shape {0}", type);
        return Lumos::Shape::Circle;
    }

    std::string CollisionShapeTypeToString(Lumos::CollisionShapeType type)
    {
        LUMOS_PROFILE_FUNCTION();
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
        case Lumos::CollisionShapeType::CollisionHull:
            return "Hull";
        default:
            LUMOS_LOG_ERROR("Unsupported Collision shape");
            break;
        }

        return "Error";
    }

    Lumos::CollisionShapeType StringToCollisionShapeType(const std::string& type)
    {
        LUMOS_PROFILE_FUNCTION();
        if(type == "Sphere")
            return Lumos::CollisionShapeType::CollisionSphere;
        if(type == "Cuboid")
            return Lumos::CollisionShapeType::CollisionCuboid;
        if(type == "Pyramid")
            return Lumos::CollisionShapeType::CollisionPyramid;
        if(type == "Capsule")
            return Lumos::CollisionShapeType::CollisionCapsule;
        if(type == "Hull")
            return Lumos::CollisionShapeType::CollisionHull;
        LUMOS_LOG_ERROR("Unsupported Collision shape {0}", type);
        return Lumos::CollisionShapeType::CollisionSphere;
    }

    template <>
    void ComponentEditorWidget<Lumos::Physics3DComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();
        auto& phys = reg.get<Lumos::Physics3DComponent>(e);

        auto pos = phys.GetRigidBody()->GetPosition();
        auto force = phys.GetRigidBody()->GetForce();
        auto torque = phys.GetRigidBody()->GetTorque();
        auto orientation = phys.GetRigidBody()->GetOrientation().EulerAngles();
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
        if(ImGui::DragFloat3("##Position", Lumos::Maths::ValuePointer(pos), 1.0f, 0.0f, 0.0f, "%.2f"))
            phys.GetRigidBody()->SetPosition(pos);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Velocity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Velocity", Lumos::Maths::ValuePointer(velocity), 1.0f, 0.0f, 0.0f, "%.2f"))
            phys.GetRigidBody()->SetLinearVelocity(velocity);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Torque");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Torque", Lumos::Maths::ValuePointer(torque), 1.0f, 0.0f, 0.0f, "%.2f"))
            phys.GetRigidBody()->SetTorque(torque);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Orientation");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Orientation", Lumos::Maths::ValuePointer(orientation), 1.0f, 0.0f, 0.0f, "%.2f"))
            phys.GetRigidBody()->SetOrientation(Lumos::Maths::Quaternion(orientation));

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Force");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Force", Lumos::Maths::ValuePointer(force), 1.0f, 0.0f, 0.0f, "%.2f"))
            phys.GetRigidBody()->SetForce(force);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Angular Velocity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Angular Velocity", Lumos::Maths::ValuePointer(angularVelocity), 1.0f, 0.0f, 0.0f, "%.2f"))
            phys.GetRigidBody()->SetAngularVelocity(angularVelocity);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Friction");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Friction", &friction, 1.0f, 0.0f, 0.0f, "%.2f"))
            phys.GetRigidBody()->SetFriction(friction);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Mass");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Mass", &mass, 1.0f, 0.0f, 0.0f, "%.2f"))
            phys.GetRigidBody()->SetInverseMass(1.0f / mass);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Elasticity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Elasticity", &elasticity, 1.0f, 0.0f, 0.0f, "%.2f"))
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

        const char* shapes[] = { "Sphere", "Cuboid", "Pyramid", "Capsule", "Hull" };
        std::string shape_current = collisionShape ? CollisionShapeTypeToString(collisionShape->GetType()) : "";
        if(ImGui::BeginCombo("", shape_current.c_str(), 0)) // The second parameter is the label previewed before opening the combo.
        {
            for(int n = 0; n < 5; n++)
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
            case Lumos::CollisionShapeType::CollisionHull:
                HullCollisionShapeInspector(reinterpret_cast<Lumos::HullCollisionShape*>(collisionShape.get()), phys);
                break;
            default:
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                LUMOS_LOG_ERROR("Unsupported Collision shape");
                break;
            }
        }
        else
        {
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
        }

        ImGui::PopItemWidth();
        ImGui::Columns(1);

        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    template <>
    void ComponentEditorWidget<Lumos::Physics2DComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
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

        const char* shapes[] = { "Circle", "Square", "Custom" };
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

    template <>
    void ComponentEditorWidget<Lumos::SoundComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& sound = reg.get<Lumos::SoundComponent>(e);
        auto soundNode = sound.GetSoundNode();

        bool updated = false;

        auto pos = soundNode->GetPosition();
        auto radius = soundNode->GetRadius();
        auto paused = soundNode->GetPaused();
        auto pitch = soundNode->GetPitch();
        auto volume = soundNode->GetVolume();
        auto referenceDistance = soundNode->GetReferenceDistance();
        auto rollOffFactor = soundNode->GetRollOffFactor();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Position");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::InputFloat3("##Position", Lumos::Maths::ValuePointer(pos)))
        {
            soundNode->SetPosition(pos);
            updated = true;
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Radius");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::InputFloat("##Radius", &radius))
        {
            soundNode->SetRadius(radius);
            updated = true;
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Pitch");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::InputFloat("##Pitch", &pitch))
        {
            soundNode->SetPitch(pitch);
            updated = true;
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Volume");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::InputFloat("##Volume", &volume))
        {
            soundNode->SetVolume(volume);
            updated = true;
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Reference Distance");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Reference Distance", &referenceDistance))
        {
            soundNode->SetReferenceDistance(referenceDistance);
            updated = true;
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Roll Off Factor");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##RollOffFactor", &rollOffFactor))
        {
            soundNode->SetRollOffFactor(rollOffFactor);
            updated = true;
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Paused");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::Checkbox("##Paused", &paused))
        {
            soundNode->SetPaused(paused);
            updated = true;
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Separator();
        auto soundPointer = soundNode->GetSound();

        std::string path = "Empty Path";
        if(soundPointer)
            path = soundPointer->GetFilePath();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("File Path");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::TextUnformatted(path.c_str());

        const ImGuiPayload* payload = ImGui::GetDragDropPayload();

        if(payload != NULL && payload->IsDataType("AssetFile"))
        {
            auto filePath = std::string(reinterpret_cast<const char*>(payload->Data));
            if(Lumos::Editor::GetEditor()->IsAudioFile(filePath))
            {
                if(ImGui::BeginDragDropTarget())
                {
                    // Drop directly on to node and append to the end of it's children list.
                    if(ImGui::AcceptDragDropPayload("AssetFile"))
                    {
                        std::string physicalPath;
                        Lumos::VFS::Get().ResolvePhysicalPath(filePath, physicalPath);
                        auto newSound = Lumos::Sound::Create(physicalPath, Lumos::StringUtilities::GetFilePathExtension(filePath));

                        soundNode->SetSound(newSound);
                    }

                    ImGui::EndDragDropTarget();
                }
            }
        }

        if(soundPointer)
        {
            ImGui::NextColumn();

            int bitrate = soundPointer->GetBitRate();
            float frequency = soundPointer->GetFrequency();
            int size = soundPointer->GetSize();
            double length = soundPointer->GetLength();
            int channels = soundPointer->GetChannels();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Bit Rate");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::Text("%i", bitrate);
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Frequency");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::Text("%.2f", frequency);

            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Size");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::Text("%i", size);
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Length");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::Text("%.2lf", length);
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Channels");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::Text("%i", channels);
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            if(updated)
                soundNode->SetSound(soundPointer);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    template <>
    void ComponentEditorWidget<Lumos::Camera>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& camera = reg.get<Lumos::Camera>(e);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        using namespace Lumos;

        float aspect = camera.GetAspectRatio();
        if(ImGuiHelpers::Property("Aspect", aspect, 0.0f, 10.0f))
            camera.SetAspectRatio(aspect);

        float fov = camera.GetFOV();
        if(ImGuiHelpers::Property("Fov", fov, 1.0f, 120.0f))
            camera.SetFOV(fov);

        float near = camera.GetNear();
        if(ImGuiHelpers::Property("Near", near, 0.0f, 10.0f))
            camera.SetNear(near);

        float far = camera.GetFar();
        if(ImGuiHelpers::Property("Far", far, 10.0f, 10000.0f))
            camera.SetFar(far);

        //        float zoom = camera.GetZoom();
        //        ImGuiHelpers::Property("Zoom", m_Zoom, 0.0f, 100.0f);
        //
        //        float offset = camera.GetOffset();
        //        ImGuiHelpers::Property("Offset", m_ProjectionOffset, 0.0f, 10.0f);

        float scale = camera.GetScale();
        if(ImGuiHelpers::Property("Scale", scale, 0.0f, 1000.0f))
            camera.SetScale(scale);

        bool ortho = camera.IsOrthographic();
        if(ImGuiHelpers::Property("Orthograhic", ortho))
            camera.SetIsOrthographic(ortho);

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    template <>
    void ComponentEditorWidget<Lumos::Graphics::Sprite>(entt::registry& reg, entt::registry::entity_type e)
    {
        using namespace Lumos;
        LUMOS_PROFILE_FUNCTION();
        auto& sprite = reg.get<Lumos::Graphics::Sprite>(e);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Position");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        auto pos = sprite.GetPosition();
        if(ImGui::InputFloat2("##Position", Maths::ValuePointer(pos)))
            sprite.SetPosition(pos);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Scale");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        auto scale = sprite.GetScale();
        if(ImGui::InputFloat2("##Scale", Maths::ValuePointer(scale)))
            sprite.SetScale(scale);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Colour");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        auto colour = sprite.GetColour();
        if(ImGui::ColorEdit4("##Colour", Maths::ValuePointer(colour)))
            sprite.SetColour(colour);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        if(ImGui::TreeNode("Texture"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();

            ImGui::AlignTextToFramePadding();
            auto tex = sprite.GetTexture();

            auto imageButtonSize = ImVec2(64, 64) * Application::Get().GetWindowDPI();
            auto callback = std::bind(&Lumos::Graphics::Sprite::SetTextureFromFile, &sprite, std::placeholders::_1);
            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            auto min = ImGui::GetCurrentWindow()->DC.CursorPos;
            auto max = min + imageButtonSize + ImGui::GetStyle().FramePadding;

            bool hoveringButton = ImGui::IsMouseHoveringRect(min, max, false);
            bool showTexture = !(hoveringButton && (payload != NULL && payload->IsDataType("AssetFile")));
            if(tex && showTexture)
            {
                if(ImGui::ImageButton(tex->GetHandle(), imageButtonSize, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
                {
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
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
                if(ImGui::Button(tex ? "" : "Empty", imageButtonSize))
                {
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
                }
            }

            if(payload != NULL && payload->IsDataType("AssetFile"))
            {
                auto filePath = std::string(reinterpret_cast<const char*>(payload->Data));
                if(Lumos::Editor::GetEditor()->IsTextureFile(filePath))
                {
                    if(ImGui::BeginDragDropTarget())
                    {
                        // Drop directly on to node and append to the end of it's children list.
                        if(ImGui::AcceptDragDropPayload("AssetFile"))
                        {
                            callback(filePath);
                            ImGui::EndDragDropTarget();

                            ImGui::Columns(1);
                            ImGui::Separator();
                            ImGui::PopStyleVar(2);

                            ImGui::TreePop();
                            return;
                        }

                        ImGui::EndDragDropTarget();
                    }
                }
            }

            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::TextUnformatted(tex ? tex->GetFilepath().c_str() : "No Texture");
            if(tex)
            {
                ImGuiHelpers::Tooltip(tex->GetFilepath());
                ImGui::Text("%u x %u", tex->GetWidth(), tex->GetHeight());
                ImGui::Text("Mip Levels : %u", tex->GetMipMapLevels());
            }

            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();
            ImGui::TreePop();
        }

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    template <>
    void ComponentEditorWidget<Lumos::Graphics::AnimatedSprite>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        using namespace Lumos;
        using namespace Graphics;
        auto& sprite = reg.get<Lumos::Graphics::AnimatedSprite>(e);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Position");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        auto pos = sprite.GetPosition();
        if(ImGui::InputFloat2("##Position", Maths::ValuePointer(pos)))
            sprite.SetPosition(pos);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Scale");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        auto scale = sprite.GetScale();
        if(ImGui::InputFloat2("##Scale", Maths::ValuePointer(scale)))
            sprite.SetScale(scale);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Colour");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        auto colour = sprite.GetColour();
        if(ImGui::ColorEdit4("##Colour", Maths::ValuePointer(colour)))
            sprite.SetColour(colour);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Current State");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        {
            std::vector<std::string> states;
            auto& animStates = sprite.GetAnimationStates();

            if(animStates.empty())
            {
                ImGui::TextUnformatted("No States Available");
            }
            else
            {
                for(auto& [name, frame] : animStates)
                {
                    states.push_back(name);
                }

                std::string currentStateName = sprite.GetState();
                if(ImGui::BeginCombo("##FrameSelect", currentStateName.c_str(), 0)) // The second parameter is the label previewed before opening the combo.
                {
                    for(int n = 0; n < animStates.size(); n++)
                    {
                        bool is_selected = (currentStateName.c_str() == states[n].c_str());
                        if(ImGui::Selectable(states[n].c_str(), currentStateName.c_str()))
                        {
                            sprite.SetState(states[n]);
                        }
                        if(is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        auto& animStates = sprite.GetAnimationStates();
        if(ImGui::TreeNode("States"))
        {
            //ImGui::Indent(20.0f);
            ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - ImGui::GetFontSize());
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));

            if(ImGui::Button(ICON_MDI_PLUS))
            {
                Graphics::AnimatedSprite::AnimationState state;
                state.Frames = {};
                state.FrameDuration = 1.0f;
                state.Mode = Graphics::AnimatedSprite::PlayMode::Loop;
                animStates["--New--"] = state;
            }

            ImGuiHelpers::Tooltip("Add New State");

            ImGui::PopStyleColor();

            ImGui::Separator();

            int frameID = 0;

            std::vector<std::string> statesToDelete;
            std::vector<std::pair<std::string, std::string>> statesToRename;

            for(auto& [name, state] : animStates)
            {
                ImGui::PushID(frameID);
                bool open = ImGui::TreeNode(&state, "%s", name.c_str());

                ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - ImGui::GetFontSize());
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));

                if(ImGui::Button((ICON_MDI_MINUS "##" + name).c_str()))
                    ImGui::OpenPopup(("##" + name).c_str());

                ImGuiHelpers::Tooltip("Remove State");

                ImGui::PopStyleColor();

                if(ImGui::BeginPopup(("##" + name).c_str(), 3))
                {
                    if(ImGui::Button(("Remove##" + name).c_str()))
                    {
                        statesToDelete.push_back(name);
                    }
                    ImGui::EndPopup();
                }

                if(open)
                {
                    ImGui::Columns(2);

                    ImGui::AlignTextToFramePadding();

                    ImGui::TextUnformatted("Name");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);

                    static char objName[INPUT_BUF_SIZE];
                    strcpy(objName, name.c_str());
                    ImGui::PushItemWidth(-1);

                    bool renameState = false;
                    std::string newName;

                    if(ImGui::InputText("##Name", objName, IM_ARRAYSIZE(objName), 0))
                    {
                        renameState = true;
                        newName = objName;
                    }

                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Duration");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);

                    ImGui::DragFloat("##Duration", &state.FrameDuration);

                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("PlayMode");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);

                    const char* modeTypes[] = { "Loop", "Ping Pong" };
                    std::string mode_current = state.Mode == Graphics::AnimatedSprite::PlayMode::Loop ? "Loop" : "PingPong";
                    if(ImGui::BeginCombo("##ModeSelect", mode_current.c_str(), 0)) // The second parameter is the label previewed before opening the combo.
                    {
                        for(int n = 0; n < 2; n++)
                        {
                            bool is_selected = (mode_current.c_str() == modeTypes[n]);
                            if(ImGui::Selectable(modeTypes[n], mode_current.c_str()))
                            {
                                state.Mode = n == 0 ? Graphics::AnimatedSprite::PlayMode::Loop : Graphics::AnimatedSprite::PlayMode::PingPong;
                            }
                            if(is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }

                    ImGui::Columns(1);
                    if(ImGui::TreeNode("Frames"))
                    {
                        ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - ImGui::GetFontSize());
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));

                        std::vector<Maths::Vector2>& frames = state.Frames;

                        if(ImGui::Button(ICON_MDI_PLUS))
                        {
                            frames.emplace_back(0.0f, 0.0f);
                        }

                        ImGui::PopStyleColor();

                        auto begin = frames.begin();
                        auto end = frames.end();

                        static int numRemoved = 0;
                        for(auto it = begin; it != end; ++it)
                        {
                            auto& pos = (*it);
                            ImGui::PushID(&pos + numRemoved * 100);
                            ImGui::PushItemWidth(ImGui::GetWindowContentRegionWidth() - ImGui::GetFontSize() * 3.0f);

                            ImGui::DragFloat2("##Position", Maths::ValuePointer(pos));

                            ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - ImGui::GetFontSize());

                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));

                            if(ImGui::Button(ICON_MDI_MINUS))
                                ImGui::OpenPopup("Remove");

                            ImGuiHelpers::Tooltip("Remove");

                            ImGui::PopStyleColor();

                            if(ImGui::BeginPopup("Remove", 3))
                            {
                                if(ImGui::Button("Remove"))
                                {
                                    frames.erase(it);
                                    numRemoved++;
                                }
                                ImGui::EndPopup();
                            }

                            ImGui::PopID();
                        }
                        ImGui::TreePop();
                    }

                    if(renameState)
                    {
                        statesToRename.emplace_back(name, newName);
                    }

                    frameID++;

                    ImGui::Separator();
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }

            for(auto& stateName : statesToDelete)
            {
                animStates.erase(stateName);
            }

            for(auto& statePair : statesToRename)
            {
                auto nodeHandler = animStates.extract(statePair.first);
                nodeHandler.key() = statePair.second;
                animStates.insert(std::move(nodeHandler));
            }

            ImGui::Unindent(20.0f);
            ImGui::TreePop();
        }

        if(ImGui::TreeNode("Texture"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();

            //ImGui::AlignTextToFramePadding();
            auto tex = sprite.GetTexture();

            auto imageButtonSize = ImVec2(64, 64) * Application::Get().GetWindowDPI();
            auto callback = std::bind(&Sprite::SetTextureFromFile, &sprite, std::placeholders::_1);
            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            auto min = ImGui::GetCurrentWindow()->DC.CursorPos;
            auto max = min + imageButtonSize + ImGui::GetStyle().FramePadding;

            bool hoveringButton = ImGui::IsMouseHoveringRect(min, max, false);
            bool showTexture = !(hoveringButton && (payload != NULL && payload->IsDataType("AssetFile")));
            if(tex && showTexture)
            {
                if(ImGui::ImageButton(tex->GetHandle(), imageButtonSize, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
                {
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
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
                if(ImGui::Button(tex ? "" : "Empty", imageButtonSize))
                {
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
                }
            }

            if(payload != NULL && payload->IsDataType("AssetFile"))
            {
                auto filePath = std::string(reinterpret_cast<const char*>(payload->Data));
                if(Lumos::Editor::GetEditor()->IsTextureFile(filePath))
                {
                    if(ImGui::BeginDragDropTarget())
                    {
                        // Drop directly on to node and append to the end of it's children list.
                        if(ImGui::AcceptDragDropPayload("AssetFile"))
                        {
                            callback(filePath);
                            ImGui::EndDragDropTarget();

                            ImGui::Columns(1);
                            ImGui::Separator();
                            ImGui::PopStyleVar(2);

                            ImGui::TreePop();
                            return;
                        }

                        ImGui::EndDragDropTarget();
                    }
                }
            }

            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::TextUnformatted(tex ? tex->GetFilepath().c_str() : "No Texture");
            if(tex)
            {
                ImGuiHelpers::Tooltip(tex->GetFilepath());
                ImGui::Text("%u x %u", tex->GetWidth(), tex->GetHeight());
                ImGui::Text("Mip Levels : %u", tex->GetMipMapLevels());
            }

            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();
            ImGui::TreePop();
        }

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    template <>
    void ComponentEditorWidget<Lumos::Graphics::Light>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
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
        Lumos::ImGuiHelpers::Property("Colour", light.Colour, true, Lumos::ImGuiHelpers::PropertyFlag::ColourProperty);
        Lumos::ImGuiHelpers::Property("Intensity", light.Intensity, 0.0f, 4.0f);

        if(light.Type == 1)
            Lumos::ImGuiHelpers::Property("Angle", light.Angle, -1.0f, 1.0f);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Light Type");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        const char* types[] = { "Directional", "Spot", "Point" };
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

    Lumos::Graphics::PrimitiveType GetPrimativeName(const std::string& type)
    {
        LUMOS_PROFILE_FUNCTION();
        if(type == "Cube")
        {
            return Lumos::Graphics::PrimitiveType::Cube;
        }
        else if(type == "Quad")
        {
            return Lumos::Graphics::PrimitiveType::Quad;
        }
        else if(type == "Sphere")
        {
            return Lumos::Graphics::PrimitiveType::Sphere;
        }
        else if(type == "Pyramid")
        {
            return Lumos::Graphics::PrimitiveType::Pyramid;
        }
        else if(type == "Capsule")
        {
            return Lumos::Graphics::PrimitiveType::Capsule;
        }
        else if(type == "Cylinder")
        {
            return Lumos::Graphics::PrimitiveType::Cylinder;
        }
        else if(type == "Terrain")
        {
            return Lumos::Graphics::PrimitiveType::Terrain;
        }

        LUMOS_LOG_ERROR("Primitive not supported");
        return Lumos::Graphics::PrimitiveType::Cube;
    };

    std::string GetPrimativeName(Lumos::Graphics::PrimitiveType type)
    {
        LUMOS_PROFILE_FUNCTION();
        switch(type)
        {
        case Lumos::Graphics::PrimitiveType::Cube:
            return "Cube";
        case Lumos::Graphics::PrimitiveType::Plane:
            return "Plane";
        case Lumos::Graphics::PrimitiveType::Quad:
            return "Quad";
        case Lumos::Graphics::PrimitiveType::Sphere:
            return "Sphere";
        case Lumos::Graphics::PrimitiveType::Pyramid:
            return "Pyramid";
        case Lumos::Graphics::PrimitiveType::Capsule:
            return "Capsule";
        case Lumos::Graphics::PrimitiveType::Cylinder:
            return "Cylinder";
        case Lumos::Graphics::PrimitiveType::Terrain:
            return "Terrain";
        case Lumos::Graphics::PrimitiveType::File:
            return "File";
        }

        LUMOS_LOG_ERROR("Primitive not supported");
        return "";
    };

    void TextureWidget(const char* label, Lumos::Graphics::Material* material, Lumos::Graphics::Texture2D* tex, bool flipImage, float& usingMapProperty, Lumos::Maths::Vector4& colourProperty, const std::function<void(const std::string&)>& callback, const ImVec2& imageButtonSize = ImVec2(64, 64))
    {
        using namespace Lumos;
        if(ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            ImGui::AlignTextToFramePadding();

            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            auto min = ImGui::GetCurrentWindow()->DC.CursorPos;
            auto max = min + imageButtonSize + ImGui::GetStyle().FramePadding;

            bool hoveringButton = ImGui::IsMouseHoveringRect(min, max, false);
            bool showTexture = !(hoveringButton && (payload != NULL && payload->IsDataType("AssetFile")));
            if(tex && showTexture)
            {
                if(ImGui::ImageButton(tex->GetHandle(), imageButtonSize, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
                {
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
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
                if(ImGui::Button(tex ? "" : "Empty", imageButtonSize))
                {
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
                }
            }

            if(payload != NULL && payload->IsDataType("AssetFile"))
            {
                auto filePath = std::string(reinterpret_cast<const char*>(payload->Data));
                if(Lumos::Editor::GetEditor()->IsTextureFile(filePath))
                {
                    if(ImGui::BeginDragDropTarget())
                    {
                        // Drop directly on to node and append to the end of it's children list.
                        if(ImGui::AcceptDragDropPayload("AssetFile"))
                        {
                            callback(filePath);
                            ImGui::EndDragDropTarget();

                            ImGui::Columns(1);
                            ImGui::Separator();
                            ImGui::PopStyleVar();

                            ImGui::TreePop();
                            return;
                        }

                        ImGui::EndDragDropTarget();
                    }
                }
            }

            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::TextUnformatted(tex ? tex->GetFilepath().c_str() : "No Texture");
            if(tex)
            {
                ImGuiHelpers::Tooltip(tex->GetFilepath());
                ImGui::Text("%u x %u", tex->GetWidth(), tex->GetHeight());
                ImGui::Text("Mip Levels : %u", tex->GetMipMapLevels());
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGuiHelpers::Property("Use Map", usingMapProperty, 0.0f, 1.0f);
            ImGuiHelpers::Property("Colour", colourProperty, 0.0f, 1.0f, false, Lumos::ImGuiHelpers::PropertyFlag::ColourProperty);

            ImGui::Columns(1);

            ImGui::Text("Workflow %.1f", material->GetProperties()->workflow);
            ImGui::Separator();
            ImGui::PopStyleVar();

            ImGui::TreePop();
        }
    }

    template <>
    void ComponentEditorWidget<Lumos::Graphics::ModelComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& model = *reg.get<Lumos::Graphics::ModelComponent>(e).ModelRef.get();
        auto& meshes = model.GetMeshes();
        auto primitiveType = model.GetPrimitiveType();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::TextUnformatted("Primitive Type");

        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        const char* shapes[] = { "Sphere", "Cube", "Pyramid", "Capsule", "Cylinder", "Terrain", "File", "Quad" };
        std::string shape_current = GetPrimativeName(primitiveType).c_str();
        if(ImGui::BeginCombo("", shape_current.c_str(), 0)) // The second parameter is the label previewed before opening the combo.
        {
            for(int n = 0; n < 8; n++)
            {
                bool is_selected = (shape_current.c_str() == shapes[n]);
                if(ImGui::Selectable(shapes[n], shape_current.c_str()))
                {
                    meshes.clear();
                    if(strcmp(shapes[n], "File") != 0)
                    {
                        meshes.push_back(Lumos::SharedPtr<Lumos::Graphics::Mesh>(Lumos::Graphics::CreatePrimative(GetPrimativeName(shapes[n]))));
                        model.SetPrimitiveType(GetPrimativeName(shapes[n]));
                    }
                    else
                        model.SetPrimitiveType(Lumos::Graphics::PrimitiveType::File);
                }
                if(is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        if(primitiveType == Lumos::Graphics::PrimitiveType::File)
        {
            ImGui::TextUnformatted("FilePath");

            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::TextUnformatted(model.GetFilePath().c_str());
            Lumos::ImGuiHelpers::Tooltip(model.GetFilePath());

            ImGui::PopItemWidth();
            ImGui::NextColumn();
        }

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();

        int matIndex = 0;

        for(auto mesh : meshes)
        {
            auto material = mesh->GetMaterial();
            std::string matName = "Material";
            matName += std::to_string(matIndex);
            matIndex++;
            if(!material)
            {
                ImGui::TextUnformatted("Empty Material");
                if(ImGui::Button("Add Material", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
                    mesh->SetMaterial(Lumos::CreateSharedPtr<Lumos::Graphics::Material>());
            }
            else if(ImGui::TreeNodeEx(matName.c_str(), 0))
            {
                using namespace Lumos;
                bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();

                Graphics::MaterialProperties* prop = material->GetProperties();
                auto colour = Maths::Vector4();
                auto& textures = material->GetTextures();
                TextureWidget("Albedo", material.get(), textures.albedo.get(), flipImage, prop->usingAlbedoMap, prop->albedoColour, std::bind(&Graphics::Material::SetAlbedoTexture, material, std::placeholders::_1), ImVec2(64, 64) * Application::Get().GetWindowDPI());
                ImGui::Separator();

                TextureWidget("Normal", material.get(), textures.normal.get(), flipImage, prop->usingNormalMap, colour, std::bind(&Graphics::Material::SetNormalTexture, material, std::placeholders::_1), ImVec2(64, 64) * Application::Get().GetWindowDPI());
                ImGui::Separator();

                TextureWidget("Metallic", material.get(), textures.metallic.get(), flipImage, prop->usingMetallicMap, prop->metallicColour, std::bind(&Graphics::Material::SetMetallicTexture, material, std::placeholders::_1), ImVec2(64, 64) * Application::Get().GetWindowDPI());
                ImGui::Separator();

                TextureWidget("Roughness", material.get(), textures.roughness.get(), flipImage, prop->usingRoughnessMap, prop->roughnessColour, std::bind(&Graphics::Material::SetRoughnessTexture, material, std::placeholders::_1), ImVec2(64, 64) * Application::Get().GetWindowDPI());
                ImGui::Separator();

                TextureWidget("AO", material.get(), textures.ao.get(), flipImage, prop->usingAOMap, colour, std::bind(&Graphics::Material::SetAOTexture, material, std::placeholders::_1), ImVec2(64, 64) * Application::Get().GetWindowDPI());
                ImGui::Separator();

                TextureWidget("Emissive", material.get(), textures.emissive.get(), flipImage, prop->usingEmissiveMap, prop->emissiveColour, std::bind(&Graphics::Material::SetEmissiveTexture, material, std::placeholders::_1), ImVec2(64, 64) * Application::Get().GetWindowDPI());

                material->SetMaterialProperites(*prop);
                ImGui::TreePop();
            }
        }
    }

    template <>
    void ComponentEditorWidget<Lumos::Graphics::Environment>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& environment = reg.get<Lumos::Graphics::Environment>(e);
        //Disable image until texturecube is supported
        //Lumos::ImGuiHelpers::Image(environment.GetEnvironmentMap(), Lumos::Maths::Vector2(200, 200));

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::TextUnformatted("File Path");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        static char filePath[INPUT_BUF_SIZE];
        strcpy(filePath, environment.GetFilePath().c_str());

        if(ImGui::InputText("##filePath", filePath, IM_ARRAYSIZE(filePath), 0))
        {
            environment.SetFilePath(filePath);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("File Type");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        static char fileType[INPUT_BUF_SIZE];
        strcpy(fileType, environment.GetFileType().c_str());

        if(ImGui::InputText("##fileType", fileType, IM_ARRAYSIZE(fileType), 0))
        {
            environment.SetFileType(fileType);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Width");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        int width = environment.GetWidth();

        if(ImGui::DragInt("##Width", &width))
        {
            environment.SetWidth(width);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Height");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        int height = environment.GetHeight();

        if(ImGui::DragInt("##Height", &height))
        {
            environment.SetHeight(height);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Num Mips");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        int numMips = environment.GetNumMips();
        if(ImGui::InputInt("##NumMips", &numMips))
        {
            environment.SetNumMips(numMips);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        if(ImGui::Button("Reload", ImVec2(ImGui::GetContentRegionAvail().x, 0.0)))
            environment.Load();

        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    template <>
    void ComponentEditorWidget<Lumos::TextureMatrixComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
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

    template <>
    void ComponentEditorWidget<Lumos::DefaultCameraController>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& controllerComp = reg.get<Lumos::DefaultCameraController>(e);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Controller Type");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        const char* controllerTypes[] = { "Editor", "FPS", "ThirdPerson", "2D", "Custom" };
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

    template <>
    void ComponentEditorWidget<Lumos::Listener>(entt::registry& reg, entt::registry::entity_type e)
    {
    }
}

namespace Lumos
{
    InspectorPanel::InspectorPanel()
    {
        m_Name = ICON_MDI_INFORMATION " Inspector###inspector";
        m_SimpleName = "Inspector";
    }

    static bool init = false;
    void InspectorPanel::OnNewScene(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        if(init)
            return;

        init = true;

        auto& registry = scene->GetRegistry();
        auto& iconMap = m_Editor->GetComponentIconMap();

#define TRIVIAL_COMPONENT(ComponentType, ComponentName)                      \
    {                                                                        \
        std::string Name;                                                    \
        if(iconMap.find(typeid(ComponentType).hash_code()) != iconMap.end()) \
            Name += iconMap[typeid(ComponentType).hash_code()];              \
        else                                                                 \
            Name += iconMap[typeid(Editor).hash_code()];                     \
        Name += "\t";                                                        \
        Name += (ComponentName);                                             \
        m_EnttEditor.registerComponent<ComponentType>(Name.c_str());         \
    }
        TRIVIAL_COMPONENT(Maths::Transform, "Transform");
        TRIVIAL_COMPONENT(Graphics::Model, "Model");
        TRIVIAL_COMPONENT(Graphics::ModelComponent, "Model");
        TRIVIAL_COMPONENT(Camera, "Camera");
        TRIVIAL_COMPONENT(Physics3DComponent, "Physics3D");
        TRIVIAL_COMPONENT(Physics2DComponent, "Physics2D");
        TRIVIAL_COMPONENT(SoundComponent, "Sound");
        TRIVIAL_COMPONENT(Graphics::AnimatedSprite, "Animated Sprite");
        TRIVIAL_COMPONENT(Graphics::Sprite, "Sprite");
        TRIVIAL_COMPONENT(Graphics::Light, "Light");
        TRIVIAL_COMPONENT(LuaScriptComponent, "LuaScript");
        TRIVIAL_COMPONENT(Graphics::Environment, "Environment");
        TRIVIAL_COMPONENT(TextureMatrixComponent, "Texture Matrix");
        TRIVIAL_COMPONENT(DefaultCameraController, "Default Camera Controller");
        TRIVIAL_COMPONENT(Listener, "Listener");
    }

    void InspectorPanel::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();
        auto& registry = Application::Get().GetSceneManager()->GetCurrentScene()->GetRegistry();
        auto selected = m_Editor->GetSelected();

        if(ImGui::Begin(m_Name.c_str(), &m_Active))
        {
            if(selected == entt::null || !registry.valid(selected))
            {
                m_Editor->SetSelected(entt::null);
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
                name = StringUtilities::ToString(entt::to_integral(selected));

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

            ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - ImGui::GetFontSize());

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));

            if(ImGui::Button(ICON_MDI_TUNE))
                ImGui::OpenPopup("SetDebugMode");
            ImGui::PopStyleColor();

            if(ImGui::BeginPopup("SetDebugMode", 3))
            {
                if(ImGui::Selectable("Debug Mode", m_DebugMode))
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
				auto idComponent = registry.try_get<IDComponent>(selected);
				
				ImGui::Text("UUID : %" PRIu64, idComponent->ID);
				
                auto hierarchyComp = registry.try_get<Hierarchy>(selected);

                if(hierarchyComp)
                {
                    if(registry.valid(hierarchyComp->Parent()))
                    {
                        ImGui::Text("Parent : ID: %d", static_cast<int>(registry.entity(hierarchyComp->Parent())));
                    }
                    else
                    {
                        ImGui::TextUnformatted("Parent : null");
                    }

                    entt::entity child = hierarchyComp->First();
                    ImGui::TextUnformatted("Children : ");
                    ImGui::Indent(24.0f);

                    while(child != entt::null)
                    {
                        ImGui::Text("ID: %d", static_cast<int>(registry.entity(child)));

                        auto hierarchy = registry.try_get<Hierarchy>(child);

                        if(hierarchy)
                        {
                            child = hierarchy->Next();
                        }
                    }

                    ImGui::Unindent(24.0f);
                }

                ImGui::Separator();
            }

            ImGui::BeginChild("Components", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_None);
            m_EnttEditor.RenderImGui(registry, selected);
            ImGui::EndChild();
        }
        ImGui::End();
    }

    void InspectorPanel::SetDebugMode(bool mode)
    {
        m_DebugMode = mode;
    }
}
