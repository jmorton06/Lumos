#pragma once

#include "lmpch.h"
#include "Physics/RigidBody.h"
#include <cereal/cereal.hpp>

class Object;
class b2Body;

namespace Lumos
{
	namespace Maths
	{
		class Vector2;
	}

	class LUMOS_EXPORT RigidBody2D : public RigidBody
	{
	public:
		RigidBody2D();
		explicit RigidBody2D(const RigidBodyParamaters& params);

		virtual ~RigidBody2D();

		b2Body* GetB2Body() const
		{
			return m_B2Body;
		}

		b2Body& GetB2BodyRef() const
		{
			return *m_B2Body;
		}

		void SetLinearVelocity(const Maths::Vector2& v) const;
		void SetAngularVelocity(float velocity);
		void SetForce(const Maths::Vector2& v) const;
		void SetPosition(const Maths::Vector2& pos) const;
		void SetOrientation(float angle) const;

		void Init(const RigidBodyParamaters& params);

		Maths::Vector2 GetPosition() const;
		float GetAngle() const;

		template<typename Archive>
		void save(Archive& archive) const
		{
			archive(cereal::make_nvp("Position", GetPosition()), cereal::make_nvp("Friction", m_Friction), cereal::make_nvp("Angle", GetAngle()), cereal::make_nvp("Static", GetIsStatic()), cereal::make_nvp("Mass", m_Mass), cereal::make_nvp("Scale", m_Scale));
		}

		template<typename Archive>
		void load(Archive& archive)
		{
			RigidBodyParamaters params;
			float angle;
			archive(cereal::make_nvp("Position", params.position), cereal::make_nvp("Friction", m_Friction), cereal::make_nvp("Angle", angle), cereal::make_nvp("Static", m_Static), cereal::make_nvp("Mass", m_Mass), cereal::make_nvp("Scale", params.scale), cereal::make_nvp("Shape", m_ShapeType));
			params.shape = m_ShapeType;
			Init(params);
			SetOrientation(angle);
		}

	protected:
		b2Body* m_B2Body;
		Shape m_ShapeType;
		float m_Mass;
		float m_Angle;
		Maths::Vector3 m_Scale;
	};
}