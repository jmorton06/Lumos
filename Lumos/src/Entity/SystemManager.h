#pragma once

#include "App/ISystem.h"

namespace Lumos
{
    class SystemManager
    {
    public:

        template<typename T>
        std::shared_ptr<T> RegisterSystem()
        {
			size_t typeName = typeid(T).hash_code();
            
            LUMOS_CORE_ASSERT(m_Systems.find(typeName) == m_Systems.end(), "Registering system more than once.");
            
            // Create a pointer to the system and return it so it can be used externally
            auto system = std::make_shared<T>();
            m_Systems.insert({typeName, system});
            return system;
        }

		template<typename T>
		std::shared_ptr<T> RegisterSystem(T* t)
		{
			size_t typeName = typeid(T).hash_code();

			LUMOS_CORE_ASSERT(m_Systems.find(typeName) == m_Systems.end(), "Registering system more than once.");

			// Create a pointer to the system and return it so it can be used externally
			auto system = std::shared_ptr<T>(t);
			m_Systems.insert({ typeName, system });
			return system;
		}

		template<typename T>
		void RemoveSystem()
		{
			size_t typeName = typeid(T).hash_code();

			if (m_Systems.find(typeName) != m_Systems.end())
			{
				m_Systems.erase(typeName);
			}
		}

		template<typename T>
		T* GetSystem()
		{
			size_t typeName = typeid(T).hash_code();

			if (m_Systems.find(typeName) != m_Systems.end())
			{
				return (T*)m_Systems[typeName].get();
			}

			return nullptr;
		}

		template<typename T>
		T* HasSystem()
		{
			size_t typeName = typeid(T).hash_code();

			return m_Systems.find(typeName) != m_Systems.end();
		}

		void OnUpdate(TimeStep* dt)
		{
			for (auto& system : m_Systems)
				system.second->OnUpdate(dt);
		}

		void OnImGUI()
		{
			for (auto& system : m_Systems)
				system.second->OnIMGUI();
		}

    private:
        // Map from system type string pointer to a system pointer
        std::unordered_map<size_t, std::shared_ptr<ISystem>> m_Systems;
    };
}
