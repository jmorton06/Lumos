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

namespace Lumos
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
		virtual void OnGuizmo();

		std::vector<Entity*>& GetChildren() { return m_vpChildren; }
		void AddChildObject(Entity* child);

		const String& GetName() const { return m_Name; }

		void  SetBoundingRadius(float radius) { m_BoundingRadius = radius; }
		float GetBoundingRadius() const { return m_BoundingRadius; }

		uint& GetFrustumCullFlags() { return m_FrustumCullFlags; }

		Scene* GetScene() const { return m_pScene; }
		void SetScene(Scene* scene) { m_pScene = scene; }

		void DebugDraw(uint64 debugFlags);
		void SetPosition(const maths::Vector3& pos) { m_Position = pos; };

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
		std::vector<Entity*>	m_vpChildren;
		float					m_BoundingRadius;
		uint					m_FrustumCullFlags;
		maths::Vector3			m_Position;
		bool					m_UpdateTransforms;
	};
}
