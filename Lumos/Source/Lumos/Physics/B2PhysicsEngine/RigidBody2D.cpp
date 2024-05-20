#include "Precompiled.h"
#include "RigidBody2D.h"
#include "B2PhysicsEngine.h"
#include "Core/Application.h"

#include <glm/ext/vector_float2.hpp>
#include <box2d/box2d.h>
#include <box2d/b2_world.h>

namespace Lumos
{

    RigidBody2D::RigidBody2D()
        : m_B2Body(nullptr)
    {
        Init(RigidBodyParameters());
    }

    RigidBody2D::RigidBody2D(const RigidBodyParameters& params)
        : m_B2Body(nullptr)
    {
        Init(params);
    }

    RigidBody2D::~RigidBody2D()
    {
        if(m_B2Body && Application::Get().GetSystemManager() && Application::Get().GetSystem<B2PhysicsEngine>())
            Application::Get().GetSystem<B2PhysicsEngine>()->GetB2World()->DestroyBody(m_B2Body);
    }

    void RigidBody2D::SetLinearVelocity(const glm::vec2& v) const
    {
        m_B2Body->SetLinearVelocity(b2Vec2(v.x, v.y));
    }

    void RigidBody2D::SetAngularVelocity(float velocity)
    {
        m_B2Body->SetAngularVelocity(velocity);
    }

    void RigidBody2D::SetForce(const glm::vec2& v) const
    {
        m_B2Body->ApplyForceToCenter(b2Vec2(v.x, v.y), true);
    }

    void RigidBody2D::SetPosition(const glm::vec2& pos) const
    {
        m_B2Body->SetTransform(b2Vec2(pos.x, pos.y), m_B2Body->GetAngle());
    }

    void RigidBody2D::SetOrientation(float angle) const
    {
        m_B2Body->SetTransform(m_B2Body->GetPosition(), angle);
    }

    void RigidBody2D::SetIsStatic(bool isStatic)
    {
        m_Static = isStatic;
        m_B2Body->SetType(isStatic ? b2BodyType::b2_staticBody : b2BodyType::b2_dynamicBody);
    }

    void RigidBody2D::Init(const RigidBodyParameters& params)
    {
        LUMOS_PROFILE_FUNCTION();
        m_Static    = params.isStatic;
        m_ShapeType = params.shape;
        m_Mass      = params.mass;
        m_Scale     = params.scale;

        b2BodyDef bodyDef;
        if(params.isStatic)
            bodyDef.type = b2_staticBody;
        else
            bodyDef.type = b2_dynamicBody;

        bodyDef.linearDamping = 1.0f;
        bodyDef.position.Set(params.position.x, params.position.y);
        m_B2Body = Application::Get().GetSystem<B2PhysicsEngine>()->CreateB2Body(&bodyDef);

        if(params.shape == Shape::Circle)
        {
            b2CircleShape dynamicBox;
            dynamicBox.m_radius = params.scale.x;
            if(params.isStatic)
                m_B2Body->CreateFixture(&dynamicBox, 0.0f);
            else
            {
                b2FixtureDef fixtureDef;
                fixtureDef.shape    = &dynamicBox;
                fixtureDef.density  = 1.0f;
                fixtureDef.friction = 0.1f;
                m_B2Body->CreateFixture(&fixtureDef);
            }
        }
        else if(params.shape == Shape::Square)
        {
            b2PolygonShape dynamicBox;
            dynamicBox.SetAsBox(params.scale.x, params.scale.y);

            if(params.isStatic)
                m_B2Body->CreateFixture(&dynamicBox, 0.0f);
            else
            {
                b2FixtureDef fixtureDef;
                fixtureDef.shape    = &dynamicBox;
                fixtureDef.density  = 1.0f;
                fixtureDef.friction = 0.1f;
                m_B2Body->CreateFixture(&fixtureDef);
            }
        }
        else if(params.shape == Shape::Custom)
        {
            m_CustomShapePositions = params.customShapePositions;

            b2PolygonShape dynamicBox;
            dynamicBox.Set((b2Vec2*)params.customShapePositions.data(), int32(params.customShapePositions.size()));

            if(params.isStatic)
                m_B2Body->CreateFixture(&dynamicBox, 0.0f);
            else
            {
                b2FixtureDef fixtureDef;
                fixtureDef.shape    = &dynamicBox;
                fixtureDef.density  = 1.0f;
                fixtureDef.friction = 0.1f;
                m_B2Body->CreateFixture(&fixtureDef);
            }
        }
        else
        {
            LUMOS_LOG_ERROR("Shape Not Supported");
        }
    }

    glm::vec2 RigidBody2D::GetPosition() const
    {
        b2Vec2 pos = m_B2Body->GetPosition();
        return glm::vec2(pos.x, pos.y);
    }

    float RigidBody2D::GetAngle() const
    {
        return m_B2Body->GetAngle();
    }

    const glm::vec2 RigidBody2D::GetLinearVelocity() const
    {
        return glm::vec2(m_B2Body->GetLinearVelocity().x, m_B2Body->GetLinearVelocity().y);
    }

    void RigidBody2D::SetShape(Shape shape, const std::vector<glm::vec2>& customPositions)
    {
        LUMOS_PROFILE_FUNCTION();
        m_ShapeType            = shape;
        m_CustomShapePositions = customPositions;

        if(m_B2Body && Application::Get().GetSystem<B2PhysicsEngine>())
            Application::Get().GetSystem<B2PhysicsEngine>()->GetB2World()->DestroyBody(m_B2Body);

        RigidBodyParameters params;
        params.shape = m_ShapeType;
        if(m_B2Body)
            params.position = glm::vec3(GetPosition(), 1.0f);
        params.customShapePositions = customPositions;
        params.mass                 = m_Mass;
        params.scale                = m_Scale;
        params.isStatic             = m_Static;
        Init(params);
    }
}
