#pragma once
#include <sol/forward.hpp>
#include <glm/ext/vector_float3.hpp>
#include <entt/fwd.hpp>
#include "Core/DataStructures/Vector.h"
#include "Core/UUID.h"

namespace Lumos
{
    class TimeStep;
    class Event;
    class Camera;
    class EntityManager;
    class Entity;
    class SceneGraph;
    class Event;
    class WindowResizeEvent;

    namespace Graphics
    {
        struct Light;
        class GBuffer;
        class Material;
    }

    class LUMOS_EXPORT Scene
    {
        template <typename Archive>
        friend void save(Archive& archive, const Scene& scene);

        template <typename Archive>
        friend void load(Archive& archive, Scene& scene);

    public:
        explicit Scene(const std::string& name);
        virtual ~Scene();

        // Called when scene is being activated, and will begin being rendered/updated.
        //	 - Initialise objects/physics here
        virtual void OnInit();

        // Called when scene is being swapped and will no longer be rendered/updated
        //	 - Remove objects/physics here
        //	   Note: Default action here automatically delete all game objects
        virtual void OnCleanupScene();

        virtual void Render3D()
        {
        }
        virtual void Render2D()
        {
        }

        // Update Scene Logic
        //   - Called once per frame and should contain all time-sensitive update logic
        //	   Note: This is time relative to seconds not milliseconds! (e.g. msec / 1000)
        virtual void OnUpdate(const TimeStep& timeStep);
        virtual void OnImGui() {};
        virtual void OnEvent(Event& e);
        // Delete all contained Objects
        //    - This is the default action upon firing OnCleanupScene()
        void DeleteAllGameObjects();

        // The friendly name associated with this scene instance
        const std::string& GetSceneName() const
        {
            return m_SceneName;
        }

        void SetName(const std::string& name)
        {
            m_SceneName = name;
        }

        void SetScreenWidth(uint32_t width)
        {
            m_ScreenWidth = width;
        }
        void SetScreenHeight(uint32_t height)
        {
            m_ScreenHeight = height;
        }

        void SetScreenSize(uint32_t width, uint32_t height);

        uint32_t GetScreenWidth() const
        {
            return m_ScreenWidth;
        }

        uint32_t GetScreenHeight() const
        {
            return m_ScreenHeight;
        }

        entt::registry& GetRegistry();

        void UpdateSceneGraph();
        void DuplicateEntity(Entity entity);
        void DuplicateEntity(Entity entity, Entity parent);
        Entity CreateEntity();
        Entity CreateEntity(const std::string& name);
        Entity GetEntityByUUID(uint64_t id);
        bool EntityExists(u64 id);
        Entity InstantiatePrefab(const std::string& path);
        void DestroyEntity(Entity entity);
        void SavePrefab(Entity entity, const std::string& path);

        EntityManager* GetEntityManager() { return m_EntityManager.get(); }

        virtual void Serialise(const std::string& filePath, bool binary = false);
        virtual void Deserialise(const std::string& filePath, bool binary = false);

        struct SceneRenderSettings
        {
            bool Renderer2DEnabled         = true;
            bool Renderer3DEnabled         = true;
            bool DebugRenderEnabled        = true;
            bool SkyboxRenderEnabled       = true;
            bool ShadowsEnabled            = true;
            bool FXAAEnabled               = true;
            bool DebandingEnabled          = true;
            bool ChromaticAberationEnabled = false;
            bool EyeAdaptation             = false;
            bool SSAOEnabled               = false;
            bool BloomEnabled              = true;
            bool FilmicGrainEnabled        = false;
            bool MotionBlurEnabled         = false;
            bool DepthOfFieldEnabled       = false;
            bool SharpenEnabled            = false;
            bool DepthPrePass              = true;
            float DepthOfFieldStrength     = 1.0f;
            float DepthOfFieldDistance     = 100.0f;

            // Shadow Settings
            float m_CascadeSplitLambda = 0.92f;
            float m_LightSize          = 1.5f;
            float m_MaxShadowDistance  = 400.0f;
            float m_ShadowFade         = 40.0f;
            float m_CascadeFade        = 3.0f;
            float m_InitialBias        = 0.0023f;
            uint32_t m_ShadowMapNum    = 4;
            uint32_t m_ShadowMapSize   = 2048;

            float m_Exposure        = 1.0f;
            uint32_t m_ToneMapIndex = 6;
            float Brightness        = 0.0f;
            float Saturation        = 1.0f;
            float Contrast          = 1.0f;

            // Bloom
            float m_BloomIntensity   = 1.0f;
            float BloomThreshold     = 1.0f;
            float BloomKnee          = 0.1f;
            float BloomUpsampleScale = 1.0f;

            // SSAO
            int SSAOBlurRadius     = 4;
            float SSAOSampleRadius = 2.0f;
            bool SSAOBlur          = true;
            float SSAOStrength     = 1.0f;

            float SkyboxMipLevel = 0.0f;
            int DebugMode        = 0;
#ifdef LUMOS_PLATFORM_WINDOWS
            uint8_t MSAASamples = 4;
#else
            uint8_t MSAASamples = 1;
#endif
        };

        struct ScenePhysics3DSettings
        {
            uint32_t MaxUpdatesPerFrame = 5;
            uint32_t VelocityIterations = 6;
            uint32_t PositionIterations = 2;

            glm::vec3 Gravity             = glm::vec3(0.0f, -9.81f, 0.0f);
            float Dampening               = 0.9995f;
            uint32_t IntegrationTypeIndex = 3;
            uint32_t BroadPhaseTypeIndex  = 2;
        };

        struct ScenePhysics2DSettings
        {
            uint32_t VelocityIterations = 6;
            uint32_t PositionIterations = 2;

            float Gravity   = -9.81f;
            float Dampening = 0.9995f;
        };

        struct SceneSettings
        {
            bool PhysicsEnabled2D = true;
            bool PhysicsEnabled3D = true;
            bool AudioEnabled     = true;

            SceneRenderSettings RenderSettings;
            ScenePhysics3DSettings Physics3DSettings;
            ScenePhysics2DSettings Physics2DSettings;
        };

        SceneSettings& GetSettings()
        {
            return m_Settings;
        }

        int GetSceneVersion() const
        {
            return m_SceneSerialisationVersion;
        }

    protected:
        std::string m_SceneName;
        int m_SceneSerialisationVersion = 0;
        SceneSettings m_Settings;

        UniquePtr<EntityManager> m_EntityManager;
        UniquePtr<SceneGraph> m_SceneGraph;

        uint32_t m_ScreenWidth;
        uint32_t m_ScreenHeight;

        // Load these assets ready to be used during a scene
        Vector<UUID> m_PreLoadAssetsList;

    private:
        NONCOPYABLE(Scene)

        bool OnWindowResize(WindowResizeEvent& e);

        friend class Entity;
    };
}
