#pragma once
#include "Maths/Maths.h"
#include "Utilities/AssetManager.h"
#include "Serialisation.h"
#include <sol/forward.hpp>

DISABLE_WARNING_PUSH
DISABLE_WARNING_CONVERSION_TO_SMALLER_TYPE
#include <entt/entity/registry.hpp>
DISABLE_WARNING_POP

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
            bool Renderer2DEnabled         = true;
            bool Renderer3DEnabled         = true;
            bool DebugRenderEnabled        = true;
            bool SkyboxRenderEnabled       = true;
            bool ShadowsEnabled            = true;
            bool FXAAEnabled               = true;
            bool DebandingEnabled          = true;
            bool ChromaticAberationEnabled = false;
            bool EyeAdaptation             = true;
            bool SSAOEnabled               = true;
            bool BloomEnabled              = true;
            bool FilmicGrainEnabled        = false;
            bool MotionBlurEnabled         = false;
            bool DepthOfFieldEnabled       = false;
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

            // Bloom
            float m_BloomIntensity   = 1.0f;
            float BloomThreshold     = 1.0f;
            float BloomKnee          = 0.1f;
            float BloomUpsampleScale = 1.0f;
        };

        struct ScenePhysics3DSettings
        {
            uint32_t m_MaxUpdatesPerFrame = 5;
            uint32_t VelocityIterations   = 20;
            uint32_t PositionIterations   = 1;

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

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(cereal::make_nvp("Version", SceneSerialisationVersion));
            archive(cereal::make_nvp("Scene Name", m_SceneName));

            archive(cereal::make_nvp("PhysicsEnabled2D", m_Settings.PhysicsEnabled2D), cereal::make_nvp("PhysicsEnabled3D", m_Settings.PhysicsEnabled3D), cereal::make_nvp("AudioEnabled", m_Settings.AudioEnabled), cereal::make_nvp("Renderer2DEnabled", m_Settings.RenderSettings.Renderer2DEnabled),
                    cereal::make_nvp("Renderer3DEnabled", m_Settings.RenderSettings.Renderer3DEnabled), cereal::make_nvp("DebugRenderEnabled", m_Settings.RenderSettings.DebugRenderEnabled), cereal::make_nvp("SkyboxRenderEnabled", m_Settings.RenderSettings.SkyboxRenderEnabled), cereal::make_nvp("ShadowsEnabled", m_Settings.RenderSettings.ShadowsEnabled));
            archive(cereal::make_nvp("Exposure", m_Settings.RenderSettings.m_Exposure), cereal::make_nvp("ToneMap", m_Settings.RenderSettings.m_ToneMapIndex));

            archive(cereal::make_nvp("BloomIntensity", m_Settings.RenderSettings.m_BloomIntensity), cereal::make_nvp("BloomKnee", m_Settings.RenderSettings.BloomKnee), cereal::make_nvp("BloomThreshold", m_Settings.RenderSettings.BloomThreshold),
                    cereal::make_nvp("BloomUpsampleScale", m_Settings.RenderSettings.BloomUpsampleScale));

            archive(cereal::make_nvp("FXAAEnabled", m_Settings.RenderSettings.FXAAEnabled), cereal::make_nvp("DebandingEnabled", m_Settings.RenderSettings.DebandingEnabled), cereal::make_nvp("ChromaticAberationEnabled", m_Settings.RenderSettings.ChromaticAberationEnabled), cereal::make_nvp("EyeAdaptation", m_Settings.RenderSettings.EyeAdaptation), cereal::make_nvp("SSAOEnabled", m_Settings.RenderSettings.SSAOEnabled), cereal::make_nvp("BloomEnabled", m_Settings.RenderSettings.BloomEnabled), cereal::make_nvp("FilmicGrainEnabled", m_Settings.RenderSettings.FilmicGrainEnabled), cereal::make_nvp("DepthOfFieldEnabled", m_Settings.RenderSettings.DepthOfFieldEnabled), cereal::make_nvp("MotionBlurEnabled", m_Settings.RenderSettings.MotionBlurEnabled));

            archive(cereal::make_nvp("DepthOFFieldEnabled", m_Settings.RenderSettings.DepthOfFieldEnabled), cereal::make_nvp("DepthOfFieldStrength", m_Settings.RenderSettings.DepthOfFieldStrength), cereal::make_nvp("DepthOfFieldDistance", m_Settings.RenderSettings.DepthOfFieldDistance));
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            archive(cereal::make_nvp("Version", m_SceneSerialisationVersion));
            archive(cereal::make_nvp("Scene Name", m_SceneName));

            Serialisation::CurrentSceneVersion = m_SceneSerialisationVersion;

            if(m_SceneSerialisationVersion > 7)
            {
                archive(cereal::make_nvp("PhysicsEnabled2D", m_Settings.PhysicsEnabled2D), cereal::make_nvp("PhysicsEnabled3D", m_Settings.PhysicsEnabled3D), cereal::make_nvp("AudioEnabled", m_Settings.AudioEnabled), cereal::make_nvp("Renderer2DEnabled", m_Settings.RenderSettings.Renderer2DEnabled),
                        cereal::make_nvp("Renderer3DEnabled", m_Settings.RenderSettings.Renderer3DEnabled), cereal::make_nvp("DebugRenderEnabled", m_Settings.RenderSettings.DebugRenderEnabled), cereal::make_nvp("SkyboxRenderEnabled", m_Settings.RenderSettings.SkyboxRenderEnabled), cereal::make_nvp("ShadowsEnabled", m_Settings.RenderSettings.ShadowsEnabled));
            }
            if(m_SceneSerialisationVersion > 9)
            {
                archive(cereal::make_nvp("Exposure", m_Settings.RenderSettings.m_Exposure), cereal::make_nvp("ToneMap", m_Settings.RenderSettings.m_ToneMapIndex));
            }

            if(Serialisation::CurrentSceneVersion > 11)
            {
                archive(cereal::make_nvp("BloomIntensity", m_Settings.RenderSettings.m_BloomIntensity), cereal::make_nvp("BloomKnee", m_Settings.RenderSettings.BloomKnee), cereal::make_nvp("BloomThreshold", m_Settings.RenderSettings.BloomThreshold),
                        cereal::make_nvp("BloomUpsampleScale", m_Settings.RenderSettings.BloomUpsampleScale));
            }
            if(Serialisation::CurrentSceneVersion > 12)
            {
                archive(cereal::make_nvp("FXAAEnabled", m_Settings.RenderSettings.FXAAEnabled), cereal::make_nvp("DebandingEnabled", m_Settings.RenderSettings.DebandingEnabled), cereal::make_nvp("ChromaticAberationEnabled", m_Settings.RenderSettings.ChromaticAberationEnabled), cereal::make_nvp("EyeAdaptation", m_Settings.RenderSettings.EyeAdaptation), cereal::make_nvp("SSAOEnabled", m_Settings.RenderSettings.SSAOEnabled), cereal::make_nvp("BloomEnabled", m_Settings.RenderSettings.BloomEnabled), cereal::make_nvp("FilmicGrainEnabled", m_Settings.RenderSettings.FilmicGrainEnabled), cereal::make_nvp("DepthOfFieldEnabled", m_Settings.RenderSettings.DepthOfFieldEnabled), cereal::make_nvp("MotionBlurEnabled", m_Settings.RenderSettings.MotionBlurEnabled));
            }

            if(Serialisation::CurrentSceneVersion > 15)
            {
                archive(cereal::make_nvp("DepthOfFieldEnabled", m_Settings.RenderSettings.DepthOfFieldEnabled), cereal::make_nvp("DepthOfFieldStrength", m_Settings.RenderSettings.DepthOfFieldStrength), cereal::make_nvp("DepthOfFieldDistance", m_Settings.RenderSettings.DepthOfFieldDistance));
            }
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
