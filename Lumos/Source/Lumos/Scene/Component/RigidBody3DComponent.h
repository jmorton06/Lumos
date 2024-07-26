#pragma once

#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Physics/LumosPhysicsEngine/Constraints/SpringConstraint.h"
#include "Physics/LumosPhysicsEngine/Constraints/WeldConstraint.h"
#include "Physics/LumosPhysicsEngine/Constraints/DistanceConstraint.h"
#include "Physics/LumosPhysicsEngine/Constraints/AxisConstraint.h"
#include "Core/Application.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include <cereal/cereal.hpp>

namespace Lumos
{
    class Entity;
    struct RigidBody3DInstance
    {
        RigidBody3DInstance();
        RigidBody3DInstance(RigidBody3D* physics);
        RigidBody3DInstance(const RigidBody3DProperties& params);

        ~RigidBody3DInstance();
        RigidBody3D* Body;
    };

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
        SpringConstraintComponent() = default;
        SpringConstraintComponent(Entity entity, Entity otherEntity, const Vec3& pos1, const Vec3& pos2, float constant = 0.9f);
        SpringConstraintComponent(Entity entity, Entity otherEntity);
        ~SpringConstraintComponent() = default;

        const SharedPtr<SpringConstraint>& GetConstraint() const { return m_Constraint; }

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(m_EntityID, m_OtherEntityID, m_Position1, m_Position2, m_Constant);
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            archive(m_EntityID, m_OtherEntityID, m_Position1, m_Position2, m_Constant);
            m_Initialised = false;
        }

        void Initialise();
        bool Initialised() const { return m_Initialised; }

        uint64_t GetEntityID() { return m_EntityID; }
        uint64_t GetOtherEntityID() { return m_OtherEntityID; }

        void SetEntityID(uint64_t entityID)
        {
            m_EntityID    = entityID;
            m_Initialised = false;
        }
        void SetOtherEntityID(uint64_t entityID)
        {
            m_OtherEntityID = entityID;
            m_Initialised   = false;
        }

        const Vec3& getPosition1() const { return m_Position1; }
        void SetPosition1(const Vec3& position)
        {
            m_Position1   = position;
            m_Initialised = false;
        }
        const Vec3& GetPosition2() const { return m_Position2; }
        void SetPosition2(const Vec3& position)
        {
            m_Position2   = position;
            m_Initialised = false;
        }
        float GetConstant() const { return m_Constant; }
        void SetConstant(float constant)
        {
            m_Constant    = constant;
            m_Initialised = false;
        }

    private:
        SharedPtr<SpringConstraint> m_Constraint;
        uint64_t m_EntityID;
        uint64_t m_OtherEntityID;
        Vec3 m_Position1;
        Vec3 m_Position2;
        float m_Constant   = 1.0f;
        bool m_Initialised = false;
    };

    class WeldConstraintComponent
    {
    public:
        WeldConstraintComponent(Entity entity, Entity otherEntity, const Vec3& pos1, const Vec3& pos2, float constant = 1.0f);
        WeldConstraintComponent(Entity entity, Entity otherEntity);
        ~WeldConstraintComponent() = default;

        const SharedPtr<WeldConstraint>& GetConstraint() const { return m_Constraint; }

    private:
        SharedPtr<WeldConstraint> m_Constraint;
        uint64_t m_OtherEntityID;
    };

    class DistanceConstraintComponent
    {
    public:
        DistanceConstraintComponent(Entity entity, Entity otherEntity, const Vec3& pos1, const Vec3& pos2, float constant = 1.0f);
        DistanceConstraintComponent(Entity entity, Entity otherEntity);
        ~DistanceConstraintComponent() = default;

        const SharedPtr<DistanceConstraint>& GetConstraint() const { return m_Constraint; }

    private:
        SharedPtr<DistanceConstraint> m_Constraint;
        uint64_t m_OtherEntityID;
    };

    class LUMOS_EXPORT RigidBody3DComponent
    {
    public:
        RigidBody3DComponent();
        RigidBody3DComponent(const RigidBody3DComponent& other);
        RigidBody3DComponent(RigidBody3D* physics);
        RigidBody3DComponent(const RigidBody3DProperties& params);

        ~RigidBody3DComponent();

        inline RigidBody3DComponent& operator=(RigidBody3DComponent& moving)
        {
            m_OwnRigidBody        = moving.m_OwnRigidBody;
            moving.m_OwnRigidBody = false;
            m_RigidBody           = moving.m_RigidBody;

            return *this;
        }

        inline RigidBody3DComponent& operator=(RigidBody3DComponent&& rhs) noexcept
        {
            m_OwnRigidBody     = rhs.m_OwnRigidBody;
            rhs.m_OwnRigidBody = false;
            m_RigidBody        = rhs.m_RigidBody;

            return *this;
        }

        void Init();
        void Update();
        void OnImGui();

        RigidBody3D* GetRigidBody() const
        {
            return m_RigidBody->Body;
        }

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(*(m_RigidBody->Body));
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            // m_RigidBody = CreateSharedPtr<RigidBody3D>();
            m_RigidBody = CreateSharedPtr<RigidBody3DInstance>();
            archive(*(m_RigidBody->Body));
        }

    private:
        SharedPtr<RigidBody3DInstance> m_RigidBody;
        bool m_OwnRigidBody = false;
    };
}
