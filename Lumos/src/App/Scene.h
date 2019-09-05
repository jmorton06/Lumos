#pragma once
#include "LM.h"
#include "Maths/Frustum.h"
#include "Graphics/RenderList.h"
#include "Utilities/AssetManager.h"

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

#include "Core/Serialisable.h"

namespace Lumos
{
	class TimeStep;
	class Font;
	class Material;
	class Event;
	class Layer;
	class Camera;
	class Entity;

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
		virtual void OnIMGUI() { };
		virtual void OnEvent(Event& e);
		// Delete all contained Objects
		//    - This is the default action upon firing OnCleanupScene()
		void DeleteAllGameObjects();

		// Add Entity to the scene list
		//    - All added Entities are managed by the scene itself, firing
		//		OnRender and OnUpdate functions automatically
		void AddEntity(Entity* game_object);

		// The friendly name associated with this scene instance
		const String& GetSceneName() const { return m_SceneName; }

		// The maximum bounds of the contained scene
		//   - This is exclusively used for shadowing purposes, ensuring all objects that could
		//     cast shadows are rendered as necessary.
		void  SetWorldRadius(float radius) { m_SceneBoundingRadius = radius; }
		float GetWorldRadius() const { return m_SceneBoundingRadius; }

		// Adds all visible objects to given RenderList
		void InsertToRenderList(RenderList* list, const Maths::Frustum& frustum) const;

		void BuildFrameRenderList();

		Entity* GetRootEntity() { return m_RootEntity; }

		void SetCamera(Camera* camera) { m_pCamera = camera; }
		Camera* GetCamera()	const { return m_pCamera; }
		Graphics::TextureCube* GetEnvironmentMap() const { return m_EnvironmentMap; }

		bool GetReflectSkybox() const { return m_ReflectSkybox; }
		void SetReflectSkybox(bool reflect) { m_ReflectSkybox = reflect; }

		void SetScreenWidth(u32 width)   { m_ScreenWidth = width; }
		void SetScreenHeight(u32 height) { m_ScreenHeight = height; }
        
        u32 GetScreenWidth() const { return m_ScreenWidth; }
        u32 GetScreenHeight() const { return m_ScreenHeight; }

		Maths::Frustum GetFrustum() const { return m_FrameFrustum; }
		RenderList* GetRenderList() const { return m_pFrameRenderList.get(); }

		void IterateEntities(const std::function<void(Entity*)>& per_object_func);

		// Inherited via Serialisable
		nlohmann::json Serialise() override;
		void Deserialise(nlohmann::json & data) override;

	protected:

		String m_SceneName;
		Camera* m_pCamera; 
		Graphics::TextureCube* m_EnvironmentMap;

		float m_SceneBoundingRadius;

		Entity* m_RootEntity;

		bool m_CurrentScene = false;
		bool m_ReflectSkybox = true;

		u32 m_ScreenWidth;
		u32 m_ScreenHeight;

		Maths::Frustum m_FrameFrustum;
		Scope<RenderList>	m_pFrameRenderList;

    private:
		NONCOPYABLE(Scene)

		bool OnWindowResize(WindowResizeEvent& e);
};
}
