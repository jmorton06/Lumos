#include "LM.h"
#include "B2PhysicsEngine.h"

#include "Utilities/TimeStep.h"

#include <Box2D/Box2D.h>
#include <Box2D/Common/b2Math.h>

namespace Lumos
{
	B2PhysicsEngine::B2PhysicsEngine()
		: m_UpdateTimestep(1.0f / 60.f)
		, m_UpdateAccum(0.0f)
        , m_B2DWorld(std::make_unique<b2World>(b2Vec2(0.0f,-9.81f)))
	{
	}

	B2PhysicsEngine::~B2PhysicsEngine()
	{
	}

	void B2PhysicsEngine::SetDefaults()
	{
		m_UpdateTimestep = 1.0f / 60.f;
		m_UpdateAccum = 0.0f;
	}

	void B2PhysicsEngine::Update(bool paused, TimeStep* timeStep)
	{
		const int max_updates_per_frame = 5;

		if (!paused)
		{
			m_UpdateAccum += timeStep->GetSeconds();
			for (int i = 0; (m_UpdateAccum >= m_UpdateTimestep) && i < max_updates_per_frame; ++i)
			{
				m_UpdateAccum -= m_UpdateTimestep;
				m_B2DWorld->Step(m_UpdateTimestep, 6, 2);
			}

			if (m_UpdateAccum >= m_UpdateTimestep)
			{
				LUMOS_CORE_ERROR("Physics too slow to run in real time!");
				//Drop Time in the hope that it can continue to run in real-time
				m_UpdateAccum = 0.0f;
			}
		}
	}

	b2Body* B2PhysicsEngine::CreateB2Body(b2BodyDef* bodyDef) const
	{
		return m_B2DWorld->CreateBody(bodyDef); 
	}

	void B2PhysicsEngine::CreateFixture(b2Body* body, const b2FixtureDef* fixtureDef)
	{
		body->CreateFixture(fixtureDef);
	}
}
