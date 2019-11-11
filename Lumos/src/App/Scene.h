#pragma once
#include "lmpch.h"
#include "SceneGraph.h"
#include "Maths/Frustum.h"
#include "Utilities/AssetManager.h"

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

#include "Core/Serialisable.h"

#include <entt/entt.hpp>

namespace Lumos
{
	class TimeStep;
	class Font;
	class Material;
	class Event;
	class Layer;
	class Camera;

	namespace Graphics
	{
		struct Light;
		class GBuffer;
		class TextureCube;
	}

	class LUMOS_EXPORT Scene : public Serialisable
	{
	public:
		explicit Scene(const String& SceneName); //Called once at program start - all scene initialization should be done in 'OnInitialize'
		virtual ~Scene();

		// Called when scene is being activated, and will begin being rendered/updated.
		//	 - Initialize objects/physics here
		virtual void OnInit();

		// Called when scene is being swapped and will no longer be rendered/updated
		//	 - Remove objects/physics here
		//	   Note: Default action here automatically delete all game objects
		virtual void OnCleanupScene();

		virtual void Render3D() { }
		virtual void Render2D() { }

		// Update Scene Logic
		//   - Called once per frame and should contain all time-sensitive update logic
		//	   Note: This is time relative to seconds not milliseconds! (e.g. msec / 1000)
		virtual void OnUpdate(TimeStep* timeStep);
		virtual void OnTick() { };
		virtual void OnImGui() { };
		virtual void OnEvent(Event& e);
		// Delete all contained Objects
		//    - This is the default action upon firing OnCleanupScene()
		void DeleteAllGameObjects();

		// The friendly name associated with this scene instance
		const String& GetSceneName() const { return m_SceneName; }

		// The maximum bounds of the contained scene
		//   - This is exclusively used for shadowing purposes, ensuring all objects that could
		//     cast shadows are rendered as necessary.
		void  SetWorldRadius(float radius) { m_SceneBoundingRadius = radius; }
		float GetWorldRadius() const { return m_SceneBoundingRadius; }

		//Entity* GetRootEntity() { return m_RootEntity; }

		void SetCamera(Camera* camera) { m_pCamera = camera; }
		Camera* GetCamera()	const { return m_pCamera; }
		Graphics::TextureCube* GetEnvironmentMap() const { return m_EnvironmentMap; }

		bool GetReflectSkybox() const { return m_ReflectSkybox; }
		void SetReflectSkybox(bool reflect) { m_ReflectSkybox = reflect; }

		void SetScreenWidth(u32 width)   { m_ScreenWidth = width; }
		void SetScreenHeight(u32 height) { m_ScreenHeight = height; }
        
        u32 GetScreenWidth() const { return m_ScreenWidth; }
		u32 GetScreenHeight() const { return m_ScreenHeight; }

		// Inherited via Serialisable
		nlohmann::json Serialise() override;
		void Deserialise(nlohmann::json & data) override;
        
        const entt::registry& GetRegistry() const { return m_Registry; }
        entt::registry& GetRegistry() { return m_Registry; }

	protected:

		String m_SceneName;
		Camera* m_pCamera; 
		Graphics::TextureCube* m_EnvironmentMap;

		float m_SceneBoundingRadius;
        
        entt::registry m_Registry;

		bool m_CurrentScene = false;
		bool m_ReflectSkybox = true;

		u32 m_ScreenWidth;
		u32 m_ScreenHeight;

		SceneGraph m_SceneGraph;

    private:
		NONCOPYABLE(Scene)

		bool OnWindowResize(WindowResizeEvent& e);
};
}
