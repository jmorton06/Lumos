#pragma once
#include "LM.h"
#include "Maths/Frustum.h"
#include "Physics/JMPhysicsEngine/JMPhysicsEngine.h"
#include "Utilities/TSingleton.h"
#include "Graphics/RenderList.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/LightSetUp.h"
#include "Entity/Entity.h"
#include "Utilities/AssetManager.h"

namespace Lumos
{
	class ParticleManager;
	class Font;
	class GBuffer;
	class TextureCube;
	class Material;

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
		// Delete all contained Objects
		//    - This is the default action upon firing OnCleanupScene()
		void DeleteAllGameObjects();

		// Add Entity to the scene list
		//    - All added Entities are managed by the scene itself, firing
		//		OnRender and OnUpdate functions automatically
		void AddEntity(std::shared_ptr<Entity> game_object);

		// The friendly name associated with this scene instance
		const String& GetSceneName() const { return m_SceneName; }

		// The maximum bounds of the contained scene
		//   - This is exclusively used for shadowing purposes, ensuring all objects that could
		//     cast shadows are rendered as necessary.
		void  SetWorldRadius(float radius) { m_SceneBoundingRadius = radius; }
		float GetWorldRadius() const { return m_SceneBoundingRadius; }

		// Adds all visible objects to given RenderList
		void InsertToRenderList(RenderList* list, const maths::Frustum& frustum) const;

		// Updates all world transforms in the Scene Tree
		virtual void BuildWorldMatrices();
		void DebugRender();

		void AddPointLight(std::shared_ptr<Light> light) const;

		void RenderString(const String& text, const maths::Vector2& pos, float scale, const maths::Vector4& colour) const;

		std::vector <std::shared_ptr<Entity>>& GetEntities() { return m_Entities; }

		void 				SetCamera(Camera* camera) { m_pCamera = camera; }
		Camera*				GetCamera()				const { return m_pCamera; }
		ParticleManager*	GetParticleSystem()		const { return m_ParticleManager; }
		LightSetup*			GetLightSetup()			const { return m_LightSetup; }
		TextureCube*		GetEnvironmentMap()		const { return m_EnvironmentMap; }

		bool GetReflectSkybox() const { return m_ReflectSkybox; }
		void SetReflectSkybox(bool reflect) { m_ReflectSkybox = reflect; }

		bool GetDrawDebugData() const { return m_DrawDebugData; }
		void SetDrawDebugData(bool draw) { m_DrawDebugData = draw; }

		uint64_t GetDebugDrawFlags() const { return m_DebugDrawFlags; }
		void SetDebugDrawFlags(uint64_t flags) { m_DebugDrawFlags = flags; }

		bool GetUseShadow() const { return m_UseShadow; }
		void SetUseShadow(bool set) { m_UseShadow = set; }

		void SetDrawObjects(bool set) { m_DrawObjects = set; }
		bool GetDrawObjects() const { return m_DrawObjects; }

		void ToggleDrawObjects() { m_DrawObjects = !m_DrawObjects; }
		bool GetReflectingScene() const { return m_ReflectScene; }
		const maths::Vector3& GetBackgroundColor() const   { return m_BackgroundColour; }
		void SetBackgroundColor(const maths::Vector3& col) { m_BackgroundColour = col; }

		void SetScreenWidth(uint width)   { m_ScreenWidth = width; }
		void SetScreenHeight(uint height) { m_ScreenHeight = height; }

	protected:

		String				m_SceneName;
		Camera*				m_pCamera;
		ParticleManager*	m_ParticleManager;
		LightSetup*			m_LightSetup;
		TextureCube*		m_EnvironmentMap;

		AssetManager<Material>* m_MaterialManager;

		float m_SceneBoundingRadius;

		std::vector<std::shared_ptr<Entity>> m_Entities;

		bool m_CurrentScene = false;
		bool m_ReflectSkybox = true;

		bool	m_DrawDebugData{};
		uint64	m_DebugDrawFlags{};

		bool m_DrawObjects;
		bool m_ReflectScene;
		bool m_UseShadow;
		maths::Vector3	m_BackgroundColour;

		uint m_ScreenWidth;
		uint m_ScreenHeight;
    private:
        Scene(Scene const&) = delete;
        Scene& operator=(Scene const&) = delete;
	};
}
