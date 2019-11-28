#pragma once
#include "ECS/ISystem.h"
#include "Core/Typename.h"

namespace Lumos
{
    class SystemManager
    {
    public:

        template <typename T, typename ... Args>
        Ref<T> RegisterSystem(Args&& ...args)
        {
			auto typeName = typeid(T).hash_code();
            
            LUMOS_ASSERT(m_Systems.find(typeName) == m_Systems.end(), "Registering system more than once.");
            
            // Create a pointer to the system and return it so it can be used externally
            Ref<T> system = CreateRef<T>(std::forward<Args>(args) ...);
            m_Systems.insert({typeName, std::move( system) });
            return system;
        }

		template<typename T>
		Ref<T> RegisterSystem(T* t)
		{
			auto typeName = typeid(T).hash_code();

			LUMOS_ASSERT(m_Systems.find(typeName) == m_Systems.end(), "Registering system more than once.");

			// Create a pointer to the system and return it so it can be used externally
            Ref<T> system = Ref<T>(t);
            m_Systems.insert({ typeName,std::move( system) });
			return system;
		}

		template<typename T>
		void RemoveSystem()
		{
			auto typeName = typeid(T).hash_code();

			if (m_Systems.find(typeName) != m_Systems.end())
			{
				m_Systems.erase(typeName);
			}
		}

		template<typename T>
		T* GetSystem()
		{
			auto typeName = typeid(T).hash_code();

			if (m_Systems.find(typeName) != m_Systems.end())
			{
				return dynamic_cast<T*>(m_Systems[typeName].get());
			}

			return nullptr;
		}

		template<typename T>
		T* HasSystem()
		{
			auto typeName = typeid(T).hash_code();

			return m_Systems.find(typeName) != m_Systems.end();
		}

		void OnUpdate(TimeStep* dt, Scene* scene)
		{
			for (auto& system : m_Systems)
				system.second->OnUpdate(dt, scene);
		}

		void OnImGui()
		{
			for (auto& system : m_Systems)
				system.second->OnImGui();
		}

    private:
        // Map from system type string pointer to a system pointer
        std::unordered_map<size_t, Ref<ISystem>> m_Systems;
    };
}
