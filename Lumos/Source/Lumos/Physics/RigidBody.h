#pragma once

#include "Maths/Vector2.h"
#include "Maths/Vector3.h"

namespace Lumos
{

    enum class LUMOS_EXPORT Shape
    {
        Square = 0,
        Circle = 1,
        Custom = 2
    };

    struct LUMOS_EXPORT RigidBodyParameters
    {
        RigidBodyParameters()
        {
            mass = 1.0f;
            shape = Shape::Square;
            position = Maths::Vector3(0.0f);
            scale = Maths::Vector3(1.0f);
            isStatic = false;
        }

        float mass;
        Maths::Vector3 position;
        Maths::Vector3 scale;
        bool isStatic;
        Shape shape;
        std::vector<Maths::Vector2> custumShapePositions;
    };

    class LUMOS_EXPORT RigidBody
    {

    public:
        RigidBody();

        virtual ~RigidBody();

        bool GetIsStatic() const
        {
            return m_Static;
        }
        bool GetIsAtRest() const
        {
            return m_AtRest;
        }
        float GetElasticity() const
        {
            return m_Elasticity;
        }
        float GetFriction() const
        {
            return m_Friction;
        }
        virtual void SetIsStatic(const bool isStatic)
        {
            m_Static = isStatic;
        }

        void SetElasticity(const float elasticity)
        {
            m_Elasticity = elasticity;
        }
        void SetFriction(const float friction)
        {
            m_Friction = friction;
        }

        virtual void SetIsAtRest(const bool isAtRest)
        {
            m_AtRest = isAtRest;
        }
        virtual void WakeUp()
        {
            m_AtRest = false;
        }
        bool IsAwake() const
        {
            return !m_AtRest;
        }

        inline bool operator<(const RigidBody& p_r) const
        {
            return false;
        }
        inline bool operator==(const RigidBody& p_r) const
        {
            return false;
        }

    protected:
        bool m_Static;
        float m_Elasticity;
        float m_Friction;
        bool m_AtRest;
    };

}
