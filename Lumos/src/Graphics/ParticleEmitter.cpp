#include "LM.h"
#include "ParticleEmitter.h"
#include "Material.h"
#include "Particle.h"
#include "Camera/Camera.h"
#include "Mesh.h"
#include "API/Textures/Texture2D.h"
#include "Utilities/RandomNumberGenerator.h"
#include "Maths/BoundingBox.h"

namespace Lumos
{

	ParticleEmitter::ParticleEmitter()
	: m_EmitterLifeTime(10.0f)
	, m_NextParticleTime(0.0f)
	, m_ParticleRate(0.04f)
	, m_NumLaunchParticles(5)
	, m_Area(maths::Vector3(0.0f))
	, m_Position(maths::Vector3(0.0))
	, m_InitialVelocity(maths::Vector3(0.0f, 4.0f, 0.0f))
	, m_ParticleLife(3.0f)
	, m_GravityEffect(0.0f)
	, m_Scale(2.0f)
	, m_PositionVarianceX(maths::Vector2(0.0f, 0.0f))
	, m_PositionVarianceY(maths::Vector2(0.0f, 0.0f))
	, m_PositionVarianceZ(maths::Vector2(0.0f, 0.0f))
	, m_VelocityVarianceX(maths::Vector2(0.0f, 0.0f))
	, m_VelocityVarianceY(maths::Vector2(0.0f, 0.0f))
	, m_VelocityVarianceZ(maths::Vector2(0.0f, 0.0f))
	, m_ScaleVariance(1)
	, m_LifeLengthVariance(1.0f)
	, m_NumTextureRows(1)
	, m_Texture(nullptr)
	{
	}

	ParticleEmitter::~ParticleEmitter() = default;

	maths::BoundingBox* ParticleEmitter::CalculateBoundingBox() const
	{
		maths::BoundingBox* box = new maths::BoundingBox();

		maths::Vector3 vel = m_InitialVelocity + maths::Vector3(m_VelocityVarianceX.GetY(), m_VelocityVarianceY.GetY(), m_VelocityVarianceZ.GetY());
		maths::Vector3 pos = m_Position + maths::Vector3(m_PositionVarianceX.GetY(), m_PositionVarianceY.GetY(), m_PositionVarianceZ.GetY());

		box->ExpandToFit(pos + vel * m_ParticleLife);

		vel = m_InitialVelocity + maths::Vector3(m_VelocityVarianceX.GetX(), m_VelocityVarianceY.GetX(), m_VelocityVarianceZ.GetX());
		pos = m_Position + maths::Vector3(m_PositionVarianceX.GetX(), m_PositionVarianceY.GetX(), m_PositionVarianceZ.GetX());

		box->ExpandToFit(pos + vel * m_ParticleLife);

		box->ExpandToFit(maths::Vector3(0.0f));

		//TODO: More Accurate box;

		return box;
	}

	void ParticleEmitter::Update(float dt)
	{
		m_NextParticleTime -= dt;

		if (m_NextParticleTime <= 0.0f)
		{
			for (uint i = 0; i < m_NumLaunchParticles; ++i)
			{
				m_Particles.push_back(std::make_shared<Particle>(m_Position + maths::Vector3(Lumos::RandomNumberGenerator32::Rand(-m_Area.x, m_Area.x),Lumos::RandomNumberGenerator32::Rand(-m_Area.y, m_Area.y),Lumos::RandomNumberGenerator32::Rand(-m_Area.z, m_Area.z)), m_InitialVelocity + maths::Vector3(Lumos::RandomNumberGenerator32::Rand(m_VelocityVarianceX.GetX(),m_VelocityVarianceX.GetY()),Lumos::RandomNumberGenerator32::Rand(m_VelocityVarianceY.GetX(), m_VelocityVarianceY.GetY()),Lumos::RandomNumberGenerator32::Rand(m_VelocityVarianceZ.GetX(), m_VelocityVarianceZ.GetY())), m_GravityEffect, m_ParticleLife * Lumos::RandomNumberGenerator32::Rand(m_LifeLengthVariance,1.0f), m_Scale * Lumos::RandomNumberGenerator32::Rand(m_ScaleVariance,1.0f)));
			}

			m_NextParticleTime += m_ParticleRate;
		}

		for (auto it = m_Particles.begin(); it != m_Particles.end();)
		{
			if (!(*it)->Update(dt, m_NumTextureRows))
				it = m_Particles.erase(it);
			else
				++it;
		}

	}

	void ParticleEmitter::Render(Material* material, Camera* camera, Mesh* quad)
	{
		//material->SetTexture("u_Texture", m_Texture);

		for (const auto& particle : m_Particles)
		{
            UpdateModelViewMatrix(camera->GetViewMatrix(), particle.get());
			//material->SetUniform("mvMatrix", UpdateModelViewMatrix(camera->GetViewMatrix(), particle.get()));
			//material->SetUniform("projMatrix", camera->GetProjectionMatrix());
			//material->SetUniform("u_TextureOffset1", particle->GetOffset1());
			//material->SetUniform("u_TextureOffset2", particle->GetOffset2());
			//material->SetUniform("u_TexCoordInfo", maths::Vector2(static_cast<float>(m_NumTextureRows), particle->GetBlend()));

			//material->Bind();
			quad->Draw(false);
		}
	}

	maths::Matrix4 ParticleEmitter::UpdateModelViewMatrix(const maths::Matrix4& viewMatrix, Particle* particle)
	{
		maths::Matrix4 mvMatrix;
		mvMatrix.ToIdentity();

		mvMatrix = maths::Matrix4::Translation(particle->GetPosition());
		mvMatrix[0] = viewMatrix[0];
		mvMatrix[1] = viewMatrix[4];
		mvMatrix[2] = viewMatrix[8];
		mvMatrix[4] = viewMatrix[1];
		mvMatrix[5] = viewMatrix[5];
		mvMatrix[6] = viewMatrix[9];
		mvMatrix[8] = viewMatrix[2];
		mvMatrix[9] = viewMatrix[6];
		mvMatrix[10] = viewMatrix[10];
		mvMatrix = mvMatrix * maths::Matrix4::Rotation(particle->GetRotation(), maths::Vector3(0.0f, 0.0f, 1.0f)) * maths::Matrix4::Scale(maths::Vector3(particle->GetScale()));
		mvMatrix = viewMatrix * mvMatrix;

		return mvMatrix;
	}

    float ParticleEmitter::GetEmitterLifeTime() const
    {
        return m_EmitterLifeTime;
    }

    void ParticleEmitter::SetEmitterLifeTime(float emitterLifeTime)
    {
        ParticleEmitter::m_EmitterLifeTime = emitterLifeTime;
    }

    float ParticleEmitter::GetNextParticleTime() const
    {
        return m_NextParticleTime;
    }

    void ParticleEmitter::SetNextParticleTime(float nextParticleTime)
    {
        ParticleEmitter::m_NextParticleTime = nextParticleTime;
    }

    float ParticleEmitter::GetParticleRate() const
    {
        return m_ParticleRate;
    }

    void ParticleEmitter::SetParticleRate(float particleRate)
    {
        ParticleEmitter::m_ParticleRate = particleRate;
    }

    uint ParticleEmitter::GetNumLaunchParticles() const
    {
        return m_NumLaunchParticles;
    }

    void ParticleEmitter::SetNumLaunchParticles(uint numLaunchParticles)
    {
        ParticleEmitter::m_NumLaunchParticles = numLaunchParticles;
    }

	maths::Vector3 ParticleEmitter::GetArea() const
    {
        return m_Area;
    }

    void ParticleEmitter::SetArea(const maths::Vector3 &area)
    {
        ParticleEmitter::m_Area = area;
    }

	maths::Vector3 ParticleEmitter::GetPosition() const
    {
        return m_Position;
    }

    void ParticleEmitter::SetPosition(const maths::Vector3 &position)
    {
        ParticleEmitter::m_Position = position;
    }

	maths::Vector3 ParticleEmitter::GetInitialVelocity() const
    {
        return m_InitialVelocity;
    }

    void ParticleEmitter::SetInitialVelocity(const maths::Vector3& initialVelocity)
    {
        ParticleEmitter::m_InitialVelocity = initialVelocity;
    }

    float ParticleEmitter::GetParticleLife() const
    {
        return m_ParticleLife;
    }

    void ParticleEmitter::SetParticleLife(float particleLife)
    {
        ParticleEmitter::m_ParticleLife = particleLife;
    }

    float ParticleEmitter::GetGravityEffect() const
    {
        return m_GravityEffect;
    }

    void ParticleEmitter::SetGravityEffect(float gravityEffect)
    {
        ParticleEmitter::m_GravityEffect = gravityEffect;
    }

    float ParticleEmitter::GetScale() const
    {
        return m_Scale;
    }

    void ParticleEmitter::SetScale(float scale)
    {
        ParticleEmitter::m_Scale = scale;
    }

    uint ParticleEmitter::GetNumTextureRows() const
    {
        return m_NumTextureRows;
    }

    void ParticleEmitter::SetNumTextureRows(uint numTextureRows)
    {
        ParticleEmitter::m_NumTextureRows = numTextureRows;
    }

    Texture2D *ParticleEmitter::GetTexture() const
    {
        return m_Texture;
    }

    void ParticleEmitter::SetTexture(Texture2D* texture)
    {
        ParticleEmitter::m_Texture = texture;
    }

    const std::vector<std::shared_ptr<Particle>> &ParticleEmitter::GetParticles() const
    {
        return m_Particles;
    }

    void ParticleEmitter::SetParticles(const std::vector<std::shared_ptr<Particle>> &particles)
    {
        ParticleEmitter::m_Particles = particles;
    }
}
