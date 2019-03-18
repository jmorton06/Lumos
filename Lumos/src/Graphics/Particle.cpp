#include "LM.h"
#include "Particle.h"

namespace Lumos
{

	Particle::Particle()
		: m_Position(maths::Vector3(0.0f))
		  , m_Velocity(maths::Vector3(10.0f))
		  , m_GravityEffect(1.0f)
		  , m_LifeLength(5.0f)
		  , m_Rotation(0.0f)
		  , m_Scale(1.0f)
		  , m_ElapsedTime(0.0f)
		  , m_TextureOffset1(maths::Vector2(0.0f))
		  , m_TextureOffset2(maths::Vector2(0.0f)), m_Blend(0)
	{
	}

	Particle::Particle(const maths::Vector3& position, const maths::Vector3& velocity, float gravityEffect, float lifeLength, float scale)
		: m_Position(position)
		  , m_Velocity(velocity)
		  , m_GravityEffect(gravityEffect)
		  , m_LifeLength(lifeLength)
		  , m_Rotation(0.0f)
		  , m_Scale(scale)
		  , m_ElapsedTime(0.0f), m_Blend(0)
	{
	}

	Particle::~Particle()
	{

	}

	bool Particle::Update(float dt, uint numTextureRows)
	{
		UpdateTextureCoord(numTextureRows);

		m_Velocity.SetY(m_Velocity.GetY() + GRAVITY * m_GravityEffect * dt);
		m_Position += m_Velocity * dt;
		m_ElapsedTime += dt;

		return m_ElapsedTime < m_LifeLength;
	}

	void Particle::UpdateTextureCoord(uint textureRows)
	{
		float lifeFactor = m_ElapsedTime / m_LifeLength;
		int stageCount = textureRows * textureRows;
		float atlasProgression = lifeFactor * stageCount;
		int index1 = static_cast<int>(floor(atlasProgression));
		int index2 = index1 < stageCount - 1 ? index1 + 1 : index1;
		m_Blend = static_cast<float>(fmod(atlasProgression, 1));
		SetTextureOffset(m_TextureOffset1, index1, textureRows);
		SetTextureOffset(m_TextureOffset2, index2, textureRows);
	}

	void Particle::SetTextureOffset(maths::Vector2& offset, int index, uint textureRows) const
	{
		int column = static_cast<int>(fmod(index, textureRows));
		int row = index / textureRows;
		offset.SetX(static_cast<float>(column) / static_cast<float>(textureRows));
		offset.SetY(static_cast<float>(row) / static_cast<float>(textureRows));
	}
}