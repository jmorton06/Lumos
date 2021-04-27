#pragma once

#include "Physics/RigidBody.h"
#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>

class Object;
class b2Body;

namespace Lumos
{
    namespace Maths
    {
        class Vector2;
    }

    class LUMOS_EXPORT RigidBody2D : public RigidBody
    {
    public:
        RigidBody2D();
        explicit RigidBody2D(const RigidBodyParameters& params);

        virtual ~RigidBody2D();

        b2Body* GetB2Body() const
        {
            return m_B2Body;
        }

        b2Body& GetB2BodyRef() const
        {
            return *m_B2Body;
        }

        void SetLinearVelocity(const Maths::Vector2& v) const;
        void SetAngularVelocity(float velocity);
        void SetForce(const Maths::Vector2& v) const;
        void SetPosition(const Maths::Vector2& pos) const;
        void SetOrientation(float angle) const;
        void SetIsStatic(bool isStatic) override;

        const Maths::Vector2 GetLinearVelocity() const;

        void Init(const RigidBodyParameters& params);

        Maths::Vector2 GetPosition() const;
        float GetAngle() const;
        Shape GetShapeType() const { return m_ShapeType; }

        void SetShape(Shape shape, const std::vector<Maths::Vector2>& customPositions = {});

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
            Maths::Vector2 pos;
            archive(cereal::make_nvp("Position", pos), cereal::make_nvp("Friction", m_Friction), cereal::make_nvp("Angle", angle), cereal::make_nvp("Static", m_Static), cereal::make_nvp("Mass", m_Mass), cereal::make_nvp("Scale", params.scale), cereal::make_nvp("Shape", m_ShapeType), cereal::make_nvp("CustomShapePos", params.custumShapePositions));
            params.shape = m_ShapeType;
            params.position = Maths::Vector3(pos, 1.0f);
            Init(params);
            SetOrientation(angle);
        }

    protected:
        b2Body* m_B2Body;
        Shape m_ShapeType;
        float m_Mass;
        float m_Angle;
        Maths::Vector3 m_Scale;
        std::vector<Maths::Vector2> m_CustomShapePositions;
    };
}
