#pragma once
#include "lmpch.h"
#include "ECS.h"
#include "ECSDefines.h"
#include "Utilities/TSingleton.h"

#include "Core/Typename.h"
#include "Core/Profiler.h"

namespace Lumos
{
	class Entity;

    template <class T>
    using HasInit = decltype(std::declval<T>().Init());
    
    template <class T>
    using HasUpdate = decltype(std::declval<T>().Update());

	template <class T>
	using HasActive = decltype(std::declval<T>().GetActive());

	template <class T>
	using HasImGui = decltype(std::declval<T>().OnImGui());

	using ComponentType = uint32_t;

	class IComponentArray
	{
	public:
		virtual ~IComponentArray() = default;
		virtual void EntityDestroyed(Entity* entity) = 0;
		virtual void OnUpdate() = 0;
		virtual void OnImGui(Entity* entity) = 0;
		virtual void RemoveData(Entity* entity) = 0;
		virtual void CreateLumosComponent(Entity* entity) = 0;
		virtual size_t GetID() = 0;
		virtual const String GetName() const = 0;

		LUMOS_EXPORT void ImGuiComponentHeader(const String& name, size_t hashCode, Entity * entity, bool& open, bool hasActive, bool& active);
	};

	template<typename T>
	class ComponentArray : public IComponentArray
	{
	public:

		ComponentArray(u32 initSize = 30)
		{
			m_ComponentArray.reserve(30);
			m_Size = 0;
		}

		void InsertData(Entity* entity, const T& component)
		{
			LUMOS_ASSERT(m_EntityToIndexMap.find(entity) == m_EntityToIndexMap.end(), "Component added to same entity more than once.");

			// Put new entry at end and update the maps
			size_t newIndex = m_Size;
			m_EntityToIndexMap[entity] = newIndex;
			m_IndexToEntityMap[newIndex] = entity;
            
            m_Size++;

            if constexpr(is_detected_v<HasInit, T>)
            {
                m_ComponentArray[newIndex].Init();
            }
		}

		void RemoveData(Entity* entity) override
		{
			LUMOS_ASSERT(m_EntityToIndexMap.find(entity) != m_EntityToIndexMap.end(), "Removing non-existent component.");

			// Copy element at end into deleted element's place to maintain density
			size_t indexOfRemovedEntity = m_EntityToIndexMap[entity];
			size_t indexOfLastElement = m_Size - 1;

            m_ComponentArray.erase(m_ComponentArray.begin() + indexOfRemovedEntity);
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
			auto index = m_EntityToIndexMap.find(entity);
			if (index != m_EntityToIndexMap.end())
				return &m_ComponentArray[index->second];

			return nullptr;
		}

		bool HasData(Entity* entity)
		{
			auto index = m_EntityToIndexMap.find(entity);
			if (index != m_EntityToIndexMap.end())
				return true;

			return false;
		}

		Entity* GetEntity(T* component)
		{
			for (size_t index = 0; index < m_Size; index++)
			{
				if (&m_ComponentArray[index] == component)
				{
					return m_IndexToEntityMap[index];
				}
			}

            LUMOS_ASSERT(false, "Entity not found");
			return nullptr;
		}

		void OnUpdate() override
		{
			LUMOS_PROFILE_BLOCK("ComponentManager::OnUpdate");
			for (int i = 0; i < m_Size; i++)
			{
				if constexpr (is_detected_v<HasUpdate, T>)
				{
					m_ComponentArray[i].Update();
				}
			}
		}

		void OnImGui(Entity* entity) override;

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

		template<typename ... Args>
		T* CreateComponent(Entity* entity, Args&& ...args)
		{
			T& component = m_ComponentArray.emplace_back(std::forward<Args>(args)...);
			InsertData(entity, component);
			return &component;
		}

		void CreateLumosComponent(Entity* entity) override
		{
			T& component = m_ComponentArray.emplace_back();
			InsertData(entity, component);
		}

		size_t GetID() override
		{
			return typeid(T).hash_code();
		}

		const String GetName() const override
		{
			return LUMOS_TYPENAME_STRING(T);
		}

		inline T* operator[](int index)
		{
			return &m_ComponentArray[index];
		}


	private:
		// The packed array of components (of generic type T),
		// set to a specified maximum amount, matching the maximum number
		// of entities allowed to exist simultaneously, so that each entity
		// has a unique spot.
		std::vector<T> m_ComponentArray{};

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

		template<typename T, typename ... Args>
		T* AddComponent(Entity* entity, Args&& ...args)
		{
			return GetComponentArray<T>()->CreateComponent(entity, std::forward<Args>(args) ...);
		}

		template<typename T>
		void RemoveComponent(Entity* entity)
		{
			GetComponentArray<T>()->RemoveData(entity);
		}

		void RemoveComponent(Entity* entity, size_t id)
		{
			m_ComponentArrays.at(id)->RemoveData(entity);
		}

		template<typename T>
		T* GetComponent(Entity* entity)
		{
			return GetComponentArray<T>()->GetData(entity);
		}

		template<typename T>
		bool HasComponent(Entity* entity)
		{
			return GetComponentArray<T>()->HasData(entity);
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

		void OnImGui(Entity* entity)
		{
			// Notify each component array that an entity has been destroyed
			// If it has a component for that entity, it will remove it
			for (auto& pair : m_ComponentArrays)
			{
				auto& component = pair.second;

				component->OnImGui(entity);
			}
		}

		template<typename T>
		ComponentArray<T>* GetComponentArray()
		{
			auto typeName = typeid(T).hash_code();
			auto index = m_ComponentArrays.find(typeName);
			if (index == m_ComponentArrays.end())
				RegisterComponent<T>();

			return static_cast<ComponentArray<T>*>(m_ComponentArrays[typeName].get());
		}

		void CreateComponent(Entity* entity, size_t id)
		{
			m_ComponentArrays.at(id)->CreateLumosComponent(entity);
		}

		const std::unordered_map<size_t, Ref<IComponentArray>>& GetComponentArrays() const { return m_ComponentArrays; }

	private:
		std::unordered_map<size_t, ComponentType> m_ComponentTypes;
		std::unordered_map<size_t, Ref<IComponentArray>> m_ComponentArrays;
		ComponentType m_NextComponentType;
	};

	template<typename T>
	inline void ComponentArray<T>::OnImGui(Entity * entity)
	{
		LUMOS_PROFILE_BLOCK("ComponentManager::OnImGui");
		
		T* component = GetData(entity);

		if (component)
		{
			String componentName = LUMOS_TYPENAME_STRING(T);
			size_t typeID = typeid(T).hash_code();
			bool open = true;
			bool hasActive = false;
			bool& active = hasActive;
			if constexpr (is_detected_v<HasActive, T>)
			{
				active = component->GetActive();
				hasActive = true;
			}

			ImGuiComponentHeader(componentName, typeID, entity, open, hasActive, active);

			if constexpr (is_detected_v<HasImGui, T>)
			{
				if (open)
				{
					component->OnImGui();
				}
			}
		}
	}
}

