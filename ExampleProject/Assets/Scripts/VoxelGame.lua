-- Minecraft-like voxel test game.
--
-- SETUP (in the editor):
--   1. Create  ->  3D Object  ->  Voxel World   (spawns the streamed world)
--   2. Create an empty entity "Player", add a LuaScript component pointing at this
--      file (//Assets/Scripts/VoxelGame.lua). Transform + Camera are auto-added.
--   3. Press Play.
--
-- CONTROLS:
--   Mouse            look (also arrow keys as a fallback)
--   WASD             move (camera-relative)
--   Space / L-Ctrl   fly up / down
--   Hold right mouse look around (cursor locked, like the editor)
--   Left mouse       break block
--   Right-click tap  place selected block (tap without dragging)
--   1-7 / scroll     select block in the hotbar
--   F                toggle gravity + ground collision (walk mode)

local entity
local transform

-- View state (degrees).
local yaw   = 45.0
local pitch = -25.0

-- Tunables.
local MOVE_SPEED   = 14.0   -- blocks/sec
local LOOK_SPEED   = 90.0   -- deg/sec (arrow keys)
local MOUSE_LOOK   = 0.15   -- deg per pixel of mouse movement
local REACH        = 8.0    -- block edit distance
local EYE_HEIGHT   = 1.7

-- Hotbar palette: { name, blockId } matching the engine block enum.
local PALETTE = {
    { "Grass",  1 },
    { "Dirt",   2 },
    { "Stone",  3 },
    { "Sand",   4 },
    { "Snow",   5 },
    { "Wood",   6 },
    { "Leaves", 7 },
}
local selected = 1
local GRAVITY      = 28.0
local JUMP_SPEED   = 9.0

-- Day/night cycle.
local DAY_LENGTH = 180.0  -- seconds for a full day
local timeOfDay  = 0.30   -- 0..1 (0.25 = noon, 0.75 = midnight); start mid-morning

-- Walk-mode state.
local gravityOn = false
local velY      = 0.0
local onGround  = false
local HALF      = 0.4       -- player half-width for AABB collision
local HEAD      = 0.2       -- clearance above the eye
local CAM_R     = 0.25      -- eye shell radius; keeps the near plane out of blocks

local lastMouse = nil
local looking   = false   -- true while RMB held (cursor captured for free-look)
local dragDist  = 0.0     -- mouse travel during the current RMB hold

local function clamp(v, lo, hi)
    if v < lo then return lo elseif v > hi then return hi else return v end
end

-- The transform position IS the eye/camera. The player body hangs below it
-- (feet at eye.y - EYE_HEIGHT). True if that body overlaps a solid block.
local function BlockedAt(eye)
    return Voxel.OverlapAABB(eye.x - HALF, eye.y - EYE_HEIGHT, eye.z - HALF,
                             eye.x + HALF, eye.y + HEAD,        eye.z + HALF)
end

-- Small shell around the eye itself, so the camera/near-plane never enters a solid.
local function EyeBlocked(x, y, z)
    return Voxel.OverlapAABB(x - CAM_R, y - CAM_R, z - CAM_R,
                             x + CAM_R, y + CAM_R, z + CAM_R)
end

function OnInit()
    entity = LuaComponent:GetCurrentEntity()
    -- Drive the camera the renderer actually uses, no matter which entity this
    -- script is on. Fall back to making this entity the camera if the scene has none.
    transform = GetMainCameraTransform()
    if not transform then
        transform = entity:GetOrAddTransform()
        entity:GetOrAddCamera()
    end
    transform:SetLocalPosition(Vec3.new(8.0, 96.0, 8.0))
    transform:SetLocalOrientation(Quat.new(pitch, yaw, 0.0))
    Log.Info("VoxelGame ready - WASD move, hold RMB to look, LMB break, RMB-tap place, 1-7 select, F gravity")
end

function OnUpdate(dt)
    -- Re-fetch in case the active camera changed; keep last good transform otherwise.
    local cam = GetMainCameraTransform()
    if cam then transform = cam end
    if not transform then return end

    -- ---- Look ---- (only while holding right mouse, cursor captured, like the editor)
    local placeTap = false
    local rmb = Input.GetMouseHeld(MouseButton.Right)
    if rmb and not looking then
        looking  = true
        dragDist = 0.0
        lastMouse = nil               -- avoid a jump on the first captured frame
        Input.SetMouseCaptured(true)
    elseif not rmb and looking then
        looking = false
        Input.SetMouseCaptured(false)
        if dragDist < 6.0 then        -- a tap (not a drag) places a block
            placeTap = true
        end
    end

    if looking then
        local m = Input.GetMousePosition()
        if lastMouse then
            local ddx = m.x - lastMouse.x
            local ddy = m.y - lastMouse.y
            if math.abs(ddx) < 200 and math.abs(ddy) < 200 then -- skip cursor warps/refocus jumps
                -- yaw is negated: +Y rotation turns forward toward -X, so mouse-right
                -- must decrease yaw to turn right (matches the editor camera convention).
                yaw   = yaw   - ddx * MOUSE_LOOK
                pitch = pitch - ddy * MOUSE_LOOK
                dragDist = dragDist + math.abs(ddx) + math.abs(ddy)
            end
        end
        lastMouse = m
    end

    -- Arrow keys are a keyboard fallback (always available).
    if Input.GetKeyHeld(Key.Left)  then yaw   = yaw   + LOOK_SPEED * dt end
    if Input.GetKeyHeld(Key.Right) then yaw   = yaw   - LOOK_SPEED * dt end
    if Input.GetKeyHeld(Key.Up)    then pitch = pitch + LOOK_SPEED * dt end
    if Input.GetKeyHeld(Key.Down)  then pitch = pitch - LOOK_SPEED * dt end

    pitch = clamp(pitch, -89.0, 89.0)
    transform:SetLocalOrientation(Quat.new(pitch, yaw, 0.0))

    -- ---- Move ----
    local fwd   = transform:GetForwardDirection()
    local right = transform:GetRightDirection()
    local pos   = transform:LocalPosition()

    -- Horizontal basis (ignore pitch for ground travel).
    local fl = math.sqrt(fwd.x * fwd.x + fwd.z * fwd.z)
    if fl < 1e-4 then fl = 1.0 end
    local fX, fZ = fwd.x / fl, fwd.z / fl

    local dx, dy, dz = 0.0, 0.0, 0.0
    if Input.GetKeyHeld(Key.W) then dx = dx + fX; dz = dz + fZ end
    if Input.GetKeyHeld(Key.S) then dx = dx - fX; dz = dz - fZ end
    if Input.GetKeyHeld(Key.D) then dx = dx + right.x; dz = dz + right.z end
    if Input.GetKeyHeld(Key.A) then dx = dx - right.x; dz = dz - right.z end

    -- Normalise horizontal move.
    local dl = math.sqrt(dx * dx + dz * dz)
    if dl > 1e-4 then dx, dz = dx / dl, dz / dl end

    if Input.GetKeyPressed(Key.F) then
        gravityOn = not gravityOn
        velY = 0.0
        Log.Info(gravityOn and "Walk mode" or "Fly mode")
    end

    local step = MOVE_SPEED * dt
    local nx = pos.x + dx * step
    local ny = pos.y
    local nz = pos.z + dz * step

    if gravityOn then
        -- Jump.
        if onGround and Input.GetKeyHeld(Key.Space) then velY = JUMP_SPEED end
        velY = velY - GRAVITY * dt
        ny = pos.y + velY * dt

        -- Resolve per-axis against the voxel grid (slide along walls/floor).
        if BlockedAt(Vec3.new(nx, pos.y, pos.z)) then nx = pos.x end
        if BlockedAt(Vec3.new(nx, pos.y, nz))     then nz = pos.z end
        onGround = false
        if BlockedAt(Vec3.new(nx, ny, nz)) then
            if velY < 0.0 then onGround = true end
            ny = pos.y
            velY = 0.0
        end
    else
        -- Fly.
        if Input.GetKeyHeld(Key.Space)       then ny = ny + step end
        if Input.GetKeyHeld(Key.LeftControl) then ny = ny - step end

        -- Camera collision: keep the eye shell out of solids, per-axis so we
        -- slide along blocks instead of stopping dead.
        if EyeBlocked(nx, pos.y, pos.z) then nx = pos.x end
        if EyeBlocked(nx, pos.y, nz)    then nz = pos.z end
        if EyeBlocked(nx, ny, nz)       then ny = pos.y end
    end

    transform:SetLocalPosition(Vec3.new(nx, ny, nz))

    -- ---- Hotbar selection ---- (number keys 1-7, or scroll wheel)
    local numKeys = { Key.Keypad1, Key.Keypad2, Key.Keypad3, Key.Keypad4,
                      Key.Keypad5, Key.Keypad6, Key.Keypad7 }
    for i, k in ipairs(numKeys) do
        if i <= #PALETTE and Input.GetKeyPressed(k) then selected = i end
    end
    local scroll = Input.GetScrollOffset()
    if scroll > 0.1 then
        selected = selected - 1; if selected < 1 then selected = #PALETTE end
    elseif scroll < -0.1 then
        selected = selected + 1; if selected > #PALETTE then selected = 1 end
    end

    -- ---- Edit ---- (transform position is the eye, so ray starts there)
    -- Left click breaks; a right-click tap (no look-drag) places.
    if Input.GetMouseClicked(MouseButton.Left) then
        Voxel.Remove(nx, ny, nz, fwd.x, fwd.y, fwd.z, REACH)
    end
    if placeTap then
        Voxel.Place(nx, ny, nz, fwd.x, fwd.y, fwd.z, REACH, PALETTE[selected][2])
    end

    UpdateSky(dt)
    DrawHUD()
end

-- Lerp between two Vec4s by t (0..1).
local function lerpV4(a, b, t)
    return Vec4.new(a.x + (b.x - a.x) * t,
                    a.y + (b.y - a.y) * t,
                    a.z + (b.z - a.z) * t,
                    a.w + (b.w - a.w) * t)
end

-- Sky palette key-frames keyed on timeOfDay (0=midnight, 0.25=dawn, 0.5=noon, 0.75=dusk).
-- Each entry: { tod, zenith(Vec4), horizon(Vec4) }
local SKY_KEYS = {
    { 0.00, Vec4.new(0.01, 0.01, 0.06, 1), Vec4.new(0.04, 0.04, 0.12, 1) }, -- midnight
    { 0.20, Vec4.new(0.01, 0.01, 0.06, 1), Vec4.new(0.04, 0.04, 0.12, 1) }, -- pre-dawn
    { 0.25, Vec4.new(0.22, 0.24, 0.50, 1), Vec4.new(0.92, 0.52, 0.22, 1) }, -- dawn
    { 0.30, Vec4.new(0.30, 0.48, 0.82, 1), Vec4.new(0.76, 0.68, 0.58, 1) }, -- morning
    { 0.50, Vec4.new(0.18, 0.38, 0.80, 1), Vec4.new(0.60, 0.74, 0.90, 1) }, -- noon
    { 0.70, Vec4.new(0.30, 0.48, 0.82, 1), Vec4.new(0.76, 0.68, 0.58, 1) }, -- afternoon
    { 0.75, Vec4.new(0.22, 0.24, 0.50, 1), Vec4.new(0.92, 0.42, 0.14, 1) }, -- dusk
    { 0.80, Vec4.new(0.01, 0.01, 0.06, 1), Vec4.new(0.04, 0.04, 0.12, 1) }, -- post-dusk
    { 1.00, Vec4.new(0.01, 0.01, 0.06, 1), Vec4.new(0.04, 0.04, 0.12, 1) }, -- midnight again
}

local function sampleSkyPalette(tod)
    local zenith, horizon
    for i = 1, #SKY_KEYS - 1 do
        local ka, kb = SKY_KEYS[i], SKY_KEYS[i + 1]
        if tod >= ka[1] and tod <= kb[1] then
            local span = kb[1] - ka[1]
            local t    = span > 0.0 and (tod - ka[1]) / span or 0.0
            zenith  = lerpV4(ka[2], kb[2], t)
            horizon = lerpV4(ka[3], kb[3], t)
            break
        end
    end
    return zenith  or SKY_KEYS[1][2],
           horizon or SKY_KEYS[1][3]
end

-- Advance the sun and drive all environment visuals from timeOfDay.
function UpdateSky(dt)
    timeOfDay = (timeOfDay + dt / DAY_LENGTH) % 1.0

    local angle = timeOfDay * 2.0 * math.pi
    local elev  = math.sin(angle)       -- +1 noon, -1 midnight
    local up    = math.max(elev, 0.0)  -- 0..1 above-horizon fraction
    local night = math.max(-elev, 0.0) -- 0..1 below-horizon fraction (darkness)

    -- Sun transform.
    local sun = GetSunTransform and GetSunTransform()
    if sun then
        sun:SetLocalOrientation(Quat.new(timeOfDay * 360.0, -40.0, 0.0))
    end

    -- Sun light: intense warm white at noon, dim cool moonlight at night.
    local light = GetSunLight and GetSunLight()
    if light then
        if elev > 0.0 then
            light.Intensity = 5000.0 + up * 110000.0
            light.Colour    = Vec4.new(1.0, 0.72 + 0.28 * up, 0.45 + 0.55 * up, 1.0)
        else
            light.Intensity = 800.0
            light.Colour    = Vec4.new(0.35, 0.42, 0.65, 1.0)
        end
    end

    -- Sky colours, fog, stars from the Environment component.
    local env = GetSceneEnvironment and GetSceneEnvironment()
    if not env then return end

    local zenith, horizon = sampleSkyPalette(timeOfDay)
    env:SetZenithColour(zenith)
    env:SetHorizonColour(horizon)

    -- Fog: match horizon colour, density rises toward night to hide chunk edges.
    local fogStrength = 0.55 + night * 0.35
    env:SetFogColour(Vec4.new(horizon.x, horizon.y, horizon.z, fogStrength))
    local fogDensity = 0.0035 + night * 0.006
    env:SetFogParams(Vec4.new(fogDensity, 0.0, 0.0, 0.0))

    -- Stars: fade in after dusk, out before dawn.
    local starDensity = math.min(night * 4.0, 1.0)
    env:SetStarParams(Vec4.new(starDensity, 0.9, 0.8, 0.35))
    env:SetStarColour(Vec4.new(0.92, 0.94, 1.0, 1.0))

    -- Clouds: tint warm at dawn/dusk, grey-blue at night, white at noon.
    local cloudTint
    if elev > 0.0 then
        -- Interpolate between golden-hour warmth and neutral white.
        local warmth = 1.0 - up
        cloudTint = Vec4.new(1.0, 0.82 + 0.18 * up, 0.55 + 0.45 * up, 1.0)
    else
        cloudTint = Vec4.new(0.25, 0.28, 0.38, 1.0)
    end
    env:SetCloudColour(cloudTint)
end

-- Crosshair + hotbar, drawn with the engine's immediate-mode UI. Overlays carry
-- no header/title (unlike panels), so the crosshair shows no stray text and the
-- hotbar items don't fight a title bar for the horizontal row.
function DrawHUD()
    if not GetUIState then return end

    -- Crosshair: a chromeless "+" pinned to screen centre (transparent overlay).
    UIPushStyle(StyleVar.BackgroundColor, Vec4.new(0, 0, 0, 0))
    UIPushStyle(StyleVar.ShadowColor,     Vec4.new(0, 0, 0, 0))
    UIPushStyle(StyleVar.Padding,         Vec4.new(0, 0, 0, 0))
    UIPushStyle(StyleVar.TextColor,       Vec4.new(1, 1, 1, 0.85))
    UIBeginOverlay("VoxelCrosshair", nil, nil, nil, nil,
                   WidgetFlags.StackVertically | WidgetFlags.CentreX | WidgetFlags.CentreY)
        UILabel("xhair", "+")
    UIEndPanel()
    UIPopStyle(); UIPopStyle(); UIPopStyle(); UIPopStyle()

    -- Hotbar: horizontal row of block names along the bottom, selection highlighted.
    UIBeginOverlay("VoxelHotbar", nil, nil, nil, nil, WidgetFlags.StackHorizontally)
        UIWindowAnchor(UIAnchor.BottomCenter, 0.0, 24.0) -- set inside the overlay window
        for i, entry in ipairs(PALETTE) do
            if i == selected then
                UIPushStyle(StyleVar.TextColor, Vec4.new(1.0, 0.9, 0.3, 1.0))
                UILabel("slot" .. i, "[" .. entry[1] .. "]")
                UIPopStyle()
            else
                UIPushStyle(StyleVar.TextColor, Vec4.new(0.8, 0.8, 0.8, 1.0))
                UILabel("slot" .. i, " " .. entry[1] .. " ")
                UIPopStyle()
            end
        end
    UIEndPanel()
end

function OnCleanUp()
end
