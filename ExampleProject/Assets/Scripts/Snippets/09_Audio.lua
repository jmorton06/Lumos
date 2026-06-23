-- ============================================================================
-- 09_Audio — sfx, looping, ambient, pitch by speed
-- ============================================================================

-- ===== Snippet: One-shot SFX ================================================
local function PlaySfx(scene, path, pos, volume)
    local snd = Sound.Create(path, "wav")
    if not snd then return end
    local e = scene:GetEntityManager():Create("Sfx")
    e:AddTransform():SetLocalPosition(pos)
    local sc = e:AddSoundComponent()
    local n  = sc:GetSoundNode()
    n:SetSound(snd)
    n:SetVolume(volume or 1.0)
    n:SetLooping(false)
    n:Play()
    return e   -- destroy externally after sound length elapses
end
-- ============================================================================


-- ===== Snippet: One-shot SFX (auto-cleanup) =================================
local _sfx = {}
local function PlaySfxOneShot(scene, path, pos, volume)
    local snd = Sound.Create(path, "wav")
    if not snd then return end
    local e = scene:GetEntityManager():Create("Sfx")
    e:AddTransform():SetLocalPosition(pos)
    local n = e:AddSoundComponent():GetSoundNode()
    n:SetSound(snd); n:SetVolume(volume or 1.0); n:Play()
    table.insert(_sfx, { e = e, ttl = snd:GetLength() + 0.1 })
end
function OnUpdate(dt)
    for i = #_sfx, 1, -1 do
        _sfx[i].ttl = _sfx[i].ttl - dt
        if _sfx[i].ttl <= 0 then
            if _sfx[i].e:Valid() then _sfx[i].e:Destroy() end
            table.remove(_sfx, i)
        end
    end
end
-- ============================================================================


-- ===== Snippet: Looping music (attach to camera / ambient) ==================
local musicEntity
function OnInit()
    local snd = Sound.Create("//Assets/Sounds/Music/loop.ogg", "ogg")
    if not snd then return end
    musicEntity = scene:GetEntityManager():Create("Music")
    musicEntity:AddTransform()
    local n = musicEntity:AddSoundComponent():GetSoundNode()
    n:SetSound(snd); n:SetLooping(true); n:SetVolume(0.5); n:Play()
end
function OnRelease()
    if musicEntity and musicEntity:Valid() then musicEntity:Destroy() end
end
-- ============================================================================


-- ===== Snippet: Sound by event key (lookup table) ===========================
local LIB = {
    hit       = "//Assets/Sounds/hit.wav",
    pickup    = "//Assets/Sounds/pickup.wav",
    explosion = "//Assets/Sounds/explosion.wav",
}
local _cache = {}
local function Get(key)
    if _cache[key] then return _cache[key] end
    local s = Sound.Create(LIB[key], "wav")
    _cache[key] = s
    return s
end
-- ============================================================================


-- ===== Snippet: Pitch shift based on speed (wind / engine) ==================
local sndNode
function OnInit()
    sndNode = LuaComponent:GetCurrentEntity():AddSoundComponent():GetSoundNode()
    local s = Sound.Create("//Assets/Sounds/wind_loop.wav", "wav")
    sndNode:SetSound(s); sndNode:SetLooping(true); sndNode:Play()
end
function OnUpdate(dt)
    local body = LuaComponent:GetCurrentEntity():GetRigidBody3DComponent():GetRigidBody()
    local v = body:GetLinearVelocity()
    local speed = math.sqrt(v.x*v.x + v.y*v.y + v.z*v.z)
    sndNode:SetPitch(0.6 + math.min(1.4, speed * 0.05))
    sndNode:SetVolume(math.min(0.9, speed * 0.02))
end
-- ============================================================================


-- ===== Snippet: Listener (camera ears) ======================================
-- Attach Listener component on the camera entity; only one should be enabled.
local camE = nil
for e in EachEntity() do if e:HasCamera() then camE = e; break end end
if camE then
    local l = camE:GetOrAddListener()
    l.enabled = true
end
-- ============================================================================
