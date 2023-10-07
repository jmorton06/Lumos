#pragma once

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "Core/UUID.h"
class Object;
class b2Body;

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
            position = glm::vec3(0.0f);
            scale    = glm::vec3(1.0f);
            isStatic = false;
        }

        float mass;
        glm::vec3 position;
        glm::vec3 scale;
        bool isStatic;
        Shape shape;
        std::vector<glm::vec2> customShapePositions;
    };

    class LUMOS_EXPORT RigidBody2D
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
        void SetIsStatic(bool isStatic);

        const glm::vec2 GetLinearVelocity() const;

        void Init(const RigidBodyParameters& params);

        glm::vec2 GetPosition() const;
        float GetAngle() const;
        Shape GetShapeType() const { return m_ShapeType; }

        void SetShape(Shape shape, const std::vector<glm::vec2>& customPositions = {});

        bool GetIsStatic() const { return m_Static; }
        bool GetIsAtRest() const { return m_AtRest; }
        float GetElasticity() const { return m_Elasticity; }
        float GetFriction() const { return m_Friction; }
        bool IsAwake() const { return !m_AtRest; }
        void SetElasticity(const float elasticity) { m_Elasticity = elasticity; }
        void SetFriction(const float friction) { m_Friction = friction; }
        void SetIsAtRest(const bool isAtRest) { m_AtRest = isAtRest; }

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
            glm::vec2 pos;
            archive(cereal::make_nvp("Position", pos), cereal::make_nvp("Friction", m_Friction), cereal::make_nvp("Angle", angle), cereal::make_nvp("Static", m_Static), cereal::make_nvp("Mass", m_Mass), cereal::make_nvp("Scale", params.scale), cereal::make_nvp("Shape", m_ShapeType), cereal::make_nvp("CustomShapePos", params.customShapePositions));
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

        bool m_Static;
        float m_Elasticity;
        float m_Friction;
        bool m_AtRest;
        UUID m_UUID;
    };
}
