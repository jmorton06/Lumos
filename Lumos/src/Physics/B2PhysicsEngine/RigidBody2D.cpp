#include "lmpch.h"
#include "RigidBody2D.h"
#include "B2PhysicsEngine.h"
#include "Core/Application.h"

#include "Maths/Vector2.h"

#include <Box2D/Box2D.h>
#include <Box2D/Dynamics/b2World.h>

namespace Lumos
{

	RigidBody2D::RigidBody2D()
		: m_B2Body(nullptr)
	{
	}

	RigidBody2D::RigidBody2D(const RigidBodyParamaters& params)
		: m_B2Body(nullptr)
	{
		Init(params);
	}

	RigidBody2D::~RigidBody2D()
	{
		if(m_B2Body && Application::Get().GetSystem<B2PhysicsEngine>())
			Application::Get().GetSystem<B2PhysicsEngine>()->GetB2World()->DestroyBody(m_B2Body);
	}

	void RigidBody2D::SetLinearVelocity(const Maths::Vector2& v) const
	{
		m_B2Body->SetLinearVelocity(b2Vec2(v.x, v.y));
	}

	void RigidBody2D::SetAngularVelocity(float velocity)
	{
		m_B2Body->SetAngularVelocity(velocity);
	}

	void RigidBody2D::SetForce(const Maths::Vector2& v) const
	{
		m_B2Body->ApplyForceToCenter(b2Vec2(v.x, v.y), true);
	}

	void RigidBody2D::SetPosition(const Maths::Vector2& pos) const
	{
		m_B2Body->SetTransform(b2Vec2(pos.x, pos.y), m_B2Body->GetAngle());
	}

	void RigidBody2D::SetOrientation(float angle) const
	{
		m_B2Body->SetTransform(m_B2Body->GetPosition(), angle);
	}

	void RigidBody2D::Init(const RigidBodyParamaters& params)
	{
		m_Static = params.isStatic;
		m_ShapeType = params.shape;
		m_Mass = params.mass;
		m_Scale = params.scale;

		b2BodyDef bodyDef;
		if(params.isStatic)
			bodyDef.type = b2_staticBody;
		else
			bodyDef.type = b2_dynamicBody;

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
				fixtureDef.shape = &dynamicBox;
				fixtureDef.density = 1.0f;
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
				fixtureDef.shape = &dynamicBox;
				fixtureDef.density = 1.0f;
				fixtureDef.friction = 0.1f;
				m_B2Body->CreateFixture(&fixtureDef);
			}
		}
		else if(params.shape == Shape::Custom)
		{
			b2PolygonShape dynamicBox;
			dynamicBox.SetAsBox(params.scale.x, params.scale.y);

			if(params.isStatic)
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
			Debug::Log::Error("Shape Not Supported");
		}
	}

	Maths::Vector2 RigidBody2D::GetPosition() const
	{
		b2Vec2 pos = m_B2Body->GetPosition();
		return Maths::Vector2(pos.x, pos.y);
	}

	float RigidBody2D::GetAngle() const
	{
		return m_B2Body->GetAngle();
	}
}
