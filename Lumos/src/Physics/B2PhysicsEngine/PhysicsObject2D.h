#pragma once

#include "LM.h"
#include "Physics/PhysicsObject.h"

class Object;
class b2Body;

namespace Lumos
{
    namespace maths
    {
        class Vector2;
    }
    
	class LUMOS_EXPORT PhysicsObject2D : public PhysicsObject
	{
	public:
		PhysicsObject2D();
		explicit PhysicsObject2D(const PhysicsObjectParamaters& params);

		virtual ~PhysicsObject2D();

		b2Body* GetB2Body() const { return m_B2Body; }

		void SetLinearVelocity(const maths::Vector2& v) const;
		void SetForce(const maths::Vector2& v) const;
		void SetPosition(const maths::Vector2& pos) const;

		void Init(const PhysicsObjectParamaters& params);
        
        maths::Vector2 GetPosition() const;
        float GetAngle() const;

	protected:

		b2Body* m_B2Body;
	};
}
