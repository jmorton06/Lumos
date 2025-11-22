#pragma once

#include "Maths/Vector2.h"
#include "Maths/Vector3.h"
#include "Core/UUID.h"
#include <box2d/id.h>

namespace Lumos
{
    enum class LUMOS_EXPORT Shape
    {
        Square = 0,
        Circle = 1,
        Custom = 2
    };

    struct RigidBodyParameters
    {
        RigidBodyParameters()
        {
            mass         = 1.0f;
            shape        = Shape::Square;
            position     = Vec3(0.0f);
            scale        = Vec3(1.0f);
            isStatic     = false;
            friction     = 0.7f;
            damping      = 1.0f;
            density      = 1.0f;
            enableEvents = true;
            elasticity   = 0.0f;
        }

        float mass;
        Vec3 position;
        Vec3 scale;
        bool isStatic;
        Shape shape;
        std::vector<Vec2> customShapePositions;
        float friction;
        float damping;
        float density;
        bool enableEvents;
        float elasticity;
    };

    class RigidBody2D
    {
        template <typename Archive>
        friend void save(Archive& archive, const RigidBody2D& rigidBody);

        template <typename Archive>
        friend void load(Archive& archive, RigidBody2D& rigidBody);

    public:
        RigidBody2D();
        explicit RigidBody2D(const RigidBodyParameters& params);

        virtual ~RigidBody2D();

        b2BodyId GetB2Body() const
        {
            return m_B2Body;
        }

        void SetLinearVelocity(const Vec2& v) const;
        void SetAngularVelocity(float velocity);
        void SetForce(const Vec2& v) const;
        void SetPosition(const Vec2& pos) const;
        void SetOrientation(float angle) const;
        void SetIsStatic(bool isStatic);

        const Vec2 GetLinearVelocity() const;

        void Init(const RigidBodyParameters& params);

        Vec2 GetPosition() const;
        Vec3 GetScale() const { return m_Scale; }
        float GetAngle() const;
        Shape GetShapeType() const { return m_ShapeType; }

        void SetShape(Shape shape, const std::vector<Vec2>& customPositions = {});

        bool GetIsStatic() const { return m_Static; }
        bool GetIsAtRest() const { return m_AtRest; }
        float GetElasticity() const { return m_Elasticity; }
        float GetFriction() const;
        float GetDamping() const { return m_Damping; }
        bool IsAwake() const { return !m_AtRest; }
        void SetElasticity(const float elasticity) { m_Elasticity = elasticity; }
        void SetFriction(const float friction) { m_Friction = friction; }
        void SetDamping(const float damping) { m_Damping = damping; }
        void SetIsAtRest(const bool isAtRest) { m_AtRest = isAtRest; }
        void SetLinearDamping(float dampening);
        void SetScale(const Vec3& scale) { m_Scale = scale; }

        UUID GetUUID() const { return m_UUID; }

        void RebuildShape();

    protected:
        b2BodyId m_B2Body;
        Shape m_ShapeType;
        float m_Mass;
        float m_Angle;
        Vec3 m_Scale;
        std::vector<Vec2> m_CustomShapePositions;

        bool m_Static;
        float m_Elasticity;
        float m_Friction;
        float m_Damping;
        bool m_AtRest;
        UUID m_UUID;
    };
}
