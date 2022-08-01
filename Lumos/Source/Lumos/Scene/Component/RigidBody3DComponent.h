#pragma once

#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Physics/LumosPhysicsEngine/Constraints/SpringConstraint.h"
#include "Physics/LumosPhysicsEngine/Constraints/WeldConstraint.h"
#include "Physics/LumosPhysicsEngine/Constraints/DistanceConstraint.h"
#include "Physics/LumosPhysicsEngine/Constraints/AxisConstraint.h"

#include "Scene/Entity.h"
#include <cereal/cereal.hpp>

namespace Lumos
{
    class AxisConstraintComponent
    {
    public:
        AxisConstraintComponent() = default;
        AxisConstraintComponent(Entity entity, Axes axis);
        ~AxisConstraintComponent() = default;

        const SharedPtr<AxisConstraint>& GetConstraint();

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(m_EntityID, (int)m_Axes);
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            int axisInt;
            archive(m_EntityID, axisInt);
            m_Initialised = false;
            m_Axes        = (Axes)axisInt;
        }

        Axes GetAxes() { return m_Axes; }
        uint64_t GetEntityID() { return m_EntityID; }
        void SetAxes(Axes axes)
        {
            m_Axes        = axes;
            m_Initialised = false;
        }

        void SetEntity(uint64_t entityID)
        {
            m_EntityID    = entityID;
            m_Initialised = false;
        }

    private:
        SharedPtr<AxisConstraint> m_Constraint;
        bool m_Initialised = false;
        uint64_t m_EntityID;
        Axes m_Axes;
    };

    class SpringConstraintComponent
    {
    public:
        SpringConstraintComponent(Entity entity, Entity otherEntity, const glm::vec3& pos1, const glm::vec3& pos2, float constant = 1.0f);
        SpringConstraintComponent(Entity entity, Entity otherEntity);
        ~SpringConstraintComponent() = default;

        const SharedPtr<SpringConstraint>& GetConstraint() const { return m_Constraint; }

    private:
        SharedPtr<SpringConstraint> m_Constraint;
        Entity m_OtherEntity;
    };

    class WeldConstraintComponent
    {
    public:
        WeldConstraintComponent(Entity entity, Entity otherEntity, const glm::vec3& pos1, const glm::vec3& pos2, float constant = 1.0f);
        WeldConstraintComponent(Entity entity, Entity otherEntity);
        ~WeldConstraintComponent() = default;

        const SharedPtr<WeldConstraint>& GetConstraint() const { return m_Constraint; }

    private:
        SharedPtr<WeldConstraint> m_Constraint;
        Entity m_OtherEntity;
    };

    class DistanceConstraintComponent
    {
    public:
        DistanceConstraintComponent(Entity entity, Entity otherEntity, const glm::vec3& pos1, const glm::vec3& pos2, float constant = 1.0f);
        DistanceConstraintComponent(Entity entity, Entity otherEntity);
        ~DistanceConstraintComponent() = default;

        const SharedPtr<DistanceConstraint>& GetConstraint() const { return m_Constraint; }

    private:
        SharedPtr<DistanceConstraint> m_Constraint;
        Entity m_OtherEntity;
    };

    class LUMOS_EXPORT RigidBody3DComponent
    {
    public:
        RigidBody3DComponent();
        RigidBody3DComponent(const RigidBody3DComponent& other);

        explicit RigidBody3DComponent(SharedPtr<RigidBody3D>& physics);

        ~RigidBody3DComponent() = default;

        void Init();
        void Update();
        void OnImGui();

        const SharedPtr<RigidBody3D>& GetRigidBody() const
        {
            return m_RigidBody;
        }

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(*m_RigidBody.get());
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            m_RigidBody = CreateSharedPtr<RigidBody3D>();
            archive(*m_RigidBody.get());
        }

    private:
        SharedPtr<RigidBody3D> m_RigidBody;
    };
}
