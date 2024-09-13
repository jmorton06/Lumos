#pragma once

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include "Maths/Vector2.h"
#include "Maths/Vector3.h"
#include "Core/UUID.h"
#include <box2d/id.h>

class Object;

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
            mass     = 1.0f;
            shape    = Shape::Square;
            position = Vec3(0.0f);
            scale    = Vec3(1.0f);
            isStatic = false;
        }

        float mass;
        Vec3 position;
        Vec3 scale;
        bool isStatic;
        Shape shape;
        std::vector<Vec2> customShapePositions;
    };

    class LUMOS_EXPORT RigidBody2D
    {
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
        float GetAngle() const;
        Shape GetShapeType() const { return m_ShapeType; }

        void SetShape(Shape shape, const std::vector<Vec2>& customPositions = {});

        bool GetIsStatic() const { return m_Static; }
        bool GetIsAtRest() const { return m_AtRest; }
        float GetElasticity() const { return m_Elasticity; }
        float GetFriction() const { return m_Friction; }
        bool IsAwake() const { return !m_AtRest; }
        void SetElasticity(const float elasticity) { m_Elasticity = elasticity; }
        void SetFriction(const float friction) { m_Friction = friction; }
        void SetIsAtRest(const bool isAtRest) { m_AtRest = isAtRest; }
        void SetLinearDamping(float dampening);

        UUID GetUUID() const { return m_UUID; }

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(cereal::make_nvp("Position", GetPosition()), cereal::make_nvp("Friction", m_Friction), cereal::make_nvp("Angle", GetAngle()), cereal::make_nvp("Static", GetIsStatic()), cereal::make_nvp("Mass", m_Mass), cereal::make_nvp("Scale", m_Scale),
                    cereal::make_nvp("Shape", m_ShapeType), cereal::make_nvp("CustomShapePos", m_CustomShapePositions));
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            RigidBodyParameters params;
            float angle;
            Vec2 pos;
            archive(cereal::make_nvp("Position", pos), cereal::make_nvp("Friction", m_Friction), cereal::make_nvp("Angle", angle), cereal::make_nvp("Static", m_Static), cereal::make_nvp("Mass", m_Mass), cereal::make_nvp("Scale", params.scale), cereal::make_nvp("Shape", m_ShapeType), cereal::make_nvp("CustomShapePos", params.customShapePositions));
            params.shape    = m_ShapeType;
            params.position = Vec3(pos, 1.0f);
            Init(params);
            SetOrientation(angle);
        }

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
        bool m_AtRest;
        UUID m_UUID;
    };
}
