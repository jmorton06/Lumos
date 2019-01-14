#pragma once

#include "LM.h"
#include "Maths/BoundingShape.h"

namespace Lumos 
{
	class Entity;

	enum class ComponentType
	{
		Model,
        Light,
        AI,
        Particle,
        Physics2D,
        Physics3D,
        Sound,
        Sprite,
        TextureMatrix,
        Transform,
        Error
	};

	class LUMOS_EXPORT JMComponent
	{
	public:
		virtual ~JMComponent() = default;
		virtual Entity* GetEntity() { return m_Entity; }
        virtual ComponentType GetType() const { return ComponentType::Error; }

		virtual void Init() {}; //Called After entity is set
		virtual void OnRenderComponent() {};
		virtual void OnUpdateComponent(float dt) {};

		void SetEntity(Entity* entity) { m_Entity = entity; }

		virtual void UpdateBoundingShape() { };

		maths::BoundingShape* GetBoundingShape() const { return m_BoundingShape.get(); }

		virtual void DebugDraw(uint64 debugFlags) {};

	protected:
		Entity* m_Entity = nullptr;
		std::unique_ptr<maths::BoundingShape> m_BoundingShape;
	};

}
