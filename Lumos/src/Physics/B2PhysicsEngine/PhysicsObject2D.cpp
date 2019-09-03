#include "LM.h"
#include "PhysicsObject2D.h"
#include "B2PhysicsEngine.h"
#include "App/Application.h"

#include "Maths/Vector2.h"

#include <Box2D/Box2D.h>
#include <Box2D/Dynamics/b2World.h>

namespace Lumos
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
		Application::Instance()->GetSystem<B2PhysicsEngine>()->GetB2World()->DestroyBody(m_B2Body);
	}

	void PhysicsObject2D::SetLinearVelocity(const Maths::Vector2& v) const
	{
		m_B2Body->SetLinearVelocity(b2Vec2(v.GetX(), v.GetY()));
	}

	void PhysicsObject2D::SetForce(const Maths::Vector2& v) const
	{
        m_B2Body->ApplyForceToCenter(b2Vec2(v.GetX(),v.GetY()), true);
	}

	void PhysicsObject2D::SetPosition(const Maths::Vector2 & pos) const
	{
        m_B2Body->SetTransform(b2Vec2(pos.GetX(), pos.GetY()), m_B2Body->GetAngle());

	}
    
    void PhysicsObject2D::SetOrientation(float angle) const
    {
        m_B2Body->SetTransform(m_B2Body->GetPosition(), angle);
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
		m_B2Body = Application::Instance()->GetSystem<B2PhysicsEngine>()->CreateB2Body(&bodyDef);

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
			LUMOS_LOG_CRITICAL("Shape Not Supported");
		}
	}
    
    Maths::Vector2 PhysicsObject2D::GetPosition() const
    {
        b2Vec2 pos = m_B2Body->GetPosition();
        return Maths::Vector2(pos.x,pos.y);
    }
    
    float PhysicsObject2D::GetAngle() const
    {
        return m_B2Body->GetAngle();
    }

	nlohmann::json PhysicsObject2D::Serialise()
	{
		nlohmann::json output;
		output["typeID"] = LUMOS_TYPENAME(PhysicsObject2D);
		output["position"] = GetPosition().Serialise();
		output["orientation"] = GetAngle();
		output["type"] = m_B2Body->GetType();
		//output["shape"] = m_B2Body->Getshape();

		return output;
	}

	void PhysicsObject2D::Deserialise(nlohmann::json& data)
	{
		PhysicsObjectParamaters params;
		params.position.Deserialise(data["position"]);

		Init(params);

		m_B2Body->SetType(data["type"]);
	}
}
