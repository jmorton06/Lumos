#pragma once

#include "LM.h"
#include "Component/Components.h"

struct EnumClassHash
{
    template <typename T>
    std::size_t operator()(T t) const
    {
        return static_cast<std::size_t>(t);
    }
};

namespace lumos
{
	class Scene;

	class LUMOS_EXPORT Entity
	{
	protected:
		std::unordered_map<ComponentType, std::unique_ptr<LumosComponent>, EnumClassHash> m_Components;
	public:
		Entity(Scene* scene);
		virtual ~Entity();
		explicit Entity(const String& name, Scene* scene);

		void AddComponent(std::unique_ptr<LumosComponent> component);

		template <typename T>
		/*const */T* GetComponent() const
		{
			return GetComponentInternal<T>();
		}

		template <typename T>
		T* GetComponent()
		{
			return static_cast<T*>(GetComponentInternal<T>());
		}

		void OnRenderObject();
		virtual void OnUpdateObject(float dt);
        virtual void OnIMGUI();
		virtual void OnGuizmo(uint mode = 0);
        virtual void Init();

		std::vector<std::shared_ptr<Entity>>& GetChildren() { return m_vpChildren; }
		void AddChildObject(std::shared_ptr<Entity>& child);

		void  SetBoundingRadius(float radius) { m_BoundingRadius = radius; }
		float GetBoundingRadius() const { return m_BoundingRadius; }

		uint& GetFrustumCullFlags() { return m_FrustumCullFlags; }

		Scene* GetScene() const { return m_pScene; }
		void SetScene(Scene* scene) { m_pScene = scene; }

		void DebugDraw(uint64 debugFlags);

		TransformComponent* GetTransform();
        
        void SetParent(Entity* parent);
        
        const String& GetName() const { return m_Name; }
        const String& GetUUID() const { return m_UUID; }

		const bool IsActive() const { return m_Active; }
		void SetActive(bool active) { m_Active = active; };
		void SetActiveRecursive(bool active);

	private:

		Entity(Entity const&) = delete;
		Entity& operator=(Entity const&) = delete;

		template <typename T>
		T* GetComponentInternal() const
		{
			ComponentType type = T::GetStaticType();
			auto it = m_Components.find(type);
			if (it == m_Components.end())
				return nullptr;
			return static_cast<T*>(it->second.get());
		}

		String					m_Name;
		Scene*					m_pScene;
		Entity*					m_pParent;
		std::vector<std::shared_ptr<Entity>> m_vpChildren;
		float					m_BoundingRadius;
		uint					m_FrustumCullFlags;
        String                  m_UUID;
		bool					m_Active;
		TransformComponent*		m_DefaultTransformComponent = nullptr;
	};
}
