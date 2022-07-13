#pragma once

#include "Physics/RigidBody.h"
#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>

class Object;
class b2Body;

namespace Lumos
{
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

        void SetLinearVelocity(const glm::vec2& v) const;
        void SetAngularVelocity(float velocity);
        void SetForce(const glm::vec2& v) const;
        void SetPosition(const glm::vec2& pos) const;
        void SetOrientation(float angle) const;
        void SetIsStatic(bool isStatic) override;

        const glm::vec2 GetLinearVelocity() const;

        void Init(const RigidBodyParameters& params);

        glm::vec2 GetPosition() const;
        float GetAngle() const;
        Shape GetShapeType() const { return m_ShapeType; }

        void SetShape(Shape shape, const std::vector<glm::vec2>& customPositions = {});

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
            glm::vec2 pos;
            archive(cereal::make_nvp("Position", pos), cereal::make_nvp("Friction", m_Friction), cereal::make_nvp("Angle", angle), cereal::make_nvp("Static", m_Static), cereal::make_nvp("Mass", m_Mass), cereal::make_nvp("Scale", params.scale), cereal::make_nvp("Shape", m_ShapeType), cereal::make_nvp("CustomShapePos", params.custumShapePositions));
            params.shape    = m_ShapeType;
            params.position = glm::vec3(pos, 1.0f);
            Init(params);
            SetOrientation(angle);
        }

    protected:
        b2Body* m_B2Body;
        Shape m_ShapeType;
        float m_Mass;
        float m_Angle;
        glm::vec3 m_Scale;
        std::vector<glm::vec2> m_CustomShapePositions;
    };
}
