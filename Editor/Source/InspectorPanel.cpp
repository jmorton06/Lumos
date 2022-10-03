#include "InspectorPanel.h"
#include "Editor.h"
#include "FileBrowserPanel.h"
#include "TextEditPanel.h"
#include <Lumos/Audio/AudioManager.h>
#include <Lumos/Core/Application.h>
#include <Lumos/Core/OS/FileSystem.h>
#include <Lumos/Scene/EntityManager.h>
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
#include <Lumos/ImGui/ImGuiUtilities.h>
#include <Lumos/Physics/LumosPhysicsEngine/CollisionShapes/CuboidCollisionShape.h>
#include <Lumos/Physics/LumosPhysicsEngine/CollisionShapes/SphereCollisionShape.h>
#include <Lumos/Physics/LumosPhysicsEngine/CollisionShapes/PyramidCollisionShape.h>
#include <Lumos/Physics/LumosPhysicsEngine/CollisionShapes/CapsuleCollisionShape.h>
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <glm/gtx/matrix_decompose.hpp>

//#include <sol/sol.hpp>
#include <inttypes.h>

namespace MM
{
    template <>
    void ComponentEditorWidget<Lumos::LuaScriptComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& script = reg.get<Lumos::LuaScriptComponent>(e);
        bool loaded  = false;
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

        // auto& solEnv         = script.GetSolEnvironment();
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
                int fileIndex                 = 0;
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
                            ((Lumos::TextEditPanel*)textEditPanel)->SetErrors(script.GetErrors()); });

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

        /*     ImGui::Indent();
             for(auto&& function : solEnv)
             {
                 if(function.second.is<sol::function>())
                 {
                     ImGui::TextUnformatted(function.first.as<std::string>().c_str());
                 }
             }
             ImGui::Unindent();*/
    }

    template <>
    void ComponentEditorWidget<Lumos::Maths::Transform>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& transform = reg.get<Lumos::Maths::Transform>(e);

        auto rotation   = glm::degrees(glm::eulerAngles(transform.GetLocalOrientation()));
        auto position   = transform.GetLocalPosition();
        auto scale      = transform.GetLocalScale();
        float itemWidth = (ImGui::GetContentRegionAvail().x - (ImGui::GetFontSize() * 3.0f)) / 3.0f;

        // Call this to fix alignment with columns
        ImGui::AlignTextToFramePadding();

        if(Lumos::ImGuiUtilities::PorpertyTransform("Position", position, itemWidth))
            transform.SetLocalPosition(position);

        ImGui::SameLine();
        if(Lumos::ImGuiUtilities::PorpertyTransform("Rotation", rotation, itemWidth))
        {
            float pitch = Lumos::Maths::Min(rotation.x, 89.9f);
            pitch       = Lumos::Maths::Max(pitch, -89.9f);
            transform.SetLocalOrientation(glm::quat(glm::radians(glm::vec3(pitch, rotation.y, rotation.z))));
        }

        ImGui::SameLine();
        if(Lumos::ImGuiUtilities::PorpertyTransform("Scale", scale, itemWidth))
        {
            transform.SetLocalScale(scale);
        }

        ImGui::Columns(1);
        ImGui::Separator();
        // ImGui::PopStyleVar();
    }

    static void CuboidCollisionShapeInspector(Lumos::CuboidCollisionShape* shape, const Lumos::RigidBody3DComponent& phys)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Half Dimensions");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        glm::vec3 size = shape->GetHalfDimensions();
        if(ImGui::DragFloat3("##CollisionShapeHalfDims", glm::value_ptr(size), 1.0f, 0.0f, 10000.0f, "%.2f"))
        {
            shape->SetHalfDimensions(size);
            phys.GetRigidBody()->CollisionShapeUpdated();
        }
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
    }

    static void SphereCollisionShapeInspector(Lumos::SphereCollisionShape* shape, const Lumos::RigidBody3DComponent& phys)
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

    static void PyramidCollisionShapeInspector(Lumos::PyramidCollisionShape* shape, const Lumos::RigidBody3DComponent& phys)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Half Dimensions");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        glm::vec3 size = shape->GetHalfDimensions();
        if(ImGui::DragFloat3("##CollisionShapeHalfDims", glm::value_ptr(size), 1.0f, 0.0f, 10000.0f, "%.2f"))
        {
            shape->SetHalfDimensions(size);
            phys.GetRigidBody()->CollisionShapeUpdated();
        }
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
    }

    static void CapsuleCollisionShapeInspector(Lumos::CapsuleCollisionShape* shape, const Lumos::RigidBody3DComponent& phys)
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

        float height = shape->GetHeight();
        if(ImGui::DragFloat("##CollisionShapeHeight", &height, 1.0f, 0.0f, 10000.0f, "%.2f"))
        {
            shape->SetHeight(height);
            phys.GetRigidBody()->CollisionShapeUpdated();
        }
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
    }

    static void HullCollisionShapeInspector(Lumos::HullCollisionShape* shape, const Lumos::RigidBody3DComponent& phys)
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
    void ComponentEditorWidget<Lumos::AxisConstraintComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        using namespace Lumos;
        LUMOS_PROFILE_FUNCTION();
        ImGuiUtilities::ScopedStyle(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

        ImGui::Columns(2);
        ImGui::Separator();
        AxisConstraintComponent& axisConstraintComponent = reg.get<Lumos::AxisConstraintComponent>(e);

        uint64_t entityID = axisConstraintComponent.GetEntityID();
        Axes axes         = axisConstraintComponent.GetAxes();
        Entity entity     = Application::Get().GetCurrentScene()->GetEntityManager()->GetEntityByUUID(entityID);

        bool hasName = entity ? entity.HasComponent<NameComponent>() : false;
        std::string name;
        if(hasName)
            name = entity.GetComponent<NameComponent>().name;
        else
            name = "Empty";
        ImGui::TextUnformatted("Entity");
        ImGui::NextColumn();
        ImGui::Text("%s", name.c_str());
        ImGui::NextColumn();

        std::vector<std::string> entities;
        uint64_t currentEntityID = axisConstraintComponent.GetEntityID();
        int index                = 0;
        int selectedIndex        = 0;

        auto physics3dEntities = Application::Get().GetCurrentScene()->GetEntityManager()->GetRegistry().view<Lumos::RigidBody3DComponent>();

        for(auto entity : physics3dEntities)
        {
            if(Entity(entity, Application::Get().GetCurrentScene()).GetID() == currentEntityID)
                selectedIndex = index;

            entities.push_back(Entity(entity, Application::Get().GetCurrentScene()).GetName());

            index++;
        }

        static std::string possibleAxes[7] = { "X", "Y", "Z", "XY", "XZ", "YZ", "XYZ" };

        selectedIndex = (int)axes;

        bool updated = Lumos::ImGuiUtilities::PropertyDropdown("Axes", possibleAxes, 7, &selectedIndex);
        if(updated)
            axisConstraintComponent.SetAxes((Axes)selectedIndex);

        // bool updated = Lumos::ImGuiUtilities::PropertyDropdown("Entity", entities.data(), (int)entities.size(), &selectedIndex);

        // if(updated)
        // axisConstraintComponent.SetEntity(Entity(physics3dEntities[selectedIndex], Application::Get().GetCurrentScene()).GetID());

        ImGui::Columns(1);
    }

    template <>
    void ComponentEditorWidget<Lumos::RigidBody3DComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();
        auto& phys = reg.get<Lumos::RigidBody3DComponent>(e);

        auto pos             = phys.GetRigidBody()->GetPosition();
        auto force           = phys.GetRigidBody()->GetForce();
        auto torque          = phys.GetRigidBody()->GetTorque();
        auto orientation     = glm::eulerAngles(phys.GetRigidBody()->GetOrientation());
        auto angularVelocity = phys.GetRigidBody()->GetAngularVelocity();
        auto friction        = phys.GetRigidBody()->GetFriction();
        auto isStatic        = phys.GetRigidBody()->GetIsStatic();
        auto isRest          = phys.GetRigidBody()->GetIsAtRest();
        auto mass            = 1.0f / phys.GetRigidBody()->GetInverseMass();
        auto velocity        = phys.GetRigidBody()->GetLinearVelocity();
        auto elasticity      = phys.GetRigidBody()->GetElasticity();
        auto angularFactor   = phys.GetRigidBody()->GetAngularFactor();
        auto collisionShape  = phys.GetRigidBody()->GetCollisionShape();
        auto UUID            = phys.GetRigidBody()->GetUUID();

        Lumos::ImGuiUtilities::Property("UUID", (uint32_t&)UUID, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);

        if(Lumos::ImGuiUtilities::Property("Position", pos))
            phys.GetRigidBody()->SetPosition(pos);

        if(Lumos::ImGuiUtilities::Property("Velocity", velocity))
            phys.GetRigidBody()->SetLinearVelocity(velocity);

        if(Lumos::ImGuiUtilities::Property("Torque", torque))
            phys.GetRigidBody()->SetTorque(torque);

        if(Lumos::ImGuiUtilities::Property("Orientation", orientation))
            phys.GetRigidBody()->SetOrientation(glm::quat(orientation));

        if(Lumos::ImGuiUtilities::Property("Force", force))
            phys.GetRigidBody()->SetForce(force);

        if(Lumos::ImGuiUtilities::Property("Angular Velocity", angularVelocity))
            phys.GetRigidBody()->SetAngularVelocity(angularVelocity);

        if(Lumos::ImGuiUtilities::Property("Friction", friction, 0.0f, 1.0f))
            phys.GetRigidBody()->SetFriction(friction);

        if(Lumos::ImGuiUtilities::Property("Mass", mass))
        {
            mass = Lumos::Maths::Max(mass, 0.0001f);
            phys.GetRigidBody()->SetInverseMass(1.0f / mass);
        }

        if(Lumos::ImGuiUtilities::Property("Elasticity", elasticity))
            phys.GetRigidBody()->SetElasticity(elasticity);

        if(Lumos::ImGuiUtilities::Property("Static", isStatic))
            phys.GetRigidBody()->SetIsStatic(isStatic);

        if(Lumos::ImGuiUtilities::Property("At Rest", isRest))
            phys.GetRigidBody()->SetIsAtRest(isRest);

        if(Lumos::ImGuiUtilities::Property("Angular Factor", angularFactor))
            phys.GetRigidBody()->SetAngularFactor(angularFactor);

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();

        std::vector<std::string> shapes = { "Sphere", "Cuboid", "Pyramid", "Capsule", "Hull" };
        int selectedIndex               = 0;
        std::string shape_current       = collisionShape ? CollisionShapeTypeToString(collisionShape->GetType()) : "";
        int index                       = 0;
        for(auto& shape : shapes)
        {
            if(shape == shape_current)
            {
                selectedIndex = index;
                break;
            }
            index++;
        }

        bool updated = Lumos::ImGuiUtilities::PropertyDropdown("Collision Shape", shapes.data(), 5, &selectedIndex);

        if(updated)
            phys.GetRigidBody()->SetCollisionShape(StringToCollisionShapeType(shapes[selectedIndex]));

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
    }

    template <>
    void ComponentEditorWidget<Lumos::RigidBody2DComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& phys = reg.get<Lumos::RigidBody2DComponent>(e);

        auto pos      = phys.GetRigidBody()->GetPosition();
        auto angle    = phys.GetRigidBody()->GetAngle();
        auto friction = phys.GetRigidBody()->GetFriction();
        auto isStatic = phys.GetRigidBody()->GetIsStatic();
        auto isRest   = phys.GetRigidBody()->GetIsAtRest();

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

        const char* shapes[]      = { "Circle", "Square", "Custom" };
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
        auto& sound    = reg.get<Lumos::SoundComponent>(e);
        auto soundNode = sound.GetSoundNode();

        bool updated = false;

        auto pos               = soundNode->GetPosition();
        auto radius            = soundNode->GetRadius();
        auto paused            = soundNode->GetPaused();
        auto pitch             = soundNode->GetPitch();
        auto volume            = soundNode->GetVolume();
        auto referenceDistance = soundNode->GetReferenceDistance();
        auto rollOffFactor     = soundNode->GetRollOffFactor();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        if(Lumos::ImGuiUtilities::Property("Position", pos))
        {
            soundNode->SetPosition(pos);
            updated = true;
        }

        if(Lumos::ImGuiUtilities::Property("Radius", radius))
        {
            soundNode->SetRadius(radius);
            updated = true;
        }

        if(Lumos::ImGuiUtilities::Property("Pitch", pitch))
        {
            soundNode->SetPitch(pitch);
            updated = true;
        }

        if(Lumos::ImGuiUtilities::Property("Volume", volume))
        {
            soundNode->SetVolume(volume);
            updated = true;
        }

        if(Lumos::ImGuiUtilities::Property("Reference Distance", referenceDistance))
        {
            soundNode->SetReferenceDistance(referenceDistance);
            updated = true;
        }

        if(Lumos::ImGuiUtilities::Property("RollOffFactor", rollOffFactor))
        {
            soundNode->SetRollOffFactor(paused);
            updated = true;
        }

        if(Lumos::ImGuiUtilities::Property("Paused", paused))
        {
            soundNode->SetPaused(paused);
            updated = true;
        }

        ImGui::Separator();
        auto soundPointer = soundNode->GetSound();

        std::string path = "Empty Path";
        if(soundPointer)
            path = soundPointer->GetFilePath();

        Lumos::ImGuiUtilities::Property("File Path", path);

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
            int bitrate     = soundPointer->GetBitRate();
            float frequency = soundPointer->GetFrequency();
            int size        = soundPointer->GetSize();
            double length   = soundPointer->GetLength();
            int channels    = soundPointer->GetChannels();

            Lumos::ImGuiUtilities::Property("Bit Rate", bitrate, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);
            Lumos::ImGuiUtilities::Property("Frequency", frequency, 0.0f, 0.0f, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);
            Lumos::ImGuiUtilities::Property("Size", size, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);
            Lumos::ImGuiUtilities::Property("Length", length, 0.0, 0.0, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);
            Lumos::ImGuiUtilities::Property("Channels", channels, 0, 0, Lumos::ImGuiUtilities::PropertyFlag::ReadOnly);

            if(updated)
                soundNode->SetSound(soundPointer);
        }

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    template <>
    void ComponentEditorWidget<Lumos::Camera>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& camera = reg.get<Lumos::Camera>(e);

        Lumos::ImGuiUtilities::ScopedStyle(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        using namespace Lumos;

        float aspect = camera.GetAspectRatio();
        if(ImGuiUtilities::Property("Aspect", aspect, 0.0f, 10.0f))
            camera.SetAspectRatio(aspect);

        float fov = camera.GetFOV();
        if(ImGuiUtilities::Property("Fov", fov, 1.0f, 120.0f))
            camera.SetFOV(fov);

        float near = camera.GetNear();
        if(ImGuiUtilities::Property("Near", near, 0.0f, 10.0f))
            camera.SetNear(near);

        float far = camera.GetFar();
        if(ImGuiUtilities::Property("Far", far, 10.0f, 10000.0f))
            camera.SetFar(far);

        float scale = camera.GetScale();
        if(ImGuiUtilities::Property("Scale", scale, 0.0f, 1000.0f))
            camera.SetScale(scale);

        bool ortho = camera.IsOrthographic();
        if(ImGuiUtilities::Property("Orthograhic", ortho))
            camera.SetIsOrthographic(ortho);

        float aperture = camera.GetAperture();
        if(ImGuiUtilities::Property("Aperture", aperture, 0.0f, 200.0f))
            camera.SetAperture(aperture);

        float shutterSpeed = camera.GetShutterSpeed();
        if(ImGuiUtilities::Property("Shutter Speed", shutterSpeed, 0.0f, 1.0f))
            camera.SetShutterSpeed(shutterSpeed);

        float sensitivity = camera.GetSensitivity();
        if(ImGuiUtilities::Property("ISO", sensitivity, 0.0f, 5000.0f))
            camera.SetSensitivity(sensitivity);

        float exposure = camera.GetExposure();
        ImGuiUtilities::Property("Exposure", exposure, 0.0f, 0.0f, ImGuiUtilities::PropertyFlag::ReadOnly);

        ImGui::Columns(1);
        ImGui::Separator();
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
        if(ImGui::InputFloat2("##Position", glm::value_ptr(pos)))
            sprite.SetPosition(pos);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Scale");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        auto scale = sprite.GetScale();
        if(ImGui::InputFloat2("##Scale", glm::value_ptr(scale)))
            sprite.SetScale(scale);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Colour");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        auto colour = sprite.GetColour();
        if(ImGui::ColorEdit4("##Colour", glm::value_ptr(colour)))
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

            auto imageButtonSize        = ImVec2(64, 64) * Application::Get().GetWindowDPI();
            auto callback               = std::bind(&Lumos::Graphics::Sprite::SetTextureFromFile, &sprite, std::placeholders::_1);
            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            auto min                    = ImGui::GetCurrentWindow()->DC.CursorPos;
            auto max                    = min + imageButtonSize + ImGui::GetStyle().FramePadding;

            bool hoveringButton = ImGui::IsMouseHoveringRect(min, max, false);
            bool showTexture    = !(hoveringButton && (payload != NULL && payload->IsDataType("AssetFile")));
            if(tex && showTexture)
            {
                if(ImGui::ImageButton((const char*)(tex), tex->GetHandle(), imageButtonSize, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
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
                ImGuiUtilities::Tooltip(tex->GetFilepath());
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
    void ComponentEditorWidget<Lumos::TextComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        using namespace Lumos;
        LUMOS_PROFILE_FUNCTION();
        auto& text = reg.get<Lumos::TextComponent>(e);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGuiUtilities::PropertyMultiline("Text String", text.TextString);
        if(text.FontHandle)
        {
            Lumos::ImGuiUtilities::PropertyConst("FilePath", text.FontHandle->GetFilePath());
        }

        Lumos::ImGuiUtilities::Property("Colour", text.Colour, true, Lumos::ImGuiUtilities::PropertyFlag::ColourProperty);
        Lumos::ImGuiUtilities::Property("Outline Colour", text.OutlineColour, true, Lumos::ImGuiUtilities::PropertyFlag::ColourProperty);
        Lumos::ImGuiUtilities::Property("Outline Width", text.OutlineWidth);

        Lumos::ImGuiUtilities::Property("Line Spacing", text.LineSpacing);
        Lumos::ImGuiUtilities::Property("Max Width", text.MaxWidth);

        ImGui::Columns(1);

        auto callback = std::bind(&TextComponent::LoadFont, &text, std::placeholders::_1);

        ImGui::Separator();

        if(ImGui::Button("Load Font File"))
        {
            Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
            Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
        }

        ImGui::Separator();

        ImGui::Columns(2);

        if(ImGui::TreeNode("Texture"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();

            ImGui::AlignTextToFramePadding();
            auto tex = text.FontHandle ? text.FontHandle->GetFontAtlas() : Lumos::Graphics::Font::GetDefaultFont()->GetFontAtlas();

            auto imageButtonSize        = ImVec2(64, 64) * Application::Get().GetWindowDPI();
            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            auto min                    = ImGui::GetCurrentWindow()->DC.CursorPos;
            auto max                    = min + imageButtonSize + ImGui::GetStyle().FramePadding;

            bool hoveringButton = ImGui::IsMouseHoveringRect(min, max, false);
            bool showTexture    = !(hoveringButton && (payload != NULL && payload->IsDataType("Font")));
            if(tex && showTexture)
            {
                if(ImGui::ImageButton((const char*)(tex.get()), tex->GetHandle(), imageButtonSize, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
                {
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
                }

                if(ImGui::IsItemHovered() && tex)
                {
                    ImGui::BeginTooltip();
                    ImGui::Image(tex->GetHandle(), ImVec2(512, 512), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
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

            if(payload != NULL && payload->IsDataType("Font"))
            {
                auto filePath = std::string(reinterpret_cast<const char*>(payload->Data));
                if(Lumos::Editor::GetEditor()->IsFontFile(filePath))
                {
                    if(ImGui::BeginDragDropTarget())
                    {
                        // Drop directly on to node and append to the end of it's children list.
                        if(ImGui::AcceptDragDropPayload("Font"))
                        {
                            Application::Get().GetFontLibrary()->Load(filePath, text.FontHandle);
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
                ImGuiUtilities::Tooltip(tex->GetFilepath());
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
        if(ImGui::InputFloat2("##Position", glm::value_ptr(pos)))
            sprite.SetPosition(pos);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Scale");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        auto scale = sprite.GetScale();
        if(ImGui::InputFloat2("##Scale", glm::value_ptr(scale)))
            sprite.SetScale(scale);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Colour");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        auto colour = sprite.GetColour();
        if(ImGui::ColorEdit4("##Colour", glm::value_ptr(colour)))
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
            // ImGui::Indent(20.0f);
            ImGui::SameLine((ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin()).x - ImGui::GetFontSize());
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));

            if(ImGui::Button(ICON_MDI_PLUS))
            {
                Graphics::AnimatedSprite::AnimationState state;
                state.Frames          = {};
                state.FrameDuration   = 1.0f;
                state.Mode            = Graphics::AnimatedSprite::PlayMode::Loop;
                animStates["--New--"] = state;
            }

            ImGuiUtilities::Tooltip("Add New State");

            ImGui::PopStyleColor();

            ImGui::Separator();

            int frameID = 0;

            std::vector<std::string> statesToDelete;
            std::vector<std::pair<std::string, std::string>> statesToRename;

            for(auto& [name, state] : animStates)
            {
                ImGui::PushID(frameID);
                bool open = ImGui::TreeNode(&state, "%s", name.c_str());

                ImGui::SameLine((ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin()).x - ImGui::GetFontSize());
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));

                if(ImGui::Button((ICON_MDI_MINUS "##" + name).c_str()))
                    ImGui::OpenPopup(("##" + name).c_str());

                ImGuiUtilities::Tooltip("Remove State");

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
                        newName     = objName;
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

                    const char* modeTypes[]  = { "Loop", "Ping Pong" };
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
                        ImGui::SameLine((ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin()).x - ImGui::GetFontSize());
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));

                        std::vector<glm::vec2>& frames = state.Frames;

                        if(ImGui::Button(ICON_MDI_PLUS))
                        {
                            frames.emplace_back(0.0f, 0.0f);
                        }

                        ImGui::PopStyleColor();

                        auto begin = frames.begin();
                        auto end   = frames.end();

                        static int numRemoved = 0;
                        for(auto it = begin; it != end; ++it)
                        {
                            auto& pos = (*it);
                            ImGui::PushID(&pos + numRemoved * 100);
                            ImGui::PushItemWidth((ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin()).x - ImGui::GetFontSize() * 3.0f);

                            ImGui::DragFloat2("##Position", glm::value_ptr(pos));

                            ImGui::SameLine((ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin()).x - ImGui::GetFontSize());

                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));

                            if(ImGui::Button(ICON_MDI_MINUS))
                                ImGui::OpenPopup("Remove");

                            ImGuiUtilities::Tooltip("Remove");

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
                auto nodeHandler  = animStates.extract(statePair.first);
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

            // ImGui::AlignTextToFramePadding();
            auto tex = sprite.GetTexture();

            auto imageButtonSize        = ImVec2(64, 64) * Application::Get().GetWindowDPI();
            auto callback               = std::bind(&Sprite::SetTextureFromFile, &sprite, std::placeholders::_1);
            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            auto min                    = ImGui::GetCurrentWindow()->DC.CursorPos;
            auto max                    = min + imageButtonSize + ImGui::GetStyle().FramePadding;

            bool hoveringButton = ImGui::IsMouseHoveringRect(min, max, false);
            bool showTexture    = !(hoveringButton && (payload != NULL && payload->IsDataType("AssetFile")));
            if(tex && showTexture)
            {
                if(ImGui::ImageButton((const char*)(tex), tex->GetHandle(), imageButtonSize, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
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
                ImGuiUtilities::Tooltip(tex->GetFilepath());
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
            Lumos::ImGuiUtilities::Property("Position", light.Position);

        if(light.Type != 2)
            Lumos::ImGuiUtilities::Property("Direction", light.Direction);

        if(light.Type != 0)
            Lumos::ImGuiUtilities::Property("Radius", light.Radius, 0.0f, 100.0f);
        Lumos::ImGuiUtilities::Property("Colour", light.Colour, true, Lumos::ImGuiUtilities::PropertyFlag::ColourProperty);
        Lumos::ImGuiUtilities::Property("Intensity", light.Intensity, 0.0f, 4.0f);

        if(light.Type == 1)
            Lumos::ImGuiUtilities::Property("Angle", light.Angle, -1.0f, 1.0f);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Light Type");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        const char* types[]       = { "Directional", "Spot", "Point" };
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
        case Lumos::Graphics::PrimitiveType::None:
            return "None";
        }

        LUMOS_LOG_ERROR("Primitive not supported");
        return "";
    };

    void TextureWidget(const char* label, Lumos::Graphics::Material* material, Lumos::Graphics::Texture2D* tex, bool flipImage, float& usingMapProperty, glm::vec4& colourProperty, const std::function<void(const std::string&)>& callback, const ImVec2& imageButtonSize = ImVec2(64, 64))
    {
        using namespace Lumos;
        if(ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            ImGui::AlignTextToFramePadding();

            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            auto min                    = ImGui::GetCurrentWindow()->DC.CursorPos;
            auto max                    = min + imageButtonSize + ImGui::GetStyle().FramePadding;

            bool hoveringButton = ImGui::IsMouseHoveringRect(min, max, false);
            bool showTexture    = !(hoveringButton && (payload != NULL && payload->IsDataType("AssetFile")));
            if(tex && showTexture)
            {
                if(ImGui::ImageButton((const char*)(tex), tex->GetHandle(), imageButtonSize, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
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
                ImGuiUtilities::Tooltip(tex->GetFilepath());
                ImGui::Text("%u x %u", tex->GetWidth(), tex->GetHeight());
                ImGui::Text("Mip Levels : %u", tex->GetMipMapLevels());
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGuiUtilities::Property("Use Map", usingMapProperty, 0.0f, 1.0f);
            ImGuiUtilities::Property("Colour", colourProperty, 0.0f, 1.0f, false, Lumos::ImGuiUtilities::PropertyFlag::ColourProperty);

            ImGui::Columns(1);

            ImGui::Separator();
            ImGui::PopStyleVar();

            ImGui::TreePop();
        }
    }

    void TextureWidget(const char* label, Lumos::Graphics::Material* material, Lumos::Graphics::Texture2D* tex, bool flipImage, float& usingMapProperty, float& amount, const std::function<void(const std::string&)>& callback, const ImVec2& imageButtonSize = ImVec2(64, 64))
    {
        using namespace Lumos;
        if(ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            ImGui::AlignTextToFramePadding();

            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            auto min                    = ImGui::GetCurrentWindow()->DC.CursorPos;
            auto max                    = min + imageButtonSize + ImGui::GetStyle().FramePadding;

            bool hoveringButton = ImGui::IsMouseHoveringRect(min, max, false);
            bool showTexture    = !(hoveringButton && (payload != NULL && payload->IsDataType("AssetFile")));
            if(tex && showTexture)
            {
                if(ImGui::ImageButton((const char*)(tex), tex->GetHandle(), imageButtonSize, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
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
                ImGuiUtilities::Tooltip(tex->GetFilepath());
                ImGui::Text("%u x %u", tex->GetWidth(), tex->GetHeight());
                ImGui::Text("Mip Levels : %u", tex->GetMipMapLevels());
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGuiUtilities::Property("Use Map", usingMapProperty, 0.0f, 1.0f);
            ImGuiUtilities::Property("Value", amount, 0.0f, 20.0f);

            ImGui::Columns(1);

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

        auto primitiveType = reg.get<Lumos::Graphics::ModelComponent>(e).ModelRef ? model.GetPrimitiveType() : Lumos::Graphics::PrimitiveType::None;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::TextUnformatted("Primitive Type");

        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        const char* shapes[]      = { "Sphere", "Cube", "Pyramid", "Capsule", "Cylinder", "Terrain", "File", "Quad", "None" };
        std::string shape_current = GetPrimativeName(primitiveType).c_str();
        if(ImGui::BeginCombo("", shape_current.c_str(), 0)) // The second parameter is the label previewed before opening the combo.
        {
            for(int n = 0; n < 8; n++)
            {
                bool is_selected = (shape_current.c_str() == shapes[n]);
                if(ImGui::Selectable(shapes[n], shape_current.c_str()))
                {
                    if(reg.get<Lumos::Graphics::ModelComponent>(e).ModelRef)
                        model.GetMeshes().clear();

                    if(strcmp(shapes[n], "File") != 0)
                    {
                        if(reg.get<Lumos::Graphics::ModelComponent>(e).ModelRef)
                        {
                            model.GetMeshes().push_back(Lumos::SharedPtr<Lumos::Graphics::Mesh>(Lumos::Graphics::CreatePrimative(GetPrimativeName(shapes[n]))));
                            model.SetPrimitiveType(GetPrimativeName(shapes[n]));
                        }
                        else
                        {
                            reg.get<Lumos::Graphics::ModelComponent>(e).LoadPrimitive(GetPrimativeName(shapes[n]));
                        }
                    }
                    else
                    {
                        if(reg.get<Lumos::Graphics::ModelComponent>(e).ModelRef)
                            model.SetPrimitiveType(Lumos::Graphics::PrimitiveType::File);
                    }
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
            Lumos::ImGuiUtilities::Tooltip(model.GetFilePath());

            ImGui::PopItemWidth();
            ImGui::NextColumn();
        }

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();

        int matIndex = 0;

        if(!reg.get<Lumos::Graphics::ModelComponent>(e).ModelRef)
            return;

        auto& meshes = reg.get<Lumos::Graphics::ModelComponent>(e).ModelRef->GetMeshes();
        for(auto mesh : meshes)
        {
            auto material       = mesh->GetMaterial();
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

                bool twoSided    = material->GetFlag(Lumos::Graphics::Material::RenderFlags::TWOSIDED);
                bool depthTested = material->GetFlag(Lumos::Graphics::Material::RenderFlags::DEPTHTEST);

                if(ImGuiUtilities::Property("Two Sided", twoSided))
                    material->SetFlag(Lumos::Graphics::Material::RenderFlags::TWOSIDED, twoSided);

                if(ImGuiUtilities::Property("Depth Tested", depthTested))
                    material->SetFlag(Lumos::Graphics::Material::RenderFlags::DEPTHTEST, depthTested);

                Graphics::MaterialProperties* prop = material->GetProperties();
                auto colour                        = glm::vec4();
                float normal                       = 0.0f;
                auto& textures                     = material->GetTextures();
                TextureWidget("Albedo", material.get(), textures.albedo.get(), flipImage, prop->albedoMapFactor, prop->albedoColour, std::bind(&Graphics::Material::SetAlbedoTexture, material, std::placeholders::_1), ImVec2(64, 64) * Application::Get().GetWindowDPI());
                ImGui::Separator();

                TextureWidget("Normal", material.get(), textures.normal.get(), flipImage, prop->normalMapFactor, normal, std::bind(&Graphics::Material::SetNormalTexture, material, std::placeholders::_1), ImVec2(64, 64) * Application::Get().GetWindowDPI());
                ImGui::Separator();

                TextureWidget("Metallic", material.get(), textures.metallic.get(), flipImage, prop->metallicMapFactor, prop->metallic, std::bind(&Graphics::Material::SetMetallicTexture, material, std::placeholders::_1), ImVec2(64, 64) * Application::Get().GetWindowDPI());
                ImGui::Separator();

                TextureWidget("Roughness", material.get(), textures.roughness.get(), flipImage, prop->roughnessMapFactor, prop->roughness, std::bind(&Graphics::Material::SetRoughnessTexture, material, std::placeholders::_1), ImVec2(64, 64) * Application::Get().GetWindowDPI());
                ImGui::Separator();

                TextureWidget("AO", material.get(), textures.ao.get(), flipImage, prop->occlusionMapFactor, normal, std::bind(&Graphics::Material::SetAOTexture, material, std::placeholders::_1), ImVec2(64, 64) * Application::Get().GetWindowDPI());
                ImGui::Separator();

                TextureWidget("Emissive", material.get(), textures.emissive.get(), flipImage, prop->emissiveMapFactor, prop->emissive, std::bind(&Graphics::Material::SetEmissiveTexture, material, std::placeholders::_1), ImVec2(64, 64) * Application::Get().GetWindowDPI());

                ImGui::Columns(2);

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("WorkFlow");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);

                int workFlow = (int)material->GetProperties()->workflow;

                if(ImGui::DragInt("##WorkFlow", &workFlow, 0.3f, 0, 2))
                {
                    material->GetProperties()->workflow = (float)workFlow;
                }

                ImGui::PopItemWidth();
                ImGui::NextColumn();

                material->SetMaterialProperites(*prop);
                ImGui::Columns(1);
                ImGui::TreePop();
            }
        }
    }

    template <>
    void ComponentEditorWidget<Lumos::Graphics::Environment>(entt::registry& reg, entt::registry::entity_type e)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& environment = reg.get<Lumos::Graphics::Environment>(e);
        // Disable image until texturecube is supported
        // Lumos::ImGuiUtilities::Image(environment.GetEnvironmentMap(), glm::vec2(200, 200));

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
        glm::mat4& mat      = textureMatrix.GetMatrix();

        glm::vec3 skew;
        glm::vec3 position;
        glm::vec3 scale;
        glm::vec4 perspective;
        glm::quat rotation;
        glm::decompose(mat, scale, rotation, position, skew, perspective);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Position");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Position", glm::value_ptr(position)))
        {
            Lumos::Maths::SetTranslation(mat, position);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Rotation");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Rotation", glm::value_ptr(rotation)))
        {
            float pitch = Lumos::Maths::Min(rotation.x, 89.9f);
            pitch       = Lumos::Maths::Max(pitch, -89.9f);
            Lumos::Maths::SetRotation(mat, glm::vec3(pitch, rotation.y, rotation.z));
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Scale");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Scale", glm::value_ptr(scale), 0.1f))
        {
            Lumos::Maths::SetScale(mat, scale);
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
        m_Name       = ICON_MDI_INFORMATION " Inspector###inspector";
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
        auto& iconMap  = m_Editor->GetComponentIconMap();

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
        // TRIVIAL_COMPONENT(Graphics::Model, "Model");
        TRIVIAL_COMPONENT(Graphics::ModelComponent, "ModelComponent");
        TRIVIAL_COMPONENT(Camera, "Camera");
        TRIVIAL_COMPONENT(AxisConstraintComponent, "AxisConstraint");
        TRIVIAL_COMPONENT(RigidBody3DComponent, "Physics3D");
        TRIVIAL_COMPONENT(RigidBody2DComponent, "Physics2D");
        TRIVIAL_COMPONENT(SoundComponent, "Sound");
        TRIVIAL_COMPONENT(Graphics::AnimatedSprite, "Animated Sprite");
        TRIVIAL_COMPONENT(Graphics::Sprite, "Sprite");
        TRIVIAL_COMPONENT(Graphics::Light, "Light");
        TRIVIAL_COMPONENT(LuaScriptComponent, "LuaScript");
        TRIVIAL_COMPONENT(Graphics::Environment, "Environment");
        TRIVIAL_COMPONENT(TextureMatrixComponent, "Texture Matrix");
        TRIVIAL_COMPONENT(DefaultCameraController, "Default Camera Controller");
        TRIVIAL_COMPONENT(Listener, "Listener");
        TRIVIAL_COMPONENT(TextComponent, "Text");
    }

    void InspectorPanel::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();

        auto selected = m_Editor->GetSelected();

        if(ImGui::Begin(m_Name.c_str(), &m_Active))
        {
            if(!Application::Get().GetSceneManager()->GetCurrentScene())
            {
                m_Editor->SetSelected(entt::null);
                ImGui::End();
                return;
            }

            auto& registry = Application::Get().GetSceneManager()->GetCurrentScene()->GetRegistry();
            if(selected == entt::null || !registry.valid(selected))
            {
                m_Editor->SetSelected(entt::null);
                ImGui::End();
                return;
            }

            // active checkbox
            auto activeComponent = registry.try_get<ActiveComponent>(selected);
            bool active          = activeComponent ? activeComponent->active : true;
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

            if(m_DebugMode)
            {
                if(registry.valid(selected))
                {
                    // ImGui::Text("ID: %d, Version: %d", static_cast<int>(registry.entity(selected)), registry.version(selected));
                }
                else
                {
                    ImGui::TextUnformatted("INVALID ENTITY");
                }
            }

            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetFontSize() * 2.0f);
            {
                ImGuiUtilities::ScopedFont boldFont(ImGui::GetIO().Fonts->Fonts[1]);
                if(ImGuiUtilities::InputText(name))
                    registry.get_or_emplace<NameComponent>(selected).name = name;
            }
            ImGui::SameLine();

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

            ImGui::Separator();

            if(m_DebugMode)
            {
                auto idComponent = registry.try_get<IDComponent>(selected);

                ImGui::Text("UUID : %" PRIu64, (uint64_t)idComponent->ID);

                auto hierarchyComp = registry.try_get<Hierarchy>(selected);

                if(hierarchyComp)
                {
                    if(registry.valid(hierarchyComp->Parent()))
                    {
                        idComponent = registry.try_get<IDComponent>(hierarchyComp->Parent());
                        ImGui::Text("Parent : ID: %" PRIu64, (uint64_t)idComponent->ID);
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
                        idComponent = registry.try_get<IDComponent>(child);
                        ImGui::Text("ID: %" PRIu64, (uint64_t)idComponent->ID);

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
