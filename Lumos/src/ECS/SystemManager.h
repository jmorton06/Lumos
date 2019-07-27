#pragma once

#include "ECS/ISystem.h"
#include "Core/Typename.h"

namespace Lumos
{
    class SystemManager
    {
    public:

        template<typename T>
        std::shared_ptr<T> RegisterSystem()
        {
			String typeName = LUMOS_TYPENAME(T);
            
            LUMOS_CORE_ASSERT(m_Systems.find(typeName) == m_Systems.end(), "Registering system more than once.");
            
            // Create a pointer to the system and return it so it can be used externally
            auto system = std::make_shared<T>();
            m_Systems.insert({typeName, system});
            return system;
        }

		template<typename T>
		std::shared_ptr<T> RegisterSystem(T* t)
		{
			String typeName = LUMOS_TYPENAME(T);

			LUMOS_CORE_ASSERT(m_Systems.find(typeName) == m_Systems.end(), "Registering system more than once.");

			// Create a pointer to the system and return it so it can be used externally
			auto system = std::shared_ptr<T>(t);
			m_Systems.insert({ typeName, system });
			return system;
		}

		template<typename T>
		void RemoveSystem()
		{
			String typeName = LUMOS_TYPENAME(T);

			if (m_Systems.find(typeName) != m_Systems.end())
			{
				m_Systems.erase(typeName);
			}
		}

		template<typename T>
		T* GetSystem()
		{
			String typeName = LUMOS_TYPENAME(T);

			if (m_Systems.find(typeName) != m_Systems.end())
			{
				return (T*)m_Systems[typeName].get();
			}

			return nullptr;
		}

		template<typename T>
		T* HasSystem()
		{
			String typeName = LUMOS_TYPENAME(T);

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
        std::unordered_map<String, std::shared_ptr<ISystem>> m_Systems;
    };
}
