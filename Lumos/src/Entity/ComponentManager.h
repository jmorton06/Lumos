#pragma once
#include "LM.h"
#include "Utilities/TSingleton.h"
#include "Entity/Component/LumosComponent.h"

namespace Lumos
{
	class IComponentArray
	{
	public:
		virtual ~IComponentArray() = default;
	};

	template<typename T>
	class ComponentArray : public IComponentArray
	{
	public:
		void InsertData(T data) { m_ComponentArray.emplace_back(data); }
	private:
		std::vector<T> m_ComponentArray;
	};

	class LUMOS_EXPORT ComponentManager : public TSingleton<ComponentManager>
	{
	friend class TSingleton<ComponentManager>;
	friend class LumosComponent;

	public:
		ComponentManager();
		~ComponentManager();

		template<typename T>
		void RegisterComponent()
		{
			size_t typeName = typeid(T).hash_code();

			LUMOS_CORE_ASSERT(m_ComponentTypes.find(typeName) == m_ComponentTypes.end(), "Registering component type more than once.");

			// Add this component type to the component type map
			m_ComponentTypes.insert({ typeName, m_NextComponentType });

			// Create a ComponentArray pointer and add it to the component arrays map
			m_ComponentArrays.insert({ typeName, std::make_shared<ComponentArray<T>>() });

			// Increment the value so that the next component registered will be different
			++m_NextComponentType;
		}

		template<typename T>
		ComponentType GetComponentType()
		{
			size_t typeName = typeid(T).hash_code();

			LUMOS_CORE_ASSERT(m_ComponentTypes.find(typeName) != m_ComponentTypes.end(), "Component not registered before use.");

			return m_ComponentTypes[typeName];
		}

		template<typename T>
		void AddComponent(Entity* entity, T component)
		{
			GetComponentArray<T>()->InsertData(entity, component);
		}

		template<typename T>
		void RemoveComponent(Entity* entity)
		{
			GetComponentArray<T>()->RemoveData(entity);
		}

		template<typename T>
		T& GetComponent(Entity* entity)
		{
			return GetComponentArray<T>()->GetData(entity);
		}

	
	private:
		std::unordered_map<size_t, ComponentType> m_ComponentTypes;
		std::unordered_map<size_t, std::shared_ptr<IComponentArray>> m_ComponentArrays;
		ComponentType m_NextComponentType;

		template<typename T>
		std::shared_ptr<ComponentArray<T>> GetComponentArray()
		{
			size_t typeName = typeid(T).hash_code();

			LUMOS_CORE_ASSERT(m_ComponentArrays.find(typeName) != m_ComponentArrays.end(), "Component not registered before use.");

			return std::static_pointer_cast<ComponentArray<T>>(m_ComponentArrays[typeName]);
		}
	};
}


