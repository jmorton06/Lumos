-- ============================================================================
-- CarTest.lua — raycast vehicle test level
--
-- Attach as a LuaScriptComponent on any entity in an (otherwise empty) scene
-- and press Play. Builds a coloured ground + ramp + obstacles + a drivable car.
--
-- Controls:  W/S throttle/brake-reverse   A/D steer   Space handbrake
-- ============================================================================

local veh             -- RaycastVehicle handle
local chassisBody     -- chassis RigidBody3D
local wheelEnts = {}  -- visual wheel entities (0-based, matches vehicle indices)

-- Car settings (live-editable via the bottom-right panel). Defaults match OnInit.
local showSettings = false
local cfg = {
    engine   = 16000.0,
    brake    = 22000.0,
    steer    = 0.55,
    antiRoll = 0.85,
    grip     = 2.5,
    stiff    = 30000.0,
    damp     = 4000.0,
}

-- Engine conventions: forward -Z, up +Y, right +X.
local CHASSIS_HALF = Vec3.new(0.9, 0.4, 2.0)
local CHASSIS_MASS = 1200.0
local WHEEL_RADIUS = 0.4
local SUSP_REST    = 0.5

-- Palette
local C_GROUND = Vec4.new(0.30, 0.55, 0.35, 1.0) -- mossy green
local C_RAMP   = Vec4.new(0.90, 0.55, 0.15, 1.0) -- orange
local C_CAR    = Vec4.new(0.85, 0.15, 0.15, 1.0) -- red
local C_WHEEL  = Vec4.new(0.08, 0.08, 0.10, 1.0) -- near-black
local BOX_COLS = {
    Vec4.new(0.20, 0.45, 0.85, 1.0), -- blue
    Vec4.new(0.85, 0.80, 0.20, 1.0), -- yellow
    Vec4.new(0.60, 0.25, 0.75, 1.0), -- purple
    Vec4.new(0.20, 0.75, 0.75, 1.0), -- teal
    Vec4.new(0.90, 0.40, 0.55, 1.0), -- pink
}

local function MakeWheelConfig(x, z, steerable, powered)
    local w = VehicleWheelConfig.new()
    w.connectionPoint = Vec3.new(x, -0.2, z)
    w.radius          = WHEEL_RADIUS
    w.suspensionRest  = SUSP_REST
    w.maxTravel       = 0.3
    w.gripLateral     = 2.5
    w.gripForward     = 2.5
    w.steerable       = steerable
    w.powered         = powered
    w.brakes          = true
    return w
end

function OnInit()
    -- Ground: static (inverseMass 0). Top surface at y = 0.
    AddPhysicsCube(scene, "Ground", Vec3.new(0, -0.5, 0), Vec3.new(60, 0.5, 60), 0.0, C_GROUND)

    -- Static tilted ramp helper. pitch/yaw in radians.
    local function MakeRamp(name, pos, half, pitch, yaw)
        local r = AddPhysicsCube(scene, name, pos, half, 0.0, C_RAMP)
        local q = Quat.new(pitch, yaw or 0.0, 0.0)
        r:GetRigidBody3DComponent():GetRigidBody():SetOrientation(q)
        r:GetTransform():SetLocalOrientation(q)
        return r
    end

    MakeRamp("Ramp1", Vec3.new(0, 1.0, -25),  Vec3.new(6, 0.25, 8),  -0.28)        -- gentle, ahead
    MakeRamp("Ramp2", Vec3.new(18, 0.7, -8),  Vec3.new(4, 0.2, 3),   -0.5)         -- steep kicker (jump)
    MakeRamp("Ramp3", Vec3.new(-20, 1.6, -14), Vec3.new(4, 0.25, 12), -0.18)       -- long gentle
    MakeRamp("RampSide", Vec3.new(-30, 1.2, 8), Vec3.new(8, 0.25, 4), -0.22, 1.57) -- yawed 90°

    -- Terrain patch in front (-Z). AddTerrain attaches a TerrainCollisionShape
    -- WITH height data so the suspension rays + collision both work. Corner
    -- origin at pos, extends +x/+z by (gridSize-1)*scaleXZ. Sunk a little and
    -- kept low-relief so the seam with the flat ground stays drivable.
    AddTerrain(scene, Vec3.new(-63, -2.0, -181), 64, 2.0, 4.0)

    -- A few light, colourful obstacle boxes to knock around (mass 20 -> invMass 0.05).
    for i = 0, 4 do
        AddPhysicsCube(scene, "Box" .. i,
            Vec3.new(-8 + i * 1.4, 0.5 + i * 1.0, 12),
            Vec3.new(0.5, 0.5, 0.5), 1.0 / 20.0, BOX_COLS[i + 1])
    end

    -- Car chassis. Rests around y ~ 1.0; spawn just above to avoid a hard drop
    -- (a big drop can bounce/flip on landing).
    local spawn = Vec3.new(0, 1.2, 0)
    local car = AddPhysicsCube(scene, "Car", spawn, CHASSIS_HALF, 1.0 / CHASSIS_MASS, C_CAR)
    chassisBody = car:GetRigidBody3DComponent():GetRigidBody()
    chassisBody:SetFriction(0.4)

    veh = Physics.CreateVehicle(chassisBody)
    veh:SetEngineForce(16000.0)
    veh:SetBrakeForce(22000.0)
    veh:SetMaxSteerAngle(0.55)
    veh:SetAntiRoll(0.85) -- keep grip near COM so it resists flipping

    -- Front (-Z) steers, rear (+Z) powered. RWD.
    veh:AddWheel(MakeWheelConfig(-0.9, -1.4, true,  false)) -- front left
    veh:AddWheel(MakeWheelConfig( 0.9, -1.4, true,  false)) -- front right
    veh:AddWheel(MakeWheelConfig(-0.9,  1.4, false, true))  -- rear left
    veh:AddWheel(MakeWheelConfig( 0.9,  1.4, false, true))  -- rear right

    -- Visual wheels: dark cylinders (no physics), driven from vehicle state.
    -- Cylinder is Y-aligned; the vehicle rotation lays it along the axle.
    -- scale x/z = diameter, y = tread width.
    for i = 0, veh:NumWheels() - 1 do
        wheelEnts[i] = AddColouredPrimitive(scene, "Wheel" .. i, Vec3.new(0, -10, 0),
            Vec3.new(WHEEL_RADIUS * 2.0, 0.3, WHEEL_RADIUS * 2.0), PrimitiveType.Cylinder, C_WHEEL)
    end

    -- Ensure there's a camera to drive the chase view (Play mode uses the scene camera).
    if not GetMainCameraTransform() then
        local camE = scene:GetEntityManager():Create("ChaseCam")
        camE:AddTransform()
        camE:AddCamera(60.0, 16.0 / 9.0)
    end

    Log.Info("CarTest ready — WASD/arrows to drive, Space handbrake")
end

-- Bottom-right settings button + live car-tuning panel.
local function UpdateUI()
    if not veh then return end

    local panelFlags = WidgetFlags.StackVertically | WidgetFlags.DrawBackground

    -- Toggle button, anchored bottom-right.
    UIBeginPanel("CarSettingsToggle", SizeKind.ChildSum, 1, SizeKind.ChildSum, 1, panelFlags)
    UIWindowAnchor(UIAnchor.BottomRight, 16, 16)
        if UIButton(showSettings and "Close Settings" or "Settings").clicked then
            showSettings = not showSettings
        end
    UIEndPanel()

    if not showSettings then return end

    -- Settings panel, sitting just above the button.
    UIBeginPanel("Car Settings", SizeKind.Pixels, 320, SizeKind.ChildSum, 1, panelFlags)
    UIWindowAnchor(UIAnchor.BottomRight, 16, 70)
        UILabel("cs_title", "Car Settings")
        UISeparator()
        cfg.engine   = UISlider("Engine Force",   cfg.engine,   0.0,     30000.0)
        cfg.brake    = UISlider("Brake Force",    cfg.brake,    0.0,     40000.0)
        cfg.steer    = UISlider("Max Steer",      cfg.steer,    0.1,     1.0)
        cfg.antiRoll = UISlider("Anti-Roll",      cfg.antiRoll, 0.0,     1.0)
        cfg.grip     = UISlider("Tyre Grip",      cfg.grip,     0.5,     5.0)
        cfg.stiff    = UISlider("Susp Stiffness", cfg.stiff,    10000.0, 60000.0)
        cfg.damp     = UISlider("Susp Damping",   cfg.damp,     1000.0,  8000.0)
    UIEndPanel()

    -- Push the (possibly edited) values to the vehicle.
    veh:SetEngineForce(cfg.engine)
    veh:SetBrakeForce(cfg.brake)
    veh:SetMaxSteerAngle(cfg.steer)
    veh:SetAntiRoll(cfg.antiRoll)
    veh:SetGrip(cfg.grip, cfg.grip)
    veh:SetSuspension(cfg.stiff, cfg.damp)
end

function OnUpdate(dt)
    if not veh then return end

    local throttle, brake, steer = 0.0, 0.0, 0.0
    local speed = veh:GetForwardSpeed()

    if Input.GetKeyHeld(Key.W) or Input.GetKeyHeld(Key.Up) then throttle = 1.0 end
    if Input.GetKeyHeld(Key.S) or Input.GetKeyHeld(Key.Down) then
        if speed > 1.0 then brake = 1.0 else throttle = -0.6 end
    end
    if Input.GetKeyHeld(Key.A) or Input.GetKeyHeld(Key.Left) then steer = steer + 1.0 end
    if Input.GetKeyHeld(Key.D) or Input.GetKeyHeld(Key.Right) then steer = steer - 1.0 end

    local handbrake = Input.GetKeyHeld(Key.Space)
    veh:SetInputs(throttle, brake, steer, handbrake)

    -- Drive the visual wheels from the vehicle state.
    for i = 0, veh:NumWheels() - 1 do
        local e = wheelEnts[i]
        if e and e:Valid() then
            local t = e:GetTransform()
            t:SetLocalPosition(veh:WheelPosition(i))
            t:SetLocalOrientation(veh:WheelRotation(i))
        end
    end

    -- Chase camera: world-locked above/behind, looking at the car. Kept above
    -- ground and clear of the Quat::LookAt back-hemisphere flip (forward stays -Z).
    local camT = GetMainCameraTransform()
    if camT and chassisBody then
        local cp     = chassisBody:GetPosition()
        local camPos = cp + Vec3.new(0, 6.0, 12.0)
        camT:SetLocalPosition(camPos)
        -- Quat::LookAt uses forward = (from - to), so pass (target, eye) to look AT the car.
        camT:SetLocalOrientation(QuatLookAt(cp + Vec3.new(0, 1.0, 0), camPos))
    end

    UpdateUI()
end

function OnRelease()
    if veh then
        Physics.DestroyVehicle(veh)
        veh = nil
    end
end
