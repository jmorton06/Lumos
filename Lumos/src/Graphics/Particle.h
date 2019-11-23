#pragma once
#include "lmpch.h"
#include "Maths/Maths.h"

#define GRAVITY -9.8f

namespace Lumos
{

	class LUMOS_EXPORT Particle
	{
	public:
		Particle();
		Particle(const Maths::Vector3& position, const Maths::Vector3& velocity, float gravityEffect, float lifeLength, float scale = 1.0f);
		~Particle();

		bool Update(float dt, u32 numTextureRows);
		void UpdateTextureCoord(u32 textureRows);
		void SetTextureOffset(Maths::Vector2& offset, int index, u32 textureRows) const;

		_FORCE_INLINE_ Maths::Vector3 GetPosition() const { return m_Position; }
		_FORCE_INLINE_ Maths::Vector3 GetVelocity() const { return m_Velocity; }
		_FORCE_INLINE_ float GetGravityEffect() const { return m_GravityEffect; }
		_FORCE_INLINE_ float GetLifeLength() const { return m_LifeLength; }
		_FORCE_INLINE_ float GetRotation() const { return m_Rotation; }
		_FORCE_INLINE_ float GetScale() const { return m_Scale; }
		_FORCE_INLINE_ float GetElapsedTime() const { return m_ElapsedTime; }
		_FORCE_INLINE_ float GetBlend() const { return m_Blend; }
		_FORCE_INLINE_ Maths::Vector2 GetOffset1() const { return m_TextureOffset1; }
		_FORCE_INLINE_ Maths::Vector2 GetOffset2() const { return m_TextureOffset2; }

	private:

		Maths::Vector3 m_Position;
		Maths::Vector3 m_Velocity;
		float m_GravityEffect;
		float m_LifeLength;
		float m_Rotation;
		float m_Scale;
		float m_ElapsedTime;
		Maths::Vector2 m_TextureOffset1;
		Maths::Vector2 m_TextureOffset2;
		float m_Blend;

	};
}
