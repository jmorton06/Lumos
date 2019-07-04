#pragma once
#include "LM.h"
#include "Utilities/TSingleton.h"
#include "Entity/Component/LumosComponent.h"

#define MAX_ENTITIES 5000

namespace Lumos
{
	class IComponentArray
	{
	public:
		virtual ~IComponentArray() = default;
		virtual void EntityDestroyed(Entity* entity) = 0;
		virtual void OnUpdate(float dt) = 0;
	};

	template<typename T>
	class ComponentArray : public IComponentArray
	{
	public:
		void InsertData(Entity* entity, T* component)
		{
			assert(m_EntityToIndexMap.find(entity) == m_EntityToIndexMap.end() && "Component added to same entity more than once.");

			// Put new entry at end and update the maps
			size_t newIndex = m_Size;
			m_EntityToIndexMap[entity] = newIndex;
			m_IndexToEntityMap[newIndex] = entity;
			m_ComponentArray[newIndex] = component;
			m_Size++;
		}

		void RemoveData(Entity* entity)
		{
			assert(m_EntityToIndexMap.find(entity) != m_EntityToIndexMap.end() && "Removing non-existent component.");

			// Copy element at end into deleted element's place to maintain density
			size_t indexOfRemovedEntity = m_EntityToIndexMap[entity];
			size_t indexOfLastElement = m_Size - 1;
			m_ComponentArray[indexOfRemovedEntity] = m_ComponentArray[indexOfLastElement];

			// Update map to point to moved spot
			Entity* entityOfLastElement = m_IndexToEntityMap[indexOfLastElement];
			m_EntityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
			m_IndexToEntityMap[indexOfRemovedEntity] = entityOfLastElement;

			m_EntityToIndexMap.erase(entity);
			m_IndexToEntityMap.erase(indexOfLastElement);

			m_Size--;
		}

		T* GetData(Entity* entity)
		{
			if (m_EntityToIndexMap.find(entity) == m_EntityToIndexMap.end())
				return nullptr;

			// Return a reference to the entity's component
			return m_ComponentArray[m_EntityToIndexMap[entity]];
		}

		void OnUpdate(float dt) override 
		{
			for (auto& component : m_ComponentArray)
			{
				if (!component)
					return;

				component->OnUpdateComponent(dt);
			}
		}

		std::array<T*, MAX_ENTITIES> GetArray() const
		{
			return m_ComponentArray;
		}

		void EntityDestroyed(Entity* entity) override
		{
			if (m_EntityToIndexMap.find(entity) != m_EntityToIndexMap.end())
			{
				// Remove the entity's component if it existed
				RemoveData(entity);
			}
		}

	private:
		// The packed array of components (of generic type T),
		// set to a specified maximum amount, matching the maximum number
		// of entities allowed to exist simultaneously, so that each entity
		// has a unique spot.
		std::array<T*, MAX_ENTITIES> m_ComponentArray;

		// Map from an entity ID to an array index.
		std::unordered_map<Entity*, size_t> m_EntityToIndexMap;

		// Map from an array index to an entity ID.
		std::unordered_map<size_t, Entity*> m_IndexToEntityMap;

		// Total size of valid entries in the array.
		size_t m_Size;
	};

	class LUMOS_EXPORT ComponentManager : public TSingleton<ComponentManager>
	{
	friend class TSingleton<ComponentManager>;
	friend class LumosComponent;

	public:
		ComponentManager();
		~ComponentManager();

		void OnUpdate(float dt);

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
			m_NextComponentType++;
		}

		template<typename T>
		ComponentType GetComponentType()
		{
			size_t typeName = typeid(T).hash_code();

			LUMOS_CORE_ASSERT(m_ComponentTypes.find(typeName) != m_ComponentTypes.end(), "Component not registered before use.");

			return m_ComponentTypes[typeName];
		}

		template<typename T>
		void AddComponent(Entity* entity, T* component)
		{
			GetComponentArray<T>()->InsertData(entity, component);
		}

		template<typename T>
		void RemoveComponent(Entity* entity)
		{
			GetComponentArray<T>()->RemoveData(entity);
		}

		template<typename T>
		T* GetComponent(Entity* entity)
		{
			return GetComponentArray<T>()->GetData(entity);
		}

		void EntityDestroyed(Entity* entity)
		{
			// Notify each component array that an entity has been destroyed
			// If it has a component for that entity, it will remove it
			for (auto const& pair : m_ComponentArrays)
			{
				auto const& component = pair.second;

				component->EntityDestroyed(entity);
			}
		}

		template<typename T>
		std::shared_ptr<ComponentArray<T>> GetComponentArray()
		{
			size_t typeName = typeid(T).hash_code();

			LUMOS_CORE_ASSERT(m_ComponentArrays.find(typeName) != m_ComponentArrays.end(), "Component not registered before use.");

			return std::static_pointer_cast<ComponentArray<T>>(m_ComponentArrays[typeName]);
		}

		std::vector<LumosComponent*> GetAllComponents(Entity* entity);

	private:
		std::unordered_map<size_t, ComponentType> m_ComponentTypes;
		std::unordered_map<size_t, std::shared_ptr<IComponentArray>> m_ComponentArrays;
		ComponentType m_NextComponentType;
	};
}


