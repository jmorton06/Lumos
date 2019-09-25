#pragma once

#include "lmpch.h"
#include "Physics/PhysicsObject.h"

class Object;
class b2Body;

namespace Lumos
{
    namespace Maths
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

		void SetLinearVelocity(const Maths::Vector2& v) const;
		void SetForce(const Maths::Vector2& v) const;
		void SetPosition(const Maths::Vector2& pos) const;
        void SetOrientation(float angle) const;

		void Init(const PhysicsObjectParamaters& params);
        
        Maths::Vector2 GetPosition() const;
        float GetAngle() const;

		nlohmann::json Serialise() override;
		void Deserialise(nlohmann::json& data) override;

	protected:

		b2Body* m_B2Body;
	};
}
