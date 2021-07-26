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

        EntityManager* GetEntityManager() { return m_EntityManager.get(); }

        void SetHasCppClass(bool value)
        {
            m_HasCppClass = value;
        }

        bool GetHasCppClass() const
        {
            return m_HasCppClass;
        }

        virtual void Serialise(const std::string& filePath, bool binary = false);
        virtual void Deserialise(const std::string& filePath, bool binary = false);

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(cereal::make_nvp("Version", 5));
            archive(cereal::make_nvp("Scene Name", m_SceneName));
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            archive(cereal::make_nvp("Version", m_SceneSerialisationVersion));
            archive(cereal::make_nvp("Scene Name", m_SceneName));
        }

    protected:
        std::string m_SceneName;
        int m_SceneSerialisationVersion = 0;

        UniqueRef<EntityManager> m_EntityManager;
        UniqueRef<SceneGraph> m_SceneGraph;

        uint32_t m_ScreenWidth;
        uint32_t m_ScreenHeight;

        bool m_HasCppClass = true;

    private:
        NONCOPYABLE(Scene)

        bool OnWindowResize(WindowResizeEvent& e);

        friend class Entity;
    };
}
