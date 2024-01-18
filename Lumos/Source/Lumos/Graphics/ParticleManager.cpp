#include "Precompiled.h"
#include "ParticleManager.h"
#include "Maths/Random.h"
#include "Maths/MathsUtilities.h"
#include "Graphics/RHI/Texture.h"

namespace Lumos
{
    ParticleEmitter::ParticleEmitter()
    {
        m_ParticleCount = 1024;
        Init();
    }

    ParticleEmitter::ParticleEmitter(uint32_t amount)
    {
        m_ParticleCount = amount;
        Init();
    }

    void ParticleEmitter::Update(float dt, glm::vec3 emitterPosition)
    {
        LUMOS_PROFILE_FUNCTION();

        if(!m_Arena || !m_Particles || m_ParticleCount > m_Arena->Size / sizeof(Particle))
        {
            m_ParticleCount = 1024;
            Init();
        }
        m_NextParticleTime -= dt;

        if(m_NextParticleTime <= 0.0f)
        {
            for(uint32_t i = 0; i < m_NumLaunchParticles; i++)
            {
                int unusedParticle = FirstUnusedParticle();
                RespawnParticle(m_Particles[unusedParticle], emitterPosition);
            }

            m_NextParticleTime += m_ParticleRate;
        }

        // update all particles
        for(uint32_t i = 0; i < m_ParticleCount; i++)
        {
            Particle& p = m_Particles[i];
            p.Life -= dt;
            if(p.Life > 0.0f)
            {
                p.Velocity += m_Gravity * dt;
                p.Position += p.Velocity * dt;

                if(m_FadeIn > 0.0f && p.Life > (m_ParticleLife - m_FadeIn))
                    p.Colour.a = (m_ParticleLife - p.Life) / m_FadeIn;
                else if(m_FadeOut > 0.0f && p.Life < m_FadeOut)
                    p.Colour.a = 1.0f - ((m_FadeOut - p.Life) / m_FadeOut);
            }
        }
    }

    void ParticleEmitter::Init()
    {
        if(m_Arena)
            ArenaRelease(m_Arena);

        m_Arena     = ArenaAlloc((m_ParticleCount + 10) * sizeof(Particle));
        m_Particles = PushArray(m_Arena, Particle, m_ParticleCount);
    }

    void ParticleEmitter::RespawnParticle(Particle& particle, glm::vec3 emitterPosition)
    {
        LUMOS_PROFILE_FUNCTION_LOW();

        particle.Position = glm::vec3(m_Spread.x > Maths::M_EPSILON ? Random32::Rand(-m_Spread.x, m_Spread.x) : 0.0f, m_Spread.y > Maths::M_EPSILON ? Random32::Rand(-m_Spread.y, m_Spread.y) : 0.0f, m_Spread.z > Maths::M_EPSILON ? Random32::Rand(-m_Spread.z, m_Spread.z) : 0.0f) + emitterPosition;
        particle.Colour   = m_InitialColour;
        particle.Life     = m_ParticleLife + Random32::Rand(-m_LifeSpread, m_LifeSpread);
        particle.Velocity = m_InitialVelocity + glm::vec3(m_VelocitySpread.x > Maths::M_EPSILON ? Random32::Rand(-m_VelocitySpread.x, m_VelocitySpread.x) : 0.0f, m_VelocitySpread.y > Maths::M_EPSILON ? Random32::Rand(-m_VelocitySpread.y, m_VelocitySpread.y) : 0.0f, m_VelocitySpread.z > Maths::M_EPSILON ? Random32::Rand(-m_VelocitySpread.z, m_VelocitySpread.z) : 0.0f);
        particle.Size     = m_ParticleSize;
    }

    uint32_t lastUsedParticle = 0;
    uint32_t ParticleEmitter::FirstUnusedParticle()
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        for(uint32_t i = lastUsedParticle; i < m_ParticleCount; ++i)
        {
            if(m_Particles[i].Life <= 0.0f)
            {
                lastUsedParticle = i;
                return i;
            }
        }

        for(uint32_t i = 0; i < lastUsedParticle; ++i)
        {
            if(m_Particles[i].Life <= 0.0f)
            {
                lastUsedParticle = i;
                return i;
            }
        }
        lastUsedParticle = 0;
        return 0;
    }

    void ParticleEmitter::SetTextureFromFile(const std::string& filePath)
    {
        auto tex = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(filePath, filePath));
        if(tex)
        {
            m_Texture = tex;
        }
    }

    void ParticleEmitter::SetParticleCount(uint32_t particleCount)
    {
        m_ParticleCount = particleCount;
        Init();
    }

    const std::array<glm::vec2, 4>& ParticleEmitter::GetDefaultUVs()
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        static std::array<glm::vec2, 4> results;
        {
            results[0] = glm::vec2(0, 1);
            results[1] = glm::vec2(1, 1);
            results[2] = glm::vec2(1, 0);
            results[3] = glm::vec2(0, 0);
        }
        return results;
    }

    std::array<glm::vec2, 4> ParticleEmitter::GetAnimatedUVs(float currentLife, int numRows)
    {
        if(numRows <= 0)
            return GetDefaultUVs();

        // Assumes numRows = numCols
        std::array<glm::vec2, 4> results;

        // Assuming normalized life value in the range [0, 1]
        float normalizedLife = glm::clamp(currentLife, 0.0f, 1.0f);

        // Calculate the total number of frames
        int totalFrames = numRows * numRows;

        // Calculate the current frame index based on the normalized life and total number of frames
        int currentFrame = static_cast<int>(normalizedLife * totalFrames);

        // Calculate the current row and column based on the current frame
        int currentRow = currentFrame / numRows;
        int currentCol = currentFrame % numRows;

        // Calculate the UV coordinates for the current frame
        float frameWidth  = 1.0f / static_cast<float>(numRows);
        float frameHeight = 1.0f / static_cast<float>(numRows);

        results[0] = glm::vec2(currentCol * frameWidth, 1.0f - ((currentRow + 1) * frameHeight));
        results[1] = glm::vec2((currentCol + 1) * frameWidth, 1.0f - ((currentRow + 1) * frameHeight));
        results[2] = glm::vec2((currentCol + 1) * frameWidth, 1.0f - ((currentRow)*frameHeight));
        results[3] = glm::vec2(currentCol * frameWidth, 1.0f - ((currentRow)*frameHeight));

        return results;
    }

    std::array<glm::vec4, 4> ParticleEmitter::GetBlendedAnimatedUVs(float currentLife, int numRows, float& outBlendAmount)
    {
        // Assumes numRows = numCols
        std::array<glm::vec4, 4> results;

        // Assuming normalized life value in the range [0, 1]
        float normalizedLife = glm::clamp(currentLife, 0.0f, 1.0f);

        // Calculate the total number of frames
        int totalFrames = numRows * numRows;

        // Calculate the current frame index based on the normalized life and total number of frames
        int currentFrame = static_cast<int>(normalizedLife * totalFrames);

        // Calculate the current row and column based on the current frame
        int currentRow = currentFrame / numRows;
        int currentCol = currentFrame % numRows;

        // Calculate the UV coordinates for the current frame
        float frameWidth  = 1.0f / static_cast<float>(numRows);
        float frameHeight = 1.0f / static_cast<float>(numRows);

        results[0].x = currentCol * frameWidth;
        results[1].x = (currentCol + 1) * frameWidth;
        results[2].x = (currentCol + 1) * frameWidth;
        results[3].x = currentCol * frameWidth;

        results[0].y = ((currentRow + 1) * frameHeight);
        results[1].y = ((currentRow + 1) * frameHeight);
        results[2].y = ((currentRow)*frameHeight);
        results[3].y = ((currentRow)*frameHeight);

        // Calculate the UV coordinates for the next tile to blend to
        int nextCol = (currentCol + 1) % numRows;
        int nextRow = currentRow + (nextCol == 0 ? 1 : 0);

        results[0].z = nextCol * frameWidth;
        results[1].z = (nextCol + 1) * frameWidth;
        results[2].z = (nextCol + 1) * frameWidth;
        results[3].z = nextCol * frameWidth;

        results[0].w = ((nextRow + 1) * frameHeight);
        results[1].w = ((nextRow + 1) * frameHeight);
        results[2].w = ((nextRow)*frameHeight);
        results[3].w = ((nextRow)*frameHeight);

        // Set blend amount
        outBlendAmount = fmod(normalizedLife * totalFrames, 1.0f); // / static_cast<float>(totalFrames);

        return results;
    }
}
