#pragma once
#include "lmpch.h"
#include "SceneGraph.h"
#include "Maths/Maths.h"
#include "Utilities/AssetManager.h"

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

#include "Core/Serialisable.h"

#include <sol/forward.hpp>
#include <entt/entity/registry.hpp>

namespace Lumos
{
	class TimeStep;
	class Font;
	class Material;
	class Event;
	class Layer;
	class Camera;
    class LayerStack;

	namespace Graphics
	{
		struct Light;
		class GBuffer;
	}

	class LUMOS_EXPORT Scene
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
		virtual void OnUpdate(const TimeStep& timeStep);
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

		void SetScreenWidth(u32 width)   { m_ScreenWidth = width; }
		void SetScreenHeight(u32 height) { m_ScreenHeight = height; }
        
        u32 GetScreenWidth() const { return m_ScreenWidth; }
		u32 GetScreenHeight() const { return m_ScreenHeight; }
        
        //const entt::registry& GetRegistry() const { return m_Registry; }
        entt::registry& GetRegistry() { return m_Registry; }
        
        void LoadLuaScene(const String& filePath);
        
        static Scene* LoadFromLua(const String& filePath);
    
        LayerStack* GetLayers() const { return m_LayerStack; }
    
        void PushLayer(Layer* layer, bool overlay = false);
    
        void Serialise(const String& filePath);
        void Deserialise(const String& filePath);
    
        template<typename Archive>
        void serialize(Archive &archive)
        {
            archive(cereal::make_nvp("Bounding Radius", m_SceneBoundingRadius), cereal::make_nvp("Scene Name", m_SceneName));
        }

	protected:

		String m_SceneName;
		float m_SceneBoundingRadius;
        
        entt::registry m_Registry;

		bool m_CurrentScene = false;

		u32 m_ScreenWidth;
		u32 m_ScreenHeight;

		SceneGraph m_SceneGraph;
    
        LayerStack* m_LayerStack = nullptr;

    private:
		NONCOPYABLE(Scene)

		bool OnWindowResize(WindowResizeEvent& e);
        Ref<sol::environment> m_LuaEnv;
        Ref<sol::protected_function> m_LuaUpdateFunction;
        
        String m_LuaFilePath;

	friend class Entity;
};
}
