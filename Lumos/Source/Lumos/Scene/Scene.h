#pragma once
#include "Maths/Maths.h"
#include "Utilities/AssetManager.h"

#include <sol/forward.hpp>
#include <cereal/cereal.hpp>
DISABLE_WARNING_PUSH
DISABLE_WARNING_CONVERSION_TO_SMALLER_TYPE
#include <entt/entity/registry.hpp>
DISABLE_WARNING_POP

namespace Lumos
{
    class TimeStep;
    class Font;
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

        EntityManager* GetEntityManager() { return m_EntityManager.get(); }

        virtual void Serialise(const std::string& filePath, bool binary = false);
        virtual void Deserialise(const std::string& filePath, bool binary = false);

        struct SceneRenderSettings
        {
            bool Renderer2DEnabled = true;
            bool Renderer3DEnabled = true;
            bool DebugRenderEnabled = true;
            bool SkyboxRenderEnabled = true;
            bool ShadowsEnabled = true;
            bool FXAAEnabled = true;
            bool DebandingEnabled = true;
            bool ChromaticAberationEnabled = true;
            bool EyeAdaptation = true;
            bool SSAOEnabled = true;

            // Shadow Settings
            float m_CascadeSplitLambda = 0.92f;
            float m_SceneRadiusMultiplier = 1.4f;

            float m_LightSize = 1.5f;
            float m_MaxShadowDistance = 400.0f;
            float m_ShadowFade = 40.0f;
            float m_CascadeTransitionFade = 3.0f;
            float m_InitialBias = 0.0023f;
            uint32_t m_ShadowMapNum = 4;
            uint32_t m_ShadowMapSize = 4096;

            float m_Exposure = 1.0f;
            uint32_t m_ToneMapIndex = 4;
        };

        struct ScenePhysics3DSettings
        {
            uint32_t m_MaxUpdatesPerFrame = 5;
            uint32_t VelocityIterations = 20;
            uint32_t PositionIterations = 1;

            glm::vec3 Gravity = glm::vec3(0.0f, -9.81f, 0.0f);
            float Dampening = 0.999f;
            uint32_t IntegrationTypeIndex = 3;
            uint32_t BroadPhaseTypeIndex = 2;
        };

        struct ScenePhysics2DSettings
        {
            uint32_t VelocityIterations = 6;
            uint32_t PositionIterations = 2;

            float Gravity = -9.81f;
            float Dampening = 0.999f;
        };

        struct SceneSettings
        {
            bool PhysicsEnabled2D = true;
            bool PhysicsEnabled3D = true;
            bool AudioEnabled = true;

            SceneRenderSettings RenderSettings;
            ScenePhysics3DSettings Physics3DSettings;
            ScenePhysics2DSettings Physics2DSettings;
        };

#define SceneVersion 9

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(cereal::make_nvp("Version", SceneVersion));
            archive(cereal::make_nvp("Scene Name", m_SceneName));

            archive(cereal::make_nvp("PhysicsEnabled2D", m_Settings.PhysicsEnabled2D), cereal::make_nvp("PhysicsEnabled3D", m_Settings.PhysicsEnabled3D), cereal::make_nvp("AudioEnabled", m_Settings.AudioEnabled), cereal::make_nvp("Renderer2DEnabled", m_Settings.RenderSettings.Renderer2DEnabled),
                cereal::make_nvp("Renderer3DEnabled", m_Settings.RenderSettings.Renderer3DEnabled), cereal::make_nvp("DebugRenderEnabled", m_Settings.RenderSettings.DebugRenderEnabled), cereal::make_nvp("SkyboxRenderEnabled", m_Settings.RenderSettings.SkyboxRenderEnabled), cereal::make_nvp("ShadowsEnabled", m_Settings.RenderSettings.ShadowsEnabled));
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            archive(cereal::make_nvp("Version", m_SceneSerialisationVersion));
            archive(cereal::make_nvp("Scene Name", m_SceneName));

            if(m_SceneSerialisationVersion > 7)
                archive(cereal::make_nvp("PhysicsEnabled2D", m_Settings.PhysicsEnabled2D), cereal::make_nvp("PhysicsEnabled3D", m_Settings.PhysicsEnabled3D), cereal::make_nvp("AudioEnabled", m_Settings.AudioEnabled), cereal::make_nvp("Renderer2DEnabled", m_Settings.RenderSettings.Renderer2DEnabled),
                    cereal::make_nvp("Renderer3DEnabled", m_Settings.RenderSettings.Renderer3DEnabled), cereal::make_nvp("DebugRenderEnabled", m_Settings.RenderSettings.DebugRenderEnabled), cereal::make_nvp("SkyboxRenderEnabled", m_Settings.RenderSettings.SkyboxRenderEnabled), cereal::make_nvp("ShadowsEnabled", m_Settings.RenderSettings.ShadowsEnabled));
        }

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

    private:
        NONCOPYABLE(Scene)

        bool OnWindowResize(WindowResizeEvent& e);

        friend class Entity;
    };
}
