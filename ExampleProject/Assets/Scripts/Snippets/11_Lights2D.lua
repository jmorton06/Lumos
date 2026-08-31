-- ============================================================================
-- 11_Lights2D — runnable forward 2D lights demo (point / spot / global + normals)
-- ============================================================================
-- Light2D.Type: 0=Point 1=Spot 2=Global
-- Sprites light up when ReceivesLight = true (default) AND the scene has a
-- Light2D. Attach this script to any entity and press play.
-- NOTE: `scene` is only valid inside OnInit/OnUpdate, not at the file top level.

local entityManager
local movingLight

function OnInit()
    entityManager = scene:GetEntityManager()

    -- Lit sprite (optionally give it a normal map for 2.5D shading).
    local s = entityManager:Create()
    s:AddTransform():SetLocalPosition(Vec3.new(0, 0, 0))
    local sprite = s:AddSprite(Vec2.new(-0.5, -0.5), Vec2.new(1, 1), Vec4.new(1, 1, 1, 1))
    sprite:SetTextureFromFile("//Assets/Textures/icon.png")
    -- sprite:SetNormalTextureFromFile("//Assets/Textures/icon_n.png")
    sprite.ReceivesLight = true

    -- Global fill so nothing is pure black.
    local g = entityManager:Create()
    g:AddTransform()
    local gl = g:AddLight2D()
    gl.Type      = 2
    gl.Colour    = Vec4.new(0.3, 0.35, 0.5, 1)
    gl.Intensity = 1.0

    -- Spot light aimed down (cone in the 2D plane; rotate transform to aim).
    local sp = entityManager:Create()
    sp:AddTransform():SetLocalPosition(Vec3.new(-2, 3, 0))
    local spl = sp:AddLight2D()
    spl.Type       = 1
    spl.Colour     = Vec4.new(0.7, 0.8, 1, 1)
    spl.Intensity  = 6.0
    spl.Radius     = 8.0
    spl.InnerAngle = 0.95
    spl.OuterAngle = 0.80

    -- Point light we orbit in OnUpdate.
    local p = entityManager:Create()
    p:AddTransform():SetLocalPosition(Vec3.new(2, 1, 0))
    movingLight = p
    local pl = p:AddLight2D()
    pl.Type      = 0
    pl.Colour    = Vec4.new(1, 0.85, 0.6, 1)
    pl.Intensity = 4.0
    pl.Radius    = 6.0
    pl.Height    = 1.5   -- virtual z, drives normal-map shading angle
    pl.Falloff   = 1.5   -- >1 sharpens the radial curve
end

local t = 0
function OnUpdate(dt)
    t = t + dt
    -- Light2D pulls its world position from the Transform each frame.
    local r = 2.5
    movingLight:GetTransform():SetLocalPosition(Vec3.new(math.cos(t) * r, math.sin(t) * r, 0))
end
