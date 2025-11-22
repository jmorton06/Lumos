#pragma once
#include "Scene/ISystem.h"
#include "Core/DataStructures/Map.h"
#include "Core/Mutex.h"

namespace Lumos
{
    class SystemManager
    {
    public:
        SystemManager();
        ~SystemManager();

        template <typename T, typename... Args>
        ISystem* RegisterSystem(Args&&... args)
        {
            ScopedMutex lock(m_Mutex);
            auto typeName = typeid(T).hash_code();
            ASSERT(!HasSystem<T>(), "Registering system more than once.");

            // Create a pointer to the system and return it so it can be used externally
            ISystem* system = new T(std::forward<Args>(args)...);
            HashMapInsert(&m_Systems, typeName, system);
            return system;
        }

        template <typename T>
        ISystem* RegisterSystem(T* t)
        {
            ScopedMutex lock(m_Mutex);
            auto typeName = typeid(T).hash_code();
            ASSERT(!HasSystem<T>(), "Registering system more than once.");

            // Create a pointer to the system and return it so it can be used externally
            ISystem* system = t;
            HashMapInsert(&m_Systems, typeName, system);
            return system;
        }

        template <typename T>
        void RemoveSystem()
        {
            ScopedMutex lock(m_Mutex);
            auto typeName = typeid(T).hash_code();
            HashMapRemove(&m_Systems, typeName);
        }

        template <typename T>
        T* GetSystem()
        {
            auto typeName = typeid(T).hash_code();

            ISystem* find;
            if(HashMapFind(&m_Systems, typeName, &find))
            {
                return dynamic_cast<T*>(find);
            }

            LWARN("Failed to find system");

            return nullptr;
        }

        template <typename T>
        bool HasSystem()
        {
            auto typeName = typeid(T).hash_code();
            ISystem* find;
            return HashMapFind(&m_Systems, typeName, &find);
        }

        void OnUpdate(const TimeStep& dt, Scene* scene)
        {
            ForHashMapEach(size_t, ISystem*, &m_Systems, it)
            {
                ISystem* value = *it.value;
                value->OnUpdate(dt, scene);
            }
        }

        void OnImGui();

        void OnDebugDraw()
        {
            ForHashMapEach(size_t, ISystem*, &m_Systems, it)
            {
                ISystem* value = *it.value;
                value->OnDebugDraw();
            }
        }

    private:
        Mutex* m_Mutex;
        Arena* m_Arena;

        HashMap(size_t, ISystem*) m_Systems;
    };
}
