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
		void SetEntity(Entity* entity) { m_Entity = entity; }

        virtual void OnImGui() {}
		virtual size_t GetTypeID() const = 0;
		virtual const String& GetName() const = 0;

		const Ref<Maths::BoundingShape>& GetBoundingShape() const { return m_BoundingShape; }

		const bool GetCanDisable() const { return m_CanDisable; }
		bool& GetActive() { return m_Active; }

		void SetActive(bool active) { m_Active = active; }

	protected:
		Entity* m_Entity = nullptr;
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
