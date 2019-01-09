#pragma once

#include "Utilities/TSingleton.h"

class b2World;
class b2Body;
struct b2BodyDef;
struct b2FixtureDef;

namespace jm
{
	struct TimeStep;

	class JM_EXPORT B2PhysicsEngine : public TSingleton<B2PhysicsEngine>
	{
		friend class TSingleton<B2PhysicsEngine>;

	public:
		B2PhysicsEngine();
		~B2PhysicsEngine();
		void SetDefaults();

		void Update(bool paused, TimeStep* timeStep);

		b2World* GetB2World() const { return m_B2DWorld.get(); }
		b2Body* CreateB2Body(b2BodyDef* bodyDef) const;

		static void CreateFixture(b2Body* body, const b2FixtureDef* fixtureDef);

	private:

		std::unique_ptr<b2World> m_B2DWorld;

		float m_UpdateTimestep, m_UpdateAccum;
	};
}
