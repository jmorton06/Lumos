#pragma once

#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Physics/LumosPhysicsEngine/SpringConstraint.h"
#include "Physics/LumosPhysicsEngine/WeldConstraint.h"
#include "Physics/LumosPhysicsEngine/DistanceConstraint.h"
#include "Physics/LumosPhysicsEngine/AxisConstraint.h"

#include "Scene/Entity.h"
#include <cereal/cereal.hpp>

namespace Lumos
{
    class AxisConstraintComponent
    {
    public:
        AxisConstraintComponent(Entity entity, Axes axis);
        ~AxisConstraintComponent() = default;
        
        const Ref<AxisConstraint>& GetConstraint() const { return m_Constraint;}

    private:
        Ref<AxisConstraint> m_Constraint;
    };

	class SpringConstraintComponent
	{
	public:
		SpringConstraintComponent(Entity entity, Entity otherEntity, const Maths::Vector3& pos1, const Maths::Vector3& pos2, float constant = 1.0f);
		SpringConstraintComponent(Entity entity, Entity otherEntity);
		~SpringConstraintComponent() = default;
		
		const Ref<SpringConstraint>& GetConstraint() const { return m_Constraint;}

	private:
		Ref<SpringConstraint> m_Constraint;
		Entity m_OtherEntity;
	};

	class WeldConstraintComponent
	{
	public:
		WeldConstraintComponent(Entity entity,Entity otherEntity, const Maths::Vector3& pos1, const Maths::Vector3& pos2, float constant = 1.0f);
		WeldConstraintComponent(Entity entity,Entity otherEntity);
		~WeldConstraintComponent() = default;
		
		const Ref<WeldConstraint>& GetConstraint() const { return m_Constraint;}
		
	private:
		Ref<WeldConstraint> m_Constraint;
		Entity m_OtherEntity;
	};

	class DistanceConstraintComponent
	{
	public:
		DistanceConstraintComponent(Entity entity,Entity otherEntity, const Maths::Vector3& pos1, const Maths::Vector3& pos2, float constant = 1.0f);
		DistanceConstraintComponent(Entity entity,Entity otherEntity);
		~DistanceConstraintComponent() = default;
		
		const Ref<DistanceConstraint>& GetConstraint() const { return m_Constraint;}
		
	private:
		Ref<DistanceConstraint> m_Constraint;
		Entity m_OtherEntity;
	};

	class LUMOS_EXPORT Physics3DComponent
	{
	public:
		Physics3DComponent();
		explicit Physics3DComponent(Ref<RigidBody3D>& physics);

		~Physics3DComponent() = default;

		void Init();
		void Update();
		void OnImGui();

		const Ref<RigidBody3D>& GetRigidBody() const
		{
			return m_RigidBody;
		}

		template<typename Archive>
		void save(Archive& archive) const
		{
			archive(*m_RigidBody.get());
		}

		template<typename Archive>
		void load(Archive& archive)
		{
			m_RigidBody = CreateRef<RigidBody3D>();
			archive(*m_RigidBody.get());
		}

	private:
		Ref<RigidBody3D> m_RigidBody;
	};
}
