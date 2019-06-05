#pragma once
#include "LM.h"
#include "Maths/Frustum.h"
#include "Graphics/RenderList.h"
#include "Utilities/AssetManager.h"

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

namespace Lumos
{
	struct TimeStep;
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
		void AddEntity(std::shared_ptr<Entity>& game_object);

		// The friendly name associated with this scene instance
		const String& GetSceneName() const { return m_SceneName; }

		// The maximum bounds of the contained scene
		//   - This is exclusively used for shadowing purposes, ensuring all objects that could
		//     cast shadows are rendered as necessary.
		void  SetWorldRadius(float radius) { m_SceneBoundingRadius = radius; }
		float GetWorldRadius() const { return m_SceneBoundingRadius; }

		// Adds all visible objects to given RenderList
		void InsertToRenderList(RenderList* list, const Maths::Frustum& frustum) const;

		// Updates all world transforms in the Scene Tree
		virtual void BuildWorldMatrices();
		void DebugRender();

		void BuildFrameRenderList();
		void BuildLightList();

		std::vector<std::shared_ptr<Graphics::Light>>& GetLightList() { return m_LightList; }

		std::shared_ptr<Entity>& GetRootEntity() { return m_RootEntity; }

		void 				SetCamera(Camera* camera) { m_pCamera = camera; }
		Camera*				GetCamera()				const { return m_pCamera; }
		Graphics::TextureCube* GetEnvironmentMap()		const { return m_EnvironmentMap; }

		bool GetReflectSkybox() const { return m_ReflectSkybox; }
		void SetReflectSkybox(bool reflect) { m_ReflectSkybox = reflect; }

		bool GetDrawDebugData() const { return m_DrawDebugData; }
		void SetDrawDebugData(bool draw) { m_DrawDebugData = draw; }

		uint64_t GetDebugDrawFlags() const { return m_DebugDrawFlags; }
		void SetDebugDrawFlags(uint64_t flags) { m_DebugDrawFlags = flags; }

		void SetScreenWidth(uint width)   { m_ScreenWidth = width; }
		void SetScreenHeight(uint height) { m_ScreenHeight = height; }
        
        uint GetScreenWidth() const { return m_ScreenWidth; }
        uint GetScreenHeight() const { return m_ScreenHeight; }

		Maths::Frustum GetFrustum() const { return m_FrameFrustum; }
		RenderList* GetRenderList() const { return m_pFrameRenderList.get(); }

		void IterateEntities(const std::function<void(std::shared_ptr<Entity>)>& per_object_func);

	protected:

		String m_SceneName;
		Camera* m_pCamera; 
		Graphics::TextureCube* m_EnvironmentMap;

		float m_SceneBoundingRadius;

		std::shared_ptr<Entity> m_RootEntity;

		bool m_CurrentScene = false;
		bool m_ReflectSkybox = true;

		bool	m_DrawDebugData{};
        uint64	m_DebugDrawFlags{};

		uint m_ScreenWidth;
		uint m_ScreenHeight;

		Maths::Frustum m_FrameFrustum;
		std::unique_ptr<RenderList>	m_pFrameRenderList;
		std::vector<std::shared_ptr<Graphics::Light>> m_LightList;

		std::vector<Layer*> m_SceneLayers;

    private:
        Scene(Scene const&) = delete;
        Scene& operator=(Scene const&) = delete;

		bool OnWindowResize(WindowResizeEvent& e);
	};
}
