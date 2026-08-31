-- ============================================================================
-- 2DTest — showcase of the engine's 2D features in one scene.
-- ============================================================================
-- Row of elements, each with a text label above it:
--   1. Unlit sprite           (ignores Light2D)
--   2. Lit sprite             (forward 2D lighting)
--   3. Lit sprite + normal    (2.5D shading from the normal map)
--   4. Tinted quad            (colour-only sprite, lit)
--   5. Sprite-sheet tile      (static frame from a sheet)
--   6. Animated cat           (sprite-sheet animation)
-- Lights: a dim global fill + a moving point light + a sweeping spot light,
-- so the lit elements visibly react while the unlit one stays constant.
--
-- Attach to any entity and press Play. `scene` is only valid inside
-- OnInit/OnUpdate (not at file top level).

local em
local pointLight     -- orbits the row
local spotLight      -- sweeps back and forth
local catSprite      -- animated in OnUpdate
local elementsY      = 0.0
local labelY         = 1.4

-- --- helpers ----------------------------------------------------------------

local function makeLabel(text, x, y)
    local e = em:Create()
    e:AddTransform():SetLocalPosition(Vec3.new(x - 0.55, y, 0.0))
    e:GetTransform():SetLocalScale(Vec3.new(0.4, 0.4, 0.4))
    local t = e:AddTextComponent()
    t.TextString = text
    t.Colour     = Vec4.new(1.0, 1.0, 1.0, 1.0)
    t.MaxWidth   = 6.0
    return e
end

-- pos/scale offset so the quad is centred on the transform
local function addCentredSprite(e, size, colour)
    local half = size * 0.5
    return e:AddSprite(Vec2.new(-half, -half), Vec2.new(size, size), colour)
end

-- --- setup ------------------------------------------------------------------

function OnInit()
    em = scene:GetEntityManager()

    local size = 1.3
    local xs   = { -5.5, -3.3, -1.1, 1.1, 3.3, 5.5 }

    -- 1. Unlit sprite -------------------------------------------------------
    do
        local e = em:Create()
        e:AddTransform():SetLocalPosition(Vec3.new(xs[1], elementsY, 0.0))
        local s = addCentredSprite(e, size, Vec4.new(1, 1, 1, 1))
        s:SetTextureFromFile("//Assets/Textures/icon.png")
        s.ReceivesLight = false
        makeLabel("Unlit", xs[1], labelY)
    end

    -- 2. Lit sprite ---------------------------------------------------------
    do
        local e = em:Create()
        e:AddTransform():SetLocalPosition(Vec3.new(xs[2], elementsY, 0.0))
        local s = addCentredSprite(e, size, Vec4.new(1, 1, 1, 1))
        s:SetTextureFromFile("//Assets/Textures/icon.png")
        s.ReceivesLight = true
        makeLabel("Lit", xs[2], labelY)
    end

    -- 3. Lit sprite + normal map (2.5D) ------------------------------------
    do
        local e = em:Create()
        e:AddTransform():SetLocalPosition(Vec3.new(xs[3], elementsY, 0.0))
        local s = addCentredSprite(e, size, Vec4.new(1, 1, 1, 1))
        s:SetTextureFromFile("//Assets/Textures/icon.png")
        s:SetNormalTextureFromFile("//Assets/Textures/water/waterNormal.png")
        s.ReceivesLight = true
        makeLabel("Lit + Normal", xs[3], labelY)
    end

    -- 4. Tinted colour-only quad (lit) -------------------------------------
    do
        local e = em:Create()
        e:AddTransform():SetLocalPosition(Vec3.new(xs[4], elementsY, 0.0))
        local s = addCentredSprite(e, size, Vec4.new(0.9, 0.3, 0.4, 1.0))
        s.ReceivesLight = true
        makeLabel("Tinted Quad", xs[4], labelY)
    end

    -- 5. Static sprite-sheet tile ------------------------------------------
    do
        local e = em:Create()
        e:AddTransform():SetLocalPosition(Vec3.new(xs[5], elementsY, 0.0))
        local s = addCentredSprite(e, size, Vec4.new(1, 1, 1, 1))
        s:SetTextureFromFile("//Assets/Textures/cat.png")
        -- pick one 32x32 frame from the 4x8 cat sheet (128x256)
        s:SetSpriteSheet(Vec2.new(1, 0), Vec2.new(32, 32), Vec2.new(32, 32), 0.0)
        s.ReceivesLight = false
        makeLabel("Sprite Sheet", xs[5], labelY)
    end

    -- 6. Animated cat (sprite-sheet animation) -----------------------------
    do
        local e = em:Create()
        e:AddTransform():SetLocalPosition(Vec3.new(xs[6], elementsY, 0.0))
        catSprite = addCentredSprite(e, size, Vec4.new(1, 1, 1, 1))
        catSprite:SetTextureFromFile("//Assets/Textures/cat.png")
        catSprite:SetSpriteSheet(Vec2.new(0, 0), Vec2.new(32, 32), Vec2.new(32, 32), 0.0)
        catSprite.ReceivesLight = true
        makeLabel("Animated Cat", xs[6], labelY)
    end

    -- Title -----------------------------------------------------------------
    makeLabel("2D Feature Test", -1.4, 3.2)

    -- --- lights ------------------------------------------------------------

    -- Dim global fill so lit elements aren't pure black between the lights.
    local g = em:Create()
    g:AddTransform()
    local gl = g:AddLight2D()
    gl.Type      = 2 -- Global
    gl.Colour    = Vec4.new(0.25, 0.28, 0.4, 1.0)
    gl.Intensity = 1.0

    -- Point light that orbits the row.
    pointLight = em:Create()
    pointLight:AddTransform():SetLocalPosition(Vec3.new(0, 0, 0))
    local pl = pointLight:AddLight2D()
    pl.Type      = 0 -- Point
    pl.Colour    = Vec4.new(1.0, 0.8, 0.5, 1.0)
    pl.Intensity = 5.0
    pl.Radius    = 5.0
    pl.Height    = 1.2 -- virtual z, drives normal-map shading
    pl.Falloff   = 1.5

    -- Spot light sweeping from above.
    spotLight = em:Create()
    spotLight:AddTransform():SetLocalPosition(Vec3.new(0, 4, 0))
    local sl = spotLight:AddLight2D()
    sl.Type       = 1 -- Spot
    sl.Colour     = Vec4.new(0.6, 0.85, 1.0, 1.0)
    sl.Intensity  = 7.0
    sl.Radius     = 9.0
    sl.InnerAngle = 0.96
    sl.OuterAngle = 0.84

    -- --- camera (orthographic 2D) -----------------------------------------
    local cam = em:Create()
    cam:AddTransform():SetLocalPosition(Vec3.new(0.0, 1.0, 0.0))
    local screen = GetAppInstance():GetWindowSize()
    cam:AddCamera(screen.x / screen.y, 6.0, 1.0)
end

-- --- per-frame --------------------------------------------------------------

local t       = 0.0
local catTime = 0.0
local catCol  = 0

function OnUpdate(dt)
    t = t + dt

    -- Light2D pulls position/direction from its Transform each frame.
    local r = 4.0
    pointLight:GetTransform():SetLocalPosition(Vec3.new(math.cos(t * 1.2) * r, math.sin(t * 1.2) * 1.5, 0.0))

    -- Sweep the spot light left/right by rotating its transform.
    spotLight:GetTransform():SetLocalPosition(Vec3.new(math.sin(t * 0.7) * 4.0, 4.0, 0.0))

    -- Advance the cat animation (~8 fps) by cycling sheet columns on row 0.
    catTime = catTime + dt
    if catTime > 0.12 then
        catTime = 0.0
        catCol  = (catCol + 1) % 4
        catSprite:SetSpriteSheet(Vec2.new(catCol, 0), Vec2.new(32, 32), Vec2.new(32, 32), 0.0)
    end
end
