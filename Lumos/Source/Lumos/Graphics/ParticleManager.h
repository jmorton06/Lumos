#pragma once
#include "Maths/Vector2.h"
#include "Maths/Vector3.h"
#include "Maths/Vector4.h"
#include "Graphics/RHI/RHIDefinitions.h"
#include "Graphics/RHI/Texture.h"

namespace Lumos
{
    struct Particle
    {
        Vec3 Position, Velocity;
        Vec4 Colour;
        float Life;
        float Size;

        Particle()
            : Position(0.0f)
            , Velocity(0.0f)
            , Colour(1.0f)
            , Life(0.0f)
            , Size(0.1f)
        {
        }
    };

    class ParticleEmitter
    {
		template <typename Archive>
		friend void save(Archive& archive, const ParticleEmitter& particleManager);

		template <typename Archive>
		friend void load(Archive& archive, ParticleEmitter& particleManager);

    public:
        ParticleEmitter();
        ParticleEmitter(uint32_t amount);

        enum BlendType : uint8_t
        {
            Additive = 0,
            Alpha    = 1,
            Off      = 2
        };

        enum AlignedType : uint8_t
        {
            Aligned2D = 0,
            Aligned3D = 1,
            None      = 2
        };

        void Update(float dt, Vec3 emitterPosition = Vec3(0.0f));
        void SetTextureFromFile(const std::string& path);

        Particle* GetParticles() { return m_Particles; }

        // Getter methods
        const SharedPtr<Graphics::Texture>& GetTexture() const { return m_Texture; }
        uint32_t GetParticleCount() const { return m_ParticleCount; }
        float GetParticleLife() const { return m_ParticleLife; }
        float GetParticleSize() const { return m_ParticleSize; }
        const Vec3& GetInitialVelocity() const { return m_InitialVelocity; }
        const Vec4& GetInitialColour() const { return m_InitialColour; }
        const Vec3& GetSpread() const { return m_Spread; }
        const Vec3& GetVelocitySpread() const { return m_VelocitySpread; }
        const Vec3& GetGravity() const { return m_Gravity; }
        float GetNextParticleTime() const { return m_NextParticleTime; }
        float GetParticleRate() const { return m_ParticleRate; }
        uint32_t GetNumLaunchParticles() const { return m_NumLaunchParticles; }
        bool GetIsAnimated() const { return m_IsAnimated; }
        uint32_t GetAnimatedTextureRows() const { return m_AnimatedTextureRows; }
        bool GetSortParticles() const { return m_SortParticles; }
        BlendType GetBlendType() const { return m_BlendType; }
        float GetFadeIn() const { return m_FadeIn; }
        float GetFadeOut() const { return m_FadeOut; }
        float GetLifeSpread() const { return m_LifeSpread; }
        AlignedType GetAlignedType() const { return m_AlignedType; }
        bool GetDepthWrite() const { return m_DepthWrite; }

        // Setter methods
        void SetTexture(const SharedPtr<Graphics::Texture>& texture) { m_Texture = texture; }
        void SetParticleCount(uint32_t particleCount);
        void SetParticleLife(const float& particleLife) { m_ParticleLife = particleLife; }
        void SetParticleSize(const float& particleSize) { m_ParticleSize = particleSize; }
        void SetInitialVelocity(const Vec3& initialVelocity) { m_InitialVelocity = initialVelocity; }
        void SetInitialColour(const Vec4& initialColour) { m_InitialColour = initialColour; }
        void SetSpread(const Vec3& spread) { m_Spread = spread; }
        void SetVelocitySpread(const Vec3& velocitySpread) { m_VelocitySpread = velocitySpread; }
        void SetGravity(const Vec3& gravity) { m_Gravity = gravity; }
        void SetNextParticleTime(const float& nextParticleTime) { m_NextParticleTime = nextParticleTime; }
        void SetParticleRate(const float& particleRate) { m_ParticleRate = particleRate; }
        void SetNumLaunchParticles(uint32_t numLaunchParticles) { m_NumLaunchParticles = numLaunchParticles; }
        void SetIsAnimated(bool isAnimated) { m_IsAnimated = isAnimated; }
        void SetAnimatedTextureRows(uint32_t animatedTextureRows) { m_AnimatedTextureRows = animatedTextureRows; }
        void SetSortParticles(bool sortParticles) { m_SortParticles = sortParticles; }
        void SetBlendType(const BlendType& blendType) { m_BlendType = blendType; }
        void SetFadeIn(const float& fadeIn) { m_FadeIn = fadeIn; }
        void SetFadeOut(const float& fadeOut) { m_FadeOut = fadeOut; }
        void SetLifeSpread(const float& spread) { m_LifeSpread = spread; }
        void SetAlignedType(const AlignedType& aligned) { m_AlignedType = aligned; }
        void SetDepthWrite(bool DepthWrite) { m_DepthWrite = DepthWrite; }

        static const std::array<Vec2, 4>& GetDefaultUVs();
        std::array<Vec2, 4> GetAnimatedUVs(float currentLife, int numRows);
        std::array<Vec4, 4> GetBlendedAnimatedUVs(float currentLife, int numRows, float& outBlendAmount);

    private:
        void Init();
        uint32_t FirstUnusedParticle();
        void RespawnParticle(Particle& particle, Vec3 emitterPosition = Vec3(0.0f));

        Particle* m_Particles = nullptr;

        SharedPtr<Graphics::Texture> m_Texture;
        uint32_t m_ParticleCount       = 1024;
        float m_ParticleLife           = 3.0f;
        float m_ParticleSize           = 0.1f;
        Vec3 m_InitialVelocity         = Vec3(0.0f, 1.0f, 0.0f);
        Vec4 m_InitialColour           = Vec4(1.0f);
        Vec3 m_Spread                  = Vec3(2.0f, 0.0f, 2.0f);
        Vec3 m_VelocitySpread          = Vec3(0.0f, 0.0f, 0.0f);
        float m_FadeIn                 = -1.0f;
        float m_FadeOut                = -1.0f;
        float m_NextParticleTime       = 0.0f;
        float m_ParticleRate           = 0.1f;
        uint32_t m_NumLaunchParticles  = 10;
        bool m_IsAnimated              = false;
        Vec3 m_Gravity                 = Vec3(0.0f, 0.0f, 0.0f);
        uint32_t m_AnimatedTextureRows = 4;
        float m_LifeSpread             = 0.1f;
        bool m_SortParticles           = false;
        bool m_DepthWrite              = false;
        BlendType m_BlendType          = BlendType::Additive;
        AlignedType m_AlignedType      = AlignedType::Aligned3D;

        Arena* m_Arena = nullptr;
    };

}
