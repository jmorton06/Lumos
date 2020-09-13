#pragma once
#include "Graphics/Camera/Camera.h"
#include "Graphics/Camera/CameraController.h"
#include "Graphics/Camera/ThirdPersonCamera.h"
#include "Graphics/Camera/Camera2D.h"
#include "Graphics/Camera/FPSCamera.h"
#include "Editor/EditorCamera.h"

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
            //if(type != m_Type)
			{
				m_Type = type;
				switch(type)
				{
				case ControllerType::ThirdPerson:
					m_CameraController = CreateRef<ThirdPersonCameraController>();
					break;
				case ControllerType::FPS:
					m_CameraController = CreateRef<FPSCameraController>();
					break;
				case ControllerType::Simple:
					m_CameraController = CreateRef<FPSCameraController>();
					break;
				case ControllerType::EditorCamera:
					m_CameraController = CreateRef<EditorCameraController>();
					break;
				case ControllerType::Camera2D:
					m_CameraController = CreateRef<CameraController2D>();
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

			LUMOS_LOG_ERROR("Unsupported Camera controller {0}", type);
			return ControllerType::Custom;
		}

		const Ref<CameraController> & GetController() const 
		{
			return m_CameraController;
		}

		template<typename Archive>
		void save(Archive& archive) const
		{
			archive(cereal::make_nvp("ControllerType", m_Type));
		}

		template<typename Archive>
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
		Ref<CameraController> m_CameraController;
	};

	struct NameComponent
	{
		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(cereal::make_nvp("Name", name));
		}
		std::string name;
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

		template<typename Archive>
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

		inline entt::entity parent() const
		{
			return _parent;
		}
		inline entt::entity next() const
		{
			return _next;
		}
		inline entt::entity prev() const
		{
			return _prev;
		}
		inline entt::entity first() const
		{
			return _first;
		}

		// Return true if rhs is an ancestor of rhs
		bool compare(const entt::registry& registry, const entt::entity rhs) const;

		// update hierarchy components when hierarchy component is added
		static void on_construct(entt::registry& registry, entt::entity entity);

		// update hierarchy components when hierarchy component is removed
		static void on_destroy(entt::registry& registry, entt::entity entity);

		static void on_update(entt::registry& registry, entt::entity entity);

		static void Reparent(entt::entity entity, entt::entity parent, entt::registry& registry, Hierarchy& hierarchy);

		entt::entity _parent;
		entt::entity _first;
		entt::entity _next;
		entt::entity _prev;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(cereal::make_nvp("First", _first), cereal::make_nvp("Next", _next), cereal::make_nvp("Previous", _prev), cereal::make_nvp("Parent", _parent));
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
