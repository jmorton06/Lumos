-- ============================================================================
-- 25_Materials — apply textures, colour swap, emissive pulse
-- ============================================================================

-- ===== Snippet: Load textures and apply to a material ======================
local function ApplyPBR(material, albedo, normal, rough, metal)
    if albedo then material:set_albedo_texture(albedo)       end
    if normal then material:set_normal_texture(normal)       end
    if rough  then material:set_roughness_texture(rough)     end
    if metal  then material:set_metallic_texture(metal)      end
end

local alb = LoadTexture("rock_alb", "//Assets/Textures/rock_albedo.png")
local nor = LoadTexture("rock_nor", "//Assets/Textures/rock_normal.png")
local rgh = LoadTexture("rock_rgh", "//Assets/Textures/rock_rough.png")
-- ApplyPBR(meshMaterial, alb, nor, rgh, nil)
-- ============================================================================


-- ===== Snippet: Emissive pulse via SetEntityPulse ==========================
-- SetEntityPulse(entity, albedoVec4, emissiveAmount)
SetEntityPulse(targetEntity, Vec4.new(1, 0.7, 0.1, 1), 4.0)
-- ============================================================================


-- ===== Snippet: Load + assign sprite sheet ================================
local tex = LoadTexture("anim", "//Assets/Textures/character.png")
local e = scene:GetEntityManager():Create("Anim")
e:AddTransform()
local sp = e:AddAnimatedSprite()
sp:SetSpriteSheet(tex)
sp.SpriteSheetTileSizeX = 32
sp.SpriteSheetTileSizeY = 32
sp:SetSpriteSheetIndex(0)
-- ============================================================================


-- ===== Snippet: Cycle sprite frames ========================================
local sp, _t, _frame = nil, 0.0, 0
function OnInit()
    local e = LuaComponent:GetCurrentEntity()
    sp = e:GetSprite()
end
function OnUpdate(dt)
    _t = _t + dt
    if _t > 0.1 then
        _t = 0
        _frame = (_frame + 1) % 8
        sp:SetSpriteSheetIndex(_frame)
    end
end
-- ============================================================================


-- ===== Snippet: Save a screenshot ==========================================
if Input.GetKeyPressed(Key.F12) then
    SaveScreenshot("//Screenshots/shot_" .. os.time() .. ".png")
end
-- ============================================================================
