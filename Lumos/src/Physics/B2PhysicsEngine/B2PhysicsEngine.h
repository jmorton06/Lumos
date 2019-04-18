#pragma once

#include "Utilities/TSingleton.h"
#include "App/ISystem.h"

class b2World;
class b2Body;
struct b2BodyDef;
struct b2FixtureDef;

namespace Lumos
{
	struct TimeStep;

	class LUMOS_EXPORT B2PhysicsEngine : public TSingleton<B2PhysicsEngine>, public ISystem
	{
		friend class TSingleton<B2PhysicsEngine>;

	public:
		B2PhysicsEngine();
		~B2PhysicsEngine();
		void SetDefaults();

		void OnUpdate(TimeStep* timeStep) override;
		void OnInit() override {};

		b2World* GetB2World() const { return m_B2DWorld.get(); }
		b2Body* CreateB2Body(b2BodyDef* bodyDef) const;

		static void CreateFixture(b2Body* body, const b2FixtureDef* fixtureDef);

		void SetPaused(bool paused) { m_Paused = paused; }
		bool IsPaused() const { return m_Paused; }
	private:

		std::unique_ptr<b2World> m_B2DWorld;

		float m_UpdateTimestep, m_UpdateAccum;
		bool m_Paused = true;
		bool m_MultipleUpdates = true;
	};
}
