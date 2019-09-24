#pragma once

#include "lmpch.h"
#include "Maths/BoundingShape.h"

#include "Core/Serialisable.h"

namespace Lumos 
{
	class Entity;
	using ComponentType = std::uint8_t;

	class LUMOS_EXPORT LumosComponent : public Serialisable
	{
	public:
		virtual ~LumosComponent() = default;
		virtual Entity* GetEntity() { return m_Entity; }

		virtual void OnRenderComponent() {};
        virtual void OnImGui() {}

		void SetEntity(Entity* entity) { m_Entity = entity; }

		virtual void UpdateBoundingShape() { };
		virtual void OnUpdateTransform(const Maths::Matrix4& entityTransform) {};
		virtual size_t GetTypeID() const = 0;

		const Ref<Maths::BoundingShape>& GetBoundingShape() const { return m_BoundingShape; }

		virtual const String& GetName() const = 0;
		const bool GetCanDisable() const { return m_CanDisable; }
		bool& GetActive() { return m_Active; }

		void SetActive(bool active) { m_Active = active; }
        //void SetName(const String& name) { m_Name = name; }

	protected:
		Entity* m_Entity = nullptr;
		//String m_Name;
		bool m_Active = true;
		bool m_CanDisable = true;
		Ref<Maths::BoundingShape> m_BoundingShape;
	};

}

#define SETUPCOMPOMENT(type_identifier)								\
    size_t GetTypeID() const override { return typeid(type_identifier).hash_code(); } \
	const String& GetName() const override { return m_Name; } \
	private: \
	String m_Name = LUMOS_TYPENAME_STRING(type_identifier);
