#pragma once
#include "JM.h"
#include "Maths/Maths.h"

namespace jm
{
	class Material;
	class Particle;
	class Camera;
	class Mesh;
	class VertexArray;
	class Texture2D;
	namespace maths
	{
		class BoundingBox;
	}
	
	class JM_EXPORT ParticleEmitter
	{
	public:

		ParticleEmitter();
		~ParticleEmitter();
		maths::BoundingBox* CalculateBoundingBox() const;

		static maths::Matrix4 UpdateModelViewMatrix(const maths::Matrix4& viewMatrix, Particle* particle);

		void Update(float dt);
		void Render(Material* material, Camera* camera, Mesh* quad);

		float GetEmitterLifeTime() const;
		void SetEmitterLifeTime(float emitterLifeTime);

		float GetNextParticleTime() const;
		void SetNextParticleTime(float nextParticleTime);

		float GetParticleRate() const;
		void SetParticleRate(float particleRate);

		uint GetNumLaunchParticles() const;
		void SetNumLaunchParticles(uint numLaunchParticles);

		maths::Vector3 GetArea() const;
		void SetArea(const maths::Vector3& area);

		maths::Vector3 GetPosition() const;
		void SetPosition(const maths::Vector3& position);

		maths::Vector3 GetInitialVelocity() const;
		void SetInitialVelocity(const maths::Vector3& initialVelocity);

		float GetParticleLife() const;
		void SetParticleLife(float particleLife);

		float GetGravityEffect() const;
		void SetGravityEffect(float gravityEffect);

		float GetScale() const;
		void SetScale(float scale);

		uint GetNumTextureRows() const;
		void SetNumTextureRows(uint numTextureRows);

		Texture2D* GetTexture() const;
		void SetTexture(Texture2D *m_Texture);

		const std::vector<std::shared_ptr<Particle>>& GetParticles() const;
		void SetParticles(const std::vector<std::shared_ptr<Particle>>& particles);

	private:

		float m_EmitterLifeTime;
		float m_NextParticleTime;
		float m_ParticleRate;
		uint m_NumLaunchParticles;
		maths::Vector3 m_Area;
		maths::Vector3 m_Position;
		maths::Vector3 m_InitialVelocity;
		float m_ParticleLife;
		float m_GravityEffect;
		float m_Scale;
		maths::Vector2 m_PositionVarianceX;

	public:
		maths::Vector2 GetPositionVarianceX() const
		{
			return m_PositionVarianceX;
		}

		void SetPositionVarianceX(const maths::Vector2& vector2)
		{
			m_PositionVarianceX = vector2;
		}

		maths::Vector2 GetPositionVarianceY() const
		{
			return m_PositionVarianceY;
		}

		void SetPositionVarianceY(const maths::Vector2& vector2)
		{
			m_PositionVarianceY = vector2;
		}

		maths::Vector2 GetPositionVarianceZ() const
		{
			return m_PositionVarianceZ;
		}

		void SetPositionVarianceZ(const maths::Vector2& vector2)
		{
			m_PositionVarianceZ = vector2;
		}

		maths::Vector2 GetVelocityVarianceX() const
		{
			return m_VelocityVarianceX;
		}

		void SetVelocityVarianceX(const maths::Vector2& vector2)
		{
			m_VelocityVarianceX = vector2;
		}

		maths::Vector2 GetVelocityVarianceY() const
		{
			return m_VelocityVarianceY;
		}

		void SetVelocityVarianceY(const maths::Vector2& vector2)
		{
			m_VelocityVarianceY = vector2;
		}

		maths::Vector2 GetVelocityVarianceZ() const
		{
			return m_VelocityVarianceZ;
		}

		void SetVelocityVarianceZ(const maths::Vector2& vector2)
		{
			m_VelocityVarianceZ = vector2;
		}

		float GetScaleVariance() const
		{
			return m_ScaleVariance;
		}

		void SetScaleVariance(float scale_variance)
		{
			m_ScaleVariance = scale_variance;
		}

		float GetLifeLengthScale() const
		{
			return m_LifeLengthVariance;
		}

		void SetLifeLengthScale(float life_length_scale)
		{
			m_LifeLengthVariance = life_length_scale;
		}

	private:
		maths::Vector2 m_PositionVarianceY;
		maths::Vector2 m_PositionVarianceZ;
		maths::Vector2 m_VelocityVarianceX;
		maths::Vector2 m_VelocityVarianceY;
		maths::Vector2 m_VelocityVarianceZ;
		float m_ScaleVariance;
		float m_LifeLengthVariance;

		uint m_NumTextureRows;

		Texture2D* m_Texture;
		std::vector<std::shared_ptr<Particle>> m_Particles;


	};
}
