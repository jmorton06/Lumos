-- ============================================================================
-- Tappy Plane - a Flappy Bird style example
--
-- Everything is built here in OnInit: an orthographic camera, a Box2D plane,
-- rock pairs that are recycled as they leave the screen, a wrapping
-- background and the menus (engine UI). Sounds come from the engine Synth,
-- so no audio files are needed.
--
--   Space / click / tap   flap
--   Esc / P               pause
--   R                     back to the title
-- ============================================================================

local ART = "//Assets/Textures/TappyPlane/PNG/"
local SAVE = "//Assets/Cache/FlappyBird.best"

local C = {
    gravity      = -32.0,
    flap         = 12.5,  -- upward speed a flap sets
    maxFall      = -18.0,
    speed        = 7.0,   -- forward speed at the start
    speedPerRock = 0.12,  -- added for every pair passed
    maxSpeed     = 13.0,
    spacing      = 9.0,   -- between rock pairs
    gap          = 5.2,   -- opening at the start
    minGap       = 3.6,
    gapPerRock   = 0.06,
    drift        = 3.5,   -- how far the opening may move from one pair to the next
    top          = 10.0,  -- the view is 20 units tall
    pairs        = 6,
    planeX       = -4.0,  -- where the plane sits on screen (camera leads it)
}

local State = { Title = 1, Ready = 2, Playing = 3, Paused = 4, Dead = 5 }

local em
local state = State.Title
local plane, camera
local planeFrames = {}
local rockTex
local backgrounds = {}
local rocks = {} -- the rock pairs, recycled
local nextX = 0.0
local lastGapY = 0.0
local score, best = 0, 0
local animTime, deadTime, readyTime = 0.0, 0.0, 0.0
local shake = 0.0
local heldVelocity -- the plane's velocity while paused
-- C (or TAPPY_DEBUG=1) draws the colliders; in debug the plane also freezes where it hit
local showColliders = os.getenv ~= nil and os.getenv("TAPPY_DEBUG") ~= nil

local syn, sfx = nil, {}

-- --- sound ------------------------------------------------------------------

local function initAudio()
    if not Synth then return end
    local ok = pcall(function()
        syn = Synth.new(44100, true)
        sfx.flap = syn:AddInstrument({ osc1 = "triangle", pitchEnv = 8, pitchDecay = 0.06, attack = 0.002,
            decay = 0.09, sustain = 0, release = 0.04, filter = "off", volume = 0.18, voices = 4 })
        sfx.point = syn:AddInstrument({ osc1 = "sine", osc2 = "sine", osc2Octave = 1, osc2Level = 0.3,
            attack = 0.002, decay = 0.25, sustain = 0, release = 0.1, filter = "off", volume = 0.16, voices = 4 })
        sfx.crash = syn:AddInstrument({ osc1 = "noise", filter = "lowpass", cutoff = 900, attack = 0.002,
            decay = 0.45, sustain = 0, release = 0.2, volume = 0.35, voices = 2 })
        syn:SetVolume(0.8)
        syn:Play()
    end)
    if not ok then syn = nil end
end

local function play(name, note, dur)
    if syn and sfx[name] then syn:NoteOn(sfx[name], note, 1.0, syn:Now(), dur or 0.1) end
end

-- --- saving the best score --------------------------------------------------

local function loadBest()
    local data = ReadFileBytes and ReadFileBytes(SAVE)
    best = tonumber(data or "") or 0
end

local function saveBest()
    if WriteFileBytes then WriteFileBytes(SAVE, tostring(best)) end
end

-- --- world ------------------------------------------------------------------

local function body(e) return e:GetRigidBody2DComponent():GetRigidBody() end

-- A rock is a spike, so its collider is a triangle, not the sprite's box.
-- rock.png's outline in -1..1 of its half size. Its left side is concave and a Box2D shape
-- has to be convex, so the collider runs along the inner line there: it never pokes outside the
-- picture (a near miss that looks clear is clear).
local ROCK_POINTS = { { -0.72, -1.0 }, { 0.15, 0.97 }, { 0.28, 0.97 }, { 0.63, -0.05 }, { 0.96, -1.0 } }

local function makeRock(x, y, h, upsideDown)
    local e = em:Create("Rock")
    local half = Vec3.new(1.7, h * 0.5, 1.0)
    e:AddSprite(Vec2.new(-half.x, -half.y), Vec2.new(half.x * 2.0, half.y * 2.0), Vec4.new(1, 1, 1, 1)):SetTexture(rockTex)
    local p = RigidBodyParameters.new()
    p.position = Vec3.new(x, y, 0.0)
    p.scale    = half
    p.shape    = Shape.Custom
    p.isStatic = true
    p.customShapePositions = {}
    for i, v in ipairs(ROCK_POINTS) do p.customShapePositions[i] = Vec2.new(v[1] * half.x, v[2] * half.y) end
    e:GetTransform():SetLocalPosition(Vec3.new(x, y, 0.0))
    e:AddRigidBody2DComponent(p)
    if upsideDown then body(e):SetOrientation(math.pi) end
    return e
end

-- The play area is always C.top tall each way. A tall narrow screen (a phone upright) would only
-- show a few units ahead, so the view grows to keep at least MIN_WIDTH across; the rocks and the
-- background are made long enough to fill it, so nothing above / below the play area shows.
local MIN_WIDTH = 18.0
local viewHalf = C.top -- half the view's height

local function updateView()
    local size = GetAppInstance():GetWindowSize()
    local aspect = size.x / math.max(size.y, 1.0)
    viewHalf = math.max(C.top, MIN_WIDTH * 0.5 / math.max(aspect, 0.1))
    local cam = camera:GetCamera()
    cam:SetAspectRatio(aspect)
    cam:SetScale(viewHalf)
end

local function currentGap() return math.max(C.minGap, C.gap - score * C.gapPerRock) end

-- The opening wanders from the last one by at most C.drift, so a pair is never
-- an impossible jump from the one before it.
local function placePair(pair)
    if pair.top then pair.top:Destroy() end
    if pair.bottom then pair.bottom:Destroy() end
    local gap = currentGap()
    local limit = C.top - gap * 0.5 - 1.5
    local y = math.max(-limit, math.min(limit, lastGapY + (math.random() * 2.0 - 1.0) * C.drift))
    lastGapY = y
    local edge = viewHalf + 1.0 -- reach past the view's top / bottom
    local topH = edge - (y + gap * 0.5)
    local bottomH = (y - gap * 0.5) + edge
    pair.top    = makeRock(nextX, edge - topH * 0.5, topH, true)
    pair.bottom = makeRock(nextX, -edge + bottomH * 0.5, bottomH, false)
    pair.heights = { topH, bottomH }
    pair.x, pair.passed = nextX, false
    nextX = nextX + C.spacing
end

local BG_W, BG_H = 40.0, 24.0 -- background size, grown with the view

local function fitBackground()
    local scale = math.max(1.0, (viewHalf * 2.0 + 2.0) / 24.0)
    BG_W, BG_H = 40.0 * scale, 24.0 * scale
    for _, e in ipairs(backgrounds) do
        local s = e:GetSprite()
        s:SetPosition(Vec2.new(-BG_W * 0.5, -BG_H * 0.5))
        s:SetScale(Vec2.new(BG_W, BG_H))
    end
end

local function makeBackground(i)
    local e = em:Create("Background")
    e:AddTransform():SetLocalPosition(Vec3.new(0, 0, -5))
    e:AddSprite(Vec2.new(-BG_W * 0.5, -BG_H * 0.5), Vec2.new(BG_W, BG_H), Vec4.new(1, 1, 1, 1)):SetTexture(
        LoadTextureWithParams("tappyBg", ART .. "background.png", TextureFilter.Linear, TextureWrap.ClampToEdge))
    backgrounds[i] = e
end

local PLANE_RADIUS = 0.7

local function makePlane()
    plane = em:Create("Plane")
    plane:AddSprite(Vec2.new(-1.1, -0.95), Vec2.new(2.2, 1.9), Vec4.new(1, 1, 1, 1)):SetTexture(planeFrames[1])
    local p = RigidBodyParameters.new()
    p.position = Vec3.new(0, 0, 0)
    p.scale    = Vec3.new(PLANE_RADIUS, PLANE_RADIUS, 1.0) -- smaller than the picture: near misses feel fair
    p.shape    = Shape.Circle
    p.isStatic = true
    plane:GetTransform():SetLocalPosition(Vec3.new(0, 0, 0))
    plane:AddRigidBody2DComponent(p)
    SetCallback(function() if state == State.Playing then Crash() end end, body(plane):GetB2Body())

    -- exhaust puffs trailing behind
    local puffs = plane:AddParticleEmitter()
    puffs:SetParticleCount(256)
    puffs:SetParticleRate(0.04)
    puffs:SetNumLaunchParticles(2)
    puffs:SetParticleLife(0.7)
    puffs:SetParticleSize(0.18)
    puffs:SetGravity(Vec3.new(0.0, 0.5, 0.0))
    puffs:SetSpread(Vec3.new(0.1, 0.2, 0.0))
    puffs:SetVelocitySpread(Vec3.new(0.2, 0.3, 0.0))
    puffs:SetInitialVelocity(Vec3.new(-2.5, 0.0, 0.0))
    puffs:SetInitialColour(Vec4.new(1.0, 1.0, 1.0, 0.35))
    puffs:SetFadeOut(0.9)
end

-- Back to the start: plane parked in the middle, a fresh set of rocks ahead.
local function resetWorld()
    score = 0
    nextX, lastGapY = 14.0, 0.0
    for i = 1, C.pairs do
        rocks[i] = rocks[i] or {}
        placePair(rocks[i])
    end
    local rb = body(plane)
    rb:SetIsStatic(true)
    rb:SetPosition(Vec2.new(0.0, 0.0))
    rb:SetLinearVelocity(Vec2.new(0.0, 0.0))
    rb:SetOrientation(0.0)
    rb:SetAngularVelocity(0.0)
    plane:GetTransform():SetLocalPosition(Vec3.new(0, 0, 0))
    shake = 0.0
end

-- --- game -------------------------------------------------------------------

local function flap()
    local rb = body(plane)
    local v = rb:GetLinearVelocity()
    rb:SetLinearVelocity(Vec2.new(v.x, C.flap))
    play("flap", 74, 0.08)
end

local function drawColliders()
    local colour = Vec4.new(1.0, 0.2, 0.3, 1.0)
    for _, pair in ipairs(rocks) do
        for _, e in ipairs({ pair.top, pair.bottom }) do
            if e and e:Valid() then
                local rb = body(e)
                local pos, angle = rb:GetPosition(), rb:GetAngle()
                local h = pair.heights and pair.heights[e == pair.top and 1 or 2]
                if h then
                    local c, sn = math.cos(angle), math.sin(angle)
                    local pts = {}
                    for i, v in ipairs(ROCK_POINTS) do
                        local lx, ly = v[1] * 1.7, v[2] * h * 0.5
                        pts[i] = Vec3.new(pos.x + lx * c - ly * sn, pos.y + lx * sn + ly * c, 0.0)
                    end
                    for i = 1, #pts do DebugLine(pts[i], pts[i % #pts + 1], 0.05, colour) end
                end
            end
        end
    end
    local p = body(plane):GetPosition()
    DebugCircle(Vec2.new(p.x, p.y), PLANE_RADIUS, Vec4.new(0.2, 1.0, 0.4, 1.0), 0.0, 0.05)
end

function Crash()
    state = State.Dead
    deadTime = 0.0
    shake = 0.45
    play("crash", 40, 0.4)
    if showColliders then body(plane):SetIsStatic(true) end
    if score > best then
        best = score
        saveBest()
    end
end

-- a static body keeps still; its speed is put back on resume
function Pause(on)
    local rb = body(plane)
    if on then
        heldVelocity = rb:GetLinearVelocity()
        state = State.Paused
    else
        state = State.Playing
        rb:SetIsStatic(false)
        if heldVelocity then rb:SetLinearVelocity(heldVelocity) end
    end
end

local function startPlaying()
    state = State.Playing
    body(plane):SetIsStatic(false)
    flap()
end

local function pressed()
    return Input.GetKeyPressed(Key.Space) or Input.GetKeyPressed(Key.Up) or Input.GetKeyPressed(Key.W)
        or Input.GetMouseClicked(MouseButton.Left)
end

function OnInit()
    em = scene:GetEntityManager()
    math.randomseed(os.time())
    SetB2DGravity(Vec2.new(0.0, C.gravity))

    for i = 1, 3 do
        planeFrames[i] = LoadTextureWithParams("plane" .. i, ART .. "Planes/planeBlue" .. i .. ".png", TextureFilter.Linear, TextureWrap.ClampToEdge)
    end
    rockTex = LoadTextureWithParams("rock", ART .. "rock.png", TextureFilter.Linear, TextureWrap.ClampToEdge)

    camera = em:Create("Camera")
    camera:AddTransform():SetLocalPosition(Vec3.new(-C.planeX, 0, 5))
    local size = GetAppInstance():GetWindowSize()
    camera:AddCamera(size.x / math.max(size.y, 1.0), C.top, 1.0)

    updateView()
    for i = 1, 3 do makeBackground(i) end
    fitBackground()
    makePlane()
    resetWorld()
    loadBest()
    initAudio()
end

local function updatePlane(dt)
    -- three frames of propeller
    animTime = animTime + dt
    local frame = math.floor(animTime * 15.0) % 3 + 1
    plane:GetSprite():SetTexture(planeFrames[frame])

    if state ~= State.Playing and state ~= State.Dead then return end
    local rb = body(plane)
    local v = rb:GetLinearVelocity()
    local speed = state == State.Dead and v.x * 0.98 or math.min(C.maxSpeed, C.speed + score * C.speedPerRock)
    rb:SetLinearVelocity(Vec2.new(speed, math.max(C.maxFall, v.y)))
    -- nose follows the climb / dive
    rb:SetOrientation(math.max(-1.2, math.min(0.5, v.y * 0.06)))
    rb:SetAngularVelocity(0.0)
end

local function updateWorld()
    local p = plane:GetTransform():GetWorldPosition()

    for _, pair in ipairs(rocks) do
        -- one point per pair, the moment the plane's centre passes it
        if state == State.Playing and not pair.passed and p.x > pair.x then
            pair.passed = true
            score = score + 1
            play("point", 84, 0.2)
        end
        -- gone off the left edge: becomes the pair furthest ahead
        if pair.x < p.x - 20.0 then placePair(pair) end
    end

    if state == State.Playing and (p.y > viewHalf + 1.0 or p.y < -viewHalf) then Crash() end

    -- camera leads the plane, shaking briefly after a crash
    local sx, sy = 0.0, 0.0
    if shake > 0.0 then
        sx, sy = (math.random() - 0.5) * shake, (math.random() - 0.5) * shake
    end
    camera:GetTransform():SetLocalPosition(Vec3.new(p.x - C.planeX + sx, sy, 5))

    -- background scrolls at half speed and wraps: three copies, always one ahead
    local bgX = (p.x - C.planeX) * 0.5
    local base = math.floor(bgX / BG_W) * BG_W
    for i, e in ipairs(backgrounds) do
        e:GetTransform():SetLocalPosition(Vec3.new(p.x - C.planeX - bgX + base + (i - 2) * BG_W, 0, -5))
    end
end

local function medal()
    if score >= 40 then return "Gold medal", Vec4.new(1.0, 0.82, 0.25, 1.0) end
    if score >= 20 then return "Silver medal", Vec4.new(0.82, 0.86, 0.92, 1.0) end
    if score >= 10 then return "Bronze medal", Vec4.new(0.85, 0.55, 0.3, 1.0) end
    return nil
end

local function label(id, text, size, colour)
    UIPushStyle(StyleVar.FontSize, size * Input.GetDPIScale())
    UIPushStyle(StyleVar.TextColor, colour or Vec4.new(1, 1, 1, 1))
    UILabel(id, text)
    UIPopStyle(StyleVar.TextColor)
    UIPopStyle(StyleVar.FontSize)
end

-- A rounded overlay with no title bar, pinned to a corner / the middle of the view.
local function beginOverlay(id, anchor, mx, my, centred)
    local d = Input.GetDPIScale()
    UIPushStyle(StyleVar.Padding, Vec2.new(16 * d, 10 * d))
    UIPushStyle(StyleVar.CornerRadius, 8.0 * d)
    UIPushStyle(StyleVar.BackgroundColor, Vec4.new(0.05, 0.07, 0.12, 0.85))
    UIPushStyle(StyleVar.Border, Vec2.new(0.0, 0.0))
    UIBeginOverlay(id, SizeKind.MaxChild, 1, SizeKind.ChildSum, 1,
        centred and (WidgetFlags.StackVertically | WidgetFlags.CentreX) or WidgetFlags.StackVertically)
    UIWindowAnchor(anchor, mx or 0, my or 0)
    UIPopStyle(StyleVar.Border)
    UIPopStyle(StyleVar.BackgroundColor)
end

local function endOverlay()
    UIPopStyle(StyleVar.CornerRadius)
    UIPopStyle(StyleVar.Padding)
    UIEndPanel()
end

local function card(id, build)
    beginOverlay(id, UIAnchor.MiddleCenter, 0, 0, true)
    build()
    endOverlay()
end

local function drawUI()
    if state == State.Playing or state == State.Paused or state == State.Dead then
        UIPushStyle(StyleVar.BackgroundColor, Vec4.new(0, 0, 0, 0))
        UIBeginOverlay("tappyScore", SizeKind.MaxChild, 1, SizeKind.ChildSum, 1, WidgetFlags.StackVertically)
        UIWindowAnchor(UIAnchor.TopCenter, 0, 24)
        UIPopStyle(StyleVar.BackgroundColor)
        label("tappyScoreNum", tostring(score), 48)
        UIEndPanel()
    end

    if state == State.Title then
        card("tappyTitle", function()
            label("tappyName", "Tappy Plane", 36, Vec4.new(1.0, 0.85, 0.3, 1.0))
            label("tappyHow", "Space, click or tap to flap", 16, Vec4.new(0.8, 0.85, 0.9, 1.0))
            if best > 0 then label("tappyBest", "Best  " .. best, 16) end
            UISpacer(8)
            if UIButton("Play###tappyplay").clicked then state, readyTime = State.Ready, 0.0 end
            if UIButton("Quit###tappyquit").clicked then ExitApp() end
        end)
    elseif state == State.Ready then
        card("tappyReady", function()
            label("tappyGetReady", "Get ready", 30, Vec4.new(1.0, 0.85, 0.3, 1.0))
            label("tappyTap", "Flap to start", 16)
        end)
    elseif state == State.Paused then
        card("tappyPause", function()
            label("tappyPaused", "Paused", 30)
            if UIButton("Resume###tappyresume").clicked then Pause(false) end
            if UIButton("Title###tappytitlebtn").clicked then resetWorld(); state = State.Title end
        end)
    elseif state == State.Dead and deadTime > 0.6 then
        card("tappyOver", function()
            label("tappyOverTitle", "Game Over", 32, Vec4.new(1.0, 0.45, 0.35, 1.0))
            label("tappyOverScore", "Score  " .. score, 20)
            label("tappyOverBest", (score >= best and score > 0) and "New best!" or ("Best  " .. best), 16,
                (score >= best and score > 0) and Vec4.new(1.0, 0.85, 0.3, 1.0) or Vec4.new(0.8, 0.85, 0.9, 1.0))
            local name, colour = medal()
            if name then label("tappyMedal", name, 18, colour) end
            UISpacer(8)
            if UIButton("Again###tappyagain").clicked then resetWorld(); state, readyTime = State.Ready, 0.0 end
            if UIButton("Title###tappytitle").clicked then resetWorld(); state = State.Title end
        end)
    end
end

function OnUpdate(dt)
    dt = math.min(dt, 0.05)
    local before = viewHalf
    updateView()
    if viewHalf ~= before then fitBackground() end
    shake = math.max(0.0, shake - dt)

    if state == State.Ready then
        readyTime = readyTime + dt
        -- the press that chose Play must not also flap
        if readyTime > 0.2 and pressed() then startPlaying() end
    elseif state == State.Playing then
        if pressed() then flap() end
        if Input.GetKeyPressed(Key.Escape) or Input.GetKeyPressed(Key.P) then Pause(true) end
    elseif state == State.Paused then
        if Input.GetKeyPressed(Key.Escape) or Input.GetKeyPressed(Key.P) then Pause(false) end
    elseif state == State.Dead then
        deadTime = deadTime + dt
    end
    if Input.GetKeyPressed(Key.R) then resetWorld(); state = State.Title end
    if Input.GetKeyPressed(Key.C) then showColliders = not showColliders end
    if showColliders then
        Render.SetBool("Debug", true)
        drawColliders()
    end

    -- paused: hold the plane still (the physics keeps stepping otherwise)
    local rb = body(plane)
    rb:SetIsStatic(state == State.Title or state == State.Ready or state == State.Paused or (state == State.Dead and showColliders))

    updatePlane(dt)
    updateWorld()
    drawUI()
end

function OnRelease()
    -- the scene is being torn down: leave its entities alone
    if syn then syn:Stop() end
    syn = nil
end
