#pragma once
#include "LM.h"
#include "Maths/Maths.h"

#define GRAVITY -9.8f

namespace lumos
{

	class LUMOS_EXPORT Particle
	{
	public:
		Particle();
		Particle(const maths::Vector3& position, const maths::Vector3& velocity, float gravityEffect, float lifeLength, float scale = 1.0f);
		~Particle();

		bool Update(float dt, uint numTextureRows);
		void UpdateTextureCoord(uint textureRows);
		void SetTextureOffset(maths::Vector2& offset, int index, uint textureRows) const;

		inline maths::Vector3 GetPosition() const { return m_Position; }
		inline maths::Vector3 GetVelocity() const { return m_Velocity; }
		inline float GetGravityEffect() const { return m_GravityEffect; }
		inline float GetLifeLength() const { return m_LifeLength; }
		inline float GetRotation() const { return m_Rotation; }
		inline float GetScale() const { return m_Scale; }
		inline float GetElapsedTime() const { return m_ElapsedTime; }
		inline float GetBlend() const { return m_Blend; }
		inline maths::Vector2 GetOffset1() const { return m_TextureOffset1; }
		inline maths::Vector2 GetOffset2() const { return m_TextureOffset2; }

	private:

		maths::Vector3 m_Position;
		maths::Vector3 m_Velocity;
		float m_GravityEffect;
		float m_LifeLength;
		float m_Rotation;
		float m_Scale;
		float m_ElapsedTime;
		maths::Vector2 m_TextureOffset1;
		maths::Vector2 m_TextureOffset2;
		float m_Blend;

	};
}
