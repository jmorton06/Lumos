#include "JM.h"
#include "PhysicsObject2D.h"
#include "B2PhysicsEngine.h"

#include "Maths/Vector2.h"

#include <Box2D/Box2D.h>
#include <Box2D/Dynamics/b2World.h>

namespace jm
{

	PhysicsObject2D::PhysicsObject2D() : m_B2Body(nullptr)
	{
	}

	PhysicsObject2D::PhysicsObject2D(const PhysicsObjectParamaters& params) : m_B2Body(nullptr)
	{
		Init(params);
	}

	PhysicsObject2D::~PhysicsObject2D()
	{
		B2PhysicsEngine::Instance()->GetB2World()->DestroyBody(m_B2Body);
	}

	void PhysicsObject2D::SetLinearVelocity(const maths::Vector2& v) const
	{
		m_B2Body->SetLinearVelocity(b2Vec2(v.GetX(), v.GetY()));
	}

	void PhysicsObject2D::SetForce(const maths::Vector2& v) const
	{
		//m_B2Body->SetForce(b2Vec2(v.x, v.y));
	}

	void PhysicsObject2D::Init(const PhysicsObjectParamaters& params)
	{
		m_Static = params.isStatic;

		b2BodyDef bodyDef;
		if (params.isStatic)
			bodyDef.type = b2_staticBody;
		else
			bodyDef.type = b2_dynamicBody;

		bodyDef.position.Set(params.position.GetX(), params.position.GetY());
		m_B2Body = B2PhysicsEngine::Instance()->CreateB2Body(&bodyDef);

		if (params.shape == Shape::Circle)
		{
			b2CircleShape dynamicBox;
			dynamicBox.m_radius = params.scale.GetX();
			if (params.isStatic)
				m_B2Body->CreateFixture(&dynamicBox, 0.0f);
			else
			{
				b2FixtureDef fixtureDef;
				fixtureDef.shape = &dynamicBox;
				fixtureDef.density = 1.0f;
				fixtureDef.friction = 0.1f;
				m_B2Body->CreateFixture(&fixtureDef);
			}
		}
		else if (params.shape == Shape::Square)
		{
			b2PolygonShape dynamicBox;
			dynamicBox.SetAsBox(params.scale.GetX(), params.scale.GetY());

			if (params.isStatic)
				m_B2Body->CreateFixture(&dynamicBox, 0.0f);
			else
			{
				b2FixtureDef fixtureDef;
				fixtureDef.shape = &dynamicBox;
				fixtureDef.density = 1.0f;
				fixtureDef.friction = 0.1f;
				m_B2Body->CreateFixture(&fixtureDef);
			}
		}
		else if (params.shape == Shape::Custom)
		{
			b2PolygonShape dynamicBox;
			dynamicBox.SetAsBox(params.scale.GetX(), params.scale.GetY());

			if (params.isStatic)
				m_B2Body->CreateFixture(&dynamicBox, 0.0f);
			else
			{
				b2FixtureDef fixtureDef;
				fixtureDef.shape = &dynamicBox;
				fixtureDef.density = 1.0f;
				fixtureDef.friction = 0.1f;
				m_B2Body->CreateFixture(&fixtureDef);
			}
		}
		else
		{
			JM_CORE_ERROR("Shape Not Supported");
		}
	}
    
    maths::Vector2 PhysicsObject2D::GetPosition() const
    {
        b2Vec2 pos = m_B2Body->GetPosition();
        return maths::Vector2(pos.x,pos.y);
    }
    
    float PhysicsObject2D::GetAngle() const
    {
        return m_B2Body->GetAngle();
    }
}
