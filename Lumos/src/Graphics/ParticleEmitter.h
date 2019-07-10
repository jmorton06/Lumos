#pragma once
#include "LM.h"
#include "Maths/Maths.h"

namespace Lumos
{
	class Material;
	class Particle;
	class Camera;
	class Mesh;
	class VertexArray;
	class Texture2D;
	namespace Maths
	{
		class BoundingBox;
	}
	
	class LUMOS_EXPORT ParticleEmitter
	{
	public:

		ParticleEmitter();
		~ParticleEmitter();
		Maths::BoundingBox* CalculateBoundingBox() const;

		static Maths::Matrix4 UpdateModelViewMatrix(const Maths::Matrix4& viewMatrix, Particle* particle);

		void Update(float dt);
		void Render(Material* material, Camera* camera, Mesh* quad);

		float GetEmitterLifeTime() const;
		void SetEmitterLifeTime(float emitterLifeTime);

		float GetNextParticleTime() const;
		void SetNextParticleTime(float nextParticleTime);

		float GetParticleRate() const;
		void SetParticleRate(float particleRate);

		u32 GetNumLaunchParticles() const;
		void SetNumLaunchParticles(u32 numLaunchParticles);

		Maths::Vector3 GetArea() const;
		void SetArea(const Maths::Vector3& area);

		Maths::Vector3 GetPosition() const;
		void SetPosition(const Maths::Vector3& position);

		Maths::Vector3 GetInitialVelocity() const;
		void SetInitialVelocity(const Maths::Vector3& initialVelocity);

		float GetParticleLife() const;
		void SetParticleLife(float particleLife);

		float GetGravityEffect() const;
		void SetGravityEffect(float gravityEffect);

		float GetScale() const;
		void SetScale(float scale);

		u32 GetNumTextureRows() const;
		void SetNumTextureRows(u32 numTextureRows);

		Texture2D* GetTexture() const;
		void SetTexture(Texture2D *m_Texture);

		const std::vector<std::shared_ptr<Particle>>& GetParticles() const;
		void SetParticles(const std::vector<std::shared_ptr<Particle>>& particles);

	private:

		float m_EmitterLifeTime;
		float m_NextParticleTime;
		float m_ParticleRate;
		u32 m_NumLaunchParticles;
		Maths::Vector3 m_Area;
		Maths::Vector3 m_Position;
		Maths::Vector3 m_InitialVelocity;
		float m_ParticleLife;
		float m_GravityEffect;
		float m_Scale;
		Maths::Vector2 m_PositionVarianceX;

	public:
		Maths::Vector2 GetPositionVarianceX() const
		{
			return m_PositionVarianceX;
		}

		void SetPositionVarianceX(const Maths::Vector2& vector2)
		{
			m_PositionVarianceX = vector2;
		}

		Maths::Vector2 GetPositionVarianceY() const
		{
			return m_PositionVarianceY;
		}

		void SetPositionVarianceY(const Maths::Vector2& vector2)
		{
			m_PositionVarianceY = vector2;
		}

		Maths::Vector2 GetPositionVarianceZ() const
		{
			return m_PositionVarianceZ;
		}

		void SetPositionVarianceZ(const Maths::Vector2& vector2)
		{
			m_PositionVarianceZ = vector2;
		}

		Maths::Vector2 GetVelocityVarianceX() const
		{
			return m_VelocityVarianceX;
		}

		void SetVelocityVarianceX(const Maths::Vector2& vector2)
		{
			m_VelocityVarianceX = vector2;
		}

		Maths::Vector2 GetVelocityVarianceY() const
		{
			return m_VelocityVarianceY;
		}

		void SetVelocityVarianceY(const Maths::Vector2& vector2)
		{
			m_VelocityVarianceY = vector2;
		}

		Maths::Vector2 GetVelocityVarianceZ() const
		{
			return m_VelocityVarianceZ;
		}

		void SetVelocityVarianceZ(const Maths::Vector2& vector2)
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
		Maths::Vector2 m_PositionVarianceY;
		Maths::Vector2 m_PositionVarianceZ;
		Maths::Vector2 m_VelocityVarianceX;
		Maths::Vector2 m_VelocityVarianceY;
		Maths::Vector2 m_VelocityVarianceZ;
		float m_ScaleVariance;
		float m_LifeLengthVariance;

		u32 m_NumTextureRows;

		Texture2D* m_Texture;
		std::vector<std::shared_ptr<Particle>> m_Particles;


	};
}
