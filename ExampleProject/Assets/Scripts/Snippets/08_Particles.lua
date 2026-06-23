-- ============================================================================
-- 08_Particles — emitters: bursts, dust, trails, fire
-- ============================================================================

-- ===== Snippet: One-shot confetti burst =====================================
local function ConfettiBurst(scene, pos, count)
    local e = scene:GetEntityManager():Create("Confetti")
    e:AddTransform():SetLocalPosition(pos)
    local p = e:AddParticleEmitter()
    p:SetParticleCount(120)
    p:SetParticleLife(2.5)
    p:SetParticleSize(0.18)
    p:SetInitialColour(Vec4.new(1.0, 0.85, 0.2, 1.0))
    p:SetInitialVelocity(Vec3.new(0, 6, 0))
    p:SetSpread(Vec3.new(0.3, 0.1, 0.3))
    p:SetVelocitySpread(Vec3.new(3, 2, 3))
    p:SetGravity(Vec3.new(0, -6, 0))
    p:SetFadeIn(0.05)
    p:SetFadeOut(1.2)
    p:SetNumLaunchParticles(count)
    p:SetBlendType(ParticleBlendType.Additive)
    return e
end
-- ============================================================================


-- ===== Snippet: Dust impact (one-shot, ground-aligned) ======================
local function DustBurst(scene, pos)
    local e = scene:GetEntityManager():Create("Dust")
    e:AddTransform():SetLocalPosition(pos)
    local p = e:AddParticleEmitter()
    p:SetParticleCount(30)
    p:SetParticleLife(0.8)
    p:SetParticleSize(0.5)
    p:SetInitialColour(Vec4.new(0.7, 0.6, 0.5, 0.6))
    p:SetInitialVelocity(Vec3.new(0, 1.5, 0))
    p:SetSpread(Vec3.new(0.2, 0.05, 0.2))
    p:SetVelocitySpread(Vec3.new(2, 0.5, 2))
    p:SetGravity(Vec3.new(0, -1, 0))
    p:SetFadeOut(0.6)
    p:SetNumLaunchParticles(30)
    p:SetBlendType(ParticleBlendType.Alpha)
    return e
end
-- ============================================================================


-- ===== Snippet: Continuous trail (attach to projectile) =====================
local function AttachTrail(entity)
    local p = entity:AddParticleEmitter()
    p:SetParticleCount(200)
    p:SetParticleLife(0.5)
    p:SetParticleSize(0.08)
    p:SetInitialColour(Vec4.new(1, 0.4, 0.1, 1))
    p:SetInitialVelocity(Vec3.new(0, 0, 0))
    p:SetSpread(Vec3.new(0.02, 0.02, 0.02))
    p:SetVelocitySpread(Vec3.new(0.2, 0.2, 0.2))
    p:SetParticleRate(0.02)        -- spawn every 20ms
    p:SetFadeOut(0.4)
    p:SetBlendType(ParticleBlendType.Additive)
    return p
end
-- ============================================================================


-- ===== Snippet: Campfire (looping, animated) ================================
local e = scene:GetEntityManager():Create("Fire")
e:AddTransform():SetLocalPosition(Vec3.new(0, 0, 0))
local p = e:AddParticleEmitter()
p:SetParticleCount(60)
p:SetParticleLife(1.0)
p:SetLifeSpread(0.4)
p:SetParticleSize(0.4)
p:SetInitialColour(Vec4.new(1.0, 0.5, 0.0, 1.0))
p:SetInitialVelocity(Vec3.new(0, 2.5, 0))
p:SetSpread(Vec3.new(0.15, 0.0, 0.15))
p:SetVelocitySpread(Vec3.new(0.4, 0.4, 0.4))
p:SetGravity(Vec3.new(0, 0.5, 0))    -- buoyancy
p:SetParticleRate(0.04)
p:SetFadeIn(0.15)
p:SetFadeOut(0.5)
p:SetBlendType(ParticleBlendType.Additive)
-- ============================================================================


-- ===== Snippet: Smoke plume =================================================
local p = entity:AddParticleEmitter()
p:SetParticleCount(80)
p:SetParticleLife(3.0)
p:SetParticleSize(0.7)
p:SetInitialColour(Vec4.new(0.2, 0.2, 0.2, 0.4))
p:SetInitialVelocity(Vec3.new(0, 1.2, 0))
p:SetSpread(Vec3.new(0.2, 0, 0.2))
p:SetVelocitySpread(Vec3.new(0.3, 0.2, 0.3))
p:SetGravity(Vec3.new(0.2, 0.1, 0))   -- drift sideways
p:SetParticleRate(0.08)
p:SetFadeIn(0.5)
p:SetFadeOut(1.5)
p:SetBlendType(ParticleBlendType.Alpha)
-- ============================================================================


-- ===== Snippet: Auto-destroy after life =====================================
local _active = {}
local function Burst(scene, pos)
    local e = ConfettiBurst(scene, pos, 100)
    table.insert(_active, { e = e, ttl = 3.0 })
end
function OnUpdate(dt)
    for i = #_active, 1, -1 do
        _active[i].ttl = _active[i].ttl - dt
        if _active[i].ttl <= 0 then
            if _active[i].e:Valid() then _active[i].e:Destroy() end
            table.remove(_active, i)
        end
    end
end
-- ============================================================================


-- ===== Snippet: Textured particles (spritesheet) ============================
local p = entity:AddParticleEmitter()
p:SetTextureFromFile("//Assets/Textures/spark.png")
p:SetIsAnimated(true)
p:SetAnimatedTextureRows(4)
-- ============================================================================
