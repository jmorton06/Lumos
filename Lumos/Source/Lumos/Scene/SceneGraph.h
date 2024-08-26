#pragma once
#include "Graphics/Camera/Camera.h"
#include "Graphics/Camera/CameraController.h"
#include "Graphics/Camera/ThirdPersonCamera.h"
#include "Graphics/Camera/Camera2D.h"
#include "Graphics/Camera/FPSCamera.h"
#include "Graphics/Camera/EditorCamera.h"

#include <entt/entity/fwd.hpp>
#include <cereal/cereal.hpp>

namespace Lumos
{

    class DefaultCameraController
    {
    public:
        enum class ControllerType : int
        {
            FPS = 0,
            ThirdPerson,
            Simple,
            Camera2D,
            EditorCamera,
            Custom
        };

        DefaultCameraController()
            : m_Type(ControllerType::Custom)
        {
        }

        DefaultCameraController(ControllerType type)
        {
            SetControllerType(type);
        }

        void SetControllerType(ControllerType type)
        {
            // if(type != m_Type)
            {
                m_Type = type;
                switch(type)
                {
                case ControllerType::ThirdPerson:
                    m_CameraController = CreateSharedPtr<ThirdPersonCameraController>();
                    break;
                case ControllerType::FPS:
                    m_CameraController = CreateSharedPtr<FPSCameraController>();
                    break;
                case ControllerType::Simple:
                    m_CameraController = CreateSharedPtr<FPSCameraController>();
                    break;
                case ControllerType::EditorCamera:
                    m_CameraController = CreateSharedPtr<EditorCameraController>();
                    break;
                case ControllerType::Camera2D:
                    m_CameraController = CreateSharedPtr<CameraController2D>();
                    break;
                case ControllerType::Custom:
                    m_CameraController = nullptr;
                    break;
                }
            }
        }

        static std::string CameraControllerTypeToString(ControllerType type)
        {
            switch(type)
            {
            case ControllerType::ThirdPerson:
                return "ThirdPerson";
            case ControllerType::FPS:
                return "FPS";
            case ControllerType::Simple:
                return "Simple";
            case ControllerType::EditorCamera:
                return "Editor";
            case ControllerType::Camera2D:
                return "2D";
            case ControllerType::Custom:
                return "Custom";
            }

            return "Custom";
        }

        static ControllerType StringToControllerType(const std::string& type)
        {
            if(type == "ThirdPerson")
                return ControllerType::ThirdPerson;
            if(type == "FPS")
                return ControllerType::FPS;
            if(type == "Simple")
                return ControllerType::Simple;
            if(type == "Editor")
                return ControllerType::EditorCamera;
            if(type == "2D")
                return ControllerType::Camera2D;
            if(type == "Custom")
                return ControllerType::Custom;

            LERROR("Unsupported Camera controller %s", type.c_str());
            return ControllerType::Custom;
        }

        const SharedPtr<CameraController>& GetController() const
        {
            return m_CameraController;
        }

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(cereal::make_nvp("ControllerType", m_Type));
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            archive(cereal::make_nvp("ControllerType", m_Type));
            SetControllerType(m_Type);
        }

        ControllerType GetType()
        {
            return m_Type;
        }

    private:
        ControllerType m_Type = ControllerType::Custom;
        SharedPtr<CameraController> m_CameraController;
    };

    struct NameComponent
    {
        template <typename Archive>
        void serialize(Archive& archive)
        {
            archive(cereal::make_nvp("Name", name));
        }
        std::string name = "";
    };

    struct ActiveComponent
    {
        ActiveComponent()
        {
            active = true;
        }

        ActiveComponent(bool act)
        {
            active = act;
        }

        template <typename Archive>
        void serialize(Archive& archive)
        {
            archive(cereal::make_nvp("Active", active));
        }

        bool active = true;
    };

    class Hierarchy
    {
    public:
        Hierarchy(entt::entity p);
        Hierarchy();

        inline entt::entity Parent() const
        {
            return m_Parent;
        }
        inline entt::entity Next() const
        {
            return m_Next;
        }
        inline entt::entity Prev() const
        {
            return m_Prev;
        }
        inline entt::entity First() const
        {
            return m_First;
        }

        // Return true if rhs is an ancestor of rhs
        bool Compare(const entt::registry& registry, const entt::entity rhs) const;
        void Reset();

        // update hierarchy components when hierarchy component is added
        static void OnConstruct(entt::registry& registry, entt::entity entity);

        // update hierarchy components when hierarchy component is removed
        static void OnDestroy(entt::registry& registry, entt::entity entity);
        static void OnUpdate(entt::registry& registry, entt::entity entity);
        static void Reparent(entt::entity entity, entt::entity parent, entt::registry& registry, Hierarchy& hierarchy);

        entt::entity m_Parent;
        entt::entity m_First;
        entt::entity m_Next;
        entt::entity m_Prev;

        template <typename Archive>
        void serialize(Archive& archive)
        {
            archive(cereal::make_nvp("First", m_First), cereal::make_nvp("Next", m_Next), cereal::make_nvp("Previous", m_Prev), cereal::make_nvp("Parent", m_Parent));
        }
    };

    class SceneGraph
    {
    public:
        SceneGraph();
        ~SceneGraph() = default;

        void Init(entt::registry& registry);

        void DisableOnConstruct(bool disable, entt::registry& registry);

        void Update(entt::registry& registry);
        void UpdateTransform(entt::entity entity, entt::registry& registry);
    };
}
