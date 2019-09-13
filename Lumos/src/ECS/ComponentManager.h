#pragma once
#include "lmpch.h"
#include "ECS.h"
#include "ECSDefines.h"
#include "Utilities/TSingleton.h"
#include "ECS/Component/LumosComponent.h"

#include "Core/Typename.h"

namespace Lumos
{
    template <class T>
    using HasInit = decltype(std::declval<T>().Init());
    
    template <class T>
    using HasUpdate = decltype(std::declval<T>().Update());

	class IComponentArray
	{
	public:
		virtual ~IComponentArray() = default;
		virtual void EntityDestroyed(Entity* entity) = 0;
		virtual void OnUpdate() = 0;
	};

	template<typename T>
	class ComponentArray : public IComponentArray
	{
	public:
		void InsertData(Entity* entity, T* component)
		{
			LUMOS_ASSERT(m_EntityToIndexMap.find(entity) == m_EntityToIndexMap.end(), "Component added to same entity more than once.");

			// Put new entry at end and update the maps
			size_t newIndex = m_Size;
			m_EntityToIndexMap[entity] = newIndex;
			m_IndexToEntityMap[newIndex] = entity;
			m_ComponentArray[newIndex] = component;
            m_ComponentArray[newIndex]->SetEntity(entity);
            
            if constexpr(is_detected_v<HasInit, T>)
            {
                m_ComponentArray[newIndex]->Init();
            }
			m_Size++;
		}

		void RemoveData(Entity* entity)
		{
			LUMOS_ASSERT(m_EntityToIndexMap.find(entity) != m_EntityToIndexMap.end(), "Removing non-existent component.");

			// Copy element at end into deleted element's place to maintain density
			size_t indexOfRemovedEntity = m_EntityToIndexMap[entity];
			size_t indexOfLastElement = m_Size - 1;
			delete m_ComponentArray[indexOfRemovedEntity];
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

		void OnUpdate() override
		{
			for (int i = 0; i < m_Size; i++)
			{
				if constexpr (is_detected_v<HasUpdate, T>)
				{
					m_ComponentArray[i]->Update();
				}
			}
		}

		const std::array<T*, MAX_ENTITIES>& GetArray() const
		{
			return m_ComponentArray;
		}
        
        T** GetRawData() const
        {
            return m_ComponentArray.data();
        }

		size_t GetSize() const
		{
			return m_Size;
		}

		void EntityDestroyed(Entity* entity) override
		{
			if (m_EntityToIndexMap.find(entity) != m_EntityToIndexMap.end())
			{
				// Remove the entity's component if it existed
				RemoveData(entity);
			}
		}

		T* CreateComponent(Entity* entity)
		{
			auto component = lmnew T();
			InsertData(entity, component);
			return component;
		}

	private:
		// The packed array of components (of generic type T),
		// set to a specified maximum amount, matching the maximum number
		// of entities allowed to exist simultaneously, so that each entity
		// has a unique spot.
		std::array<T*, MAX_ENTITIES> m_ComponentArray{};

		// Map from an entity ID to an array index.
		std::unordered_map<Entity*, size_t> m_EntityToIndexMap{};

		// Map from an array index to an entity ID.
		std::unordered_map<size_t, Entity*> m_IndexToEntityMap{};

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

		void OnUpdate();

		template<typename T>
		void RegisterComponent()
		{
			auto typeName = typeid(T).hash_code();

			LUMOS_ASSERT(m_ComponentTypes.find(typeName) == m_ComponentTypes.end(), "Registering component type more than once.");

			// Add this component type to the component type map
            m_ComponentTypes[typeName] = m_NextComponentType;

			// Create a ComponentArray pointer and add it to the component arrays map
			m_ComponentArrays[typeName] = CreateRef<ComponentArray<T>>();

			// Increment the value so that the next component registered will be different
			m_NextComponentType++;
		}

		template<typename T>
		ComponentType GetComponentType()
		{
			auto typeName = typeid(T).hash_code();

			LUMOS_ASSERT(m_ComponentTypes.find(typeName) != m_ComponentTypes.end(), "Component not registered before use.");

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
			for (auto& pair : m_ComponentArrays)
			{
				auto& component = pair.second;

				component->EntityDestroyed(entity);
			}
		}

		template<typename T>
		ComponentArray<T>* GetComponentArray()
		{
			auto typeName = typeid(T).hash_code();

			LUMOS_ASSERT(m_ComponentArrays.find(typeName) != m_ComponentArrays.end(), "Component not registered before use.");

			return static_cast<ComponentArray<T>*>(m_ComponentArrays[typeName].get());
		}

		const std::vector<LumosComponent*> GetAllComponents(Entity* entity);

	private:
		std::unordered_map<size_t, ComponentType> m_ComponentTypes;
		std::unordered_map<size_t, Ref<IComponentArray>> m_ComponentArrays;
		ComponentType m_NextComponentType;
	};
}

