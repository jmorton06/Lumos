#pragma once
#include "Scene/ISystem.h"

namespace Lumos
{
    class SystemManager
    {
    public:
        template <typename T, typename... Args>
        SharedRef<T> RegisterSystem(Args&&... args)
        {
            auto typeName = typeid(T).hash_code();

            LUMOS_ASSERT(m_Systems.find(typeName) == m_Systems.end(), "Registering system more than once.");

            // Create a pointer to the system and return it so it can be used externally
            SharedRef<T> system = CreateSharedRef<T>(std::forward<Args>(args)...);
            m_Systems.insert({ typeName, std::move(system) });
            return system;
        }

        template <typename T>
        SharedRef<T> RegisterSystem(T* t)
        {
            auto typeName = typeid(T).hash_code();

            LUMOS_ASSERT(m_Systems.find(typeName) == m_Systems.end(), "Registering system more than once.");

            // Create a pointer to the system and return it so it can be used externally
            SharedRef<T> system = SharedRef<T>(t);
            m_Systems.insert({ typeName, std::move(system) });
            return system;
        }

        template <typename T>
        void RemoveSystem()
        {
            auto typeName = typeid(T).hash_code();

            if(m_Systems.find(typeName) != m_Systems.end())
            {
                m_Systems.erase(typeName);
            }
        }

        template <typename T>
        T* GetSystem()
        {
            auto typeName = typeid(T).hash_code();

            if(m_Systems.find(typeName) != m_Systems.end())
            {
                return dynamic_cast<T*>(m_Systems[typeName].get());
            }

            return nullptr;
        }

        template <typename T>
        T* HasSystem()
        {
            auto typeName = typeid(T).hash_code();

            return m_Systems.find(typeName) != m_Systems.end();
        }

        void OnUpdate(const TimeStep& dt, Scene* scene)
        {
            for(auto& system : m_Systems)
                system.second->OnUpdate(dt, scene);
        }

        void OnImGui()
        {
            for(auto& system : m_Systems)
                system.second->OnImGui();
        }

        void OnDebugDraw()
        {
            for(auto& system : m_Systems)
                system.second->OnDebugDraw();
        }

    private:
        // Map from system type string pointer to a system pointer
        std::unordered_map<size_t, SharedRef<ISystem>> m_Systems;
    };
}
