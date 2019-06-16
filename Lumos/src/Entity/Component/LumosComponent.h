#pragma once

#include "LM.h"
#include "Maths/BoundingShape.h"

namespace Lumos 
{
	class Entity;

	enum class ComponentType
	{
		Mesh,
        Light,
        AI,
        Particle,
        Physics2D,
        Physics3D,
        Sound,
        Sprite,
        TextureMatrix,
        Transform,
		Camera,
        Material,
        Error
	};

	class LUMOS_EXPORT LumosComponent
	{
	public:
		virtual ~LumosComponent() = default;
		virtual Entity* GetEntity() { return m_Entity; }
        virtual ComponentType GetType() const { return ComponentType::Error; }

		virtual void Init() {}; //Called After entity is set
		virtual void OnRenderComponent() {};
		virtual void OnUpdateComponent(float dt) {};
        virtual void OnIMGUI() {}

		void SetEntity(Entity* entity) { m_Entity = entity; }

		virtual void UpdateBoundingShape() { };
		virtual void OnUpdateTransform(const Maths::Matrix4& entityTransform) {};

		Maths::BoundingShape* GetBoundingShape() const { return m_BoundingShape.get(); }

		virtual void DebugDraw(uint64 debugFlags) {};

		const String& GetName() const { return m_Name; }
		bool& GetActive() { return m_Active; }
		const bool GetCanDisable() const { return m_CanDisable; }
		void SetActive(bool active) { m_Active = active; }

	protected:
		Entity* m_Entity = nullptr;
		String m_Name;
		bool m_Active = true;
		bool m_CanDisable = true;
		std::unique_ptr<Maths::BoundingShape> m_BoundingShape;
	};

}
