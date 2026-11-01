-- Walk around Sponza. The building gets a triangle-mesh collider built from its model,
-- the player is a physics capsule whose velocity is set here each frame.
-- WASD move, mouse look, Space jump, Shift run, Tab frees the mouse, LMB / J throws a ball,
-- B shows collision shapes, M next scene.

local RADIUS, SEGMENT = 0.35, 1.1 -- capsule: 1.8 tall
local HALF_HEIGHT     = RADIUS + SEGMENT * 0.5
local EYE             = 0.7       -- above the capsule centre
local WALK, RUN       = 5.0, 9.0
local JUMP            = 5.5
local ACCEL, AIR      = 60.0, 12.0

local hero, camera, level
local yaw, pitch = 90.0, 0.0
local captured   = true
local spawn
local fireTimer  = 0.0
local showShapes = false

local function body()
    return hero:GetRigidBody3DComponent():GetRigidBody()
end

local function respawn()
    local rb = body()
    rb:SetPosition(Vec3.new(spawn.x, spawn.y, spawn.z))
    rb:SetLinearVelocity(Vec3.new(0, 0, 0))
end

function OnInit()
    level = GetEntityByName(scene, "Sponza.gltf")
    -- baked to a file the first time, loaded from it after that
    AddMeshCollider(level, "//Assets/Imported/Sponza.lcol")

    -- a saved scene already has the player and its camera in it
    local savedHero = GetEntityByName(scene, "Player")
    if savedHero:Valid() then
        hero, camera = savedHero, GetEntityByName(scene, "PlayerCamera")
        local at = body():GetPosition()
        spawn = { x = at.x, y = at.y, z = at.z }
        Input.SetMouseCaptured(true)
        return
    end

    -- take over from the scene's fly camera, starting where it was
    local old = Entity.new(scene:GetEntityManager():GetRegistry():view_Camera():front(), scene)
    local p   = old:GetTransform():LocalPosition()
    spawn     = { x = p.x, y = p.y, z = p.z }
    old:Destroy()

    local screen = GetAppInstance():GetWindowSize()
    camera = scene:GetEntityManager():Create("PlayerCamera")
    camera:AddTransform():SetLocalPosition(Vec3.new(spawn.x, spawn.y, spawn.z))
    local cam = camera:AddCamera(screen.x / math.max(screen.y, 1.0), 10.0)
    cam:SetIsOrthographic(false)
    cam:SetFOV(75.0)
    cam:SetNearPlane(0.05)
    cam:SetFarPlane(800.0)

    hero = AddPhysicsCapsule(scene, "Player", Vec3.new(spawn.x, spawn.y, spawn.z), RADIUS, SEGMENT, 1.0, Vec4.new(1, 1, 1, 1))
    SetEntityVisible(hero, false)
    local rb = body()
    rb:SetFriction(0.0) -- this script decides how it stops
    rb:SetElasticity(0.0)
    rb:SetAngularFactor(Vec3.new(0, 0, 0))

    Input.SetMouseCaptured(true)
end

local function throwBall()
    local t = camera:GetTransform()
    local y, p = math.rad(yaw), math.rad(pitch)
    local dir = Vec3.new(-math.sin(y) * math.cos(p), math.sin(p), -math.cos(y) * math.cos(p))
    local from = t:GetWorldPosition()
    AddSphereEntity(scene, Vec3.new(from.x + dir.x, from.y + dir.y, from.z + dir.z), dir)
end

function OnUpdate(dt)
    if not hero then OnInit() end -- scripts loaded with a saved scene are not initialised
    if Input.GetKeyPressed(Key.M) then
        Input.SetMouseCaptured(false)
        SwitchScene()
        return
    end
    if Input.GetKeyPressed(Key.Tab) then
        captured = not captured
        Input.SetMouseCaptured(captured)
    end
    if Input.GetKeyPressed(Key.B) then
        showShapes = not showShapes
        SetPhysicsDebugFlags(showShapes and PhysicsDebugFlags.COLLISIONVOLUMES or 0)
    end

    -- hold still until there is a floor to stand on
    if GetMeshColliderTriangles(level) == 0 then
        respawn()
        return
    end

    if captured then
        local delta = Input.GetMouseDelta()
        yaw   = yaw - delta.x * 0.12
        pitch = math.max(-85.0, math.min(85.0, pitch - delta.y * 0.12))
    end

    local rb  = body()
    local pos = rb:GetPosition()
    local vel = rb:GetLinearVelocity()
    if pos.y < -50.0 then
        respawn()
        return
    end

    -- feet: a short ray down from the centre, ignoring our own capsule
    local hit      = Raycast(Vec3.new(pos.x, pos.y, pos.z), Vec3.new(0, -1, 0), HALF_HEIGHT + 0.15, rb)
    local grounded = hit:Hit() and hit.normal.y > 0.5

    local y      = math.rad(yaw)
    local fx, fz = -math.sin(y), -math.cos(y)
    local rx, rz = math.cos(y), -math.sin(y)
    local mx, mz = 0.0, 0.0
    if Input.GetKeyHeld(Key.W) then mx, mz = mx + fx, mz + fz end
    if Input.GetKeyHeld(Key.S) then mx, mz = mx - fx, mz - fz end
    if Input.GetKeyHeld(Key.D) then mx, mz = mx + rx, mz + rz end
    if Input.GetKeyHeld(Key.A) then mx, mz = mx - rx, mz - rz end
    local len = math.sqrt(mx * mx + mz * mz)
    if len > 0.0 then mx, mz = mx / len, mz / len end

    local speed  = Input.GetKeyHeld(Key.LeftShift) and RUN or WALK
    local rate   = (grounded and ACCEL or AIR) * dt
    local wantX, wantZ = mx * speed, mz * speed
    local dx, dz = wantX - vel.x, wantZ - vel.z
    local dl     = math.sqrt(dx * dx + dz * dz)
    local vx, vz = wantX, wantZ
    if dl > rate then vx, vz = vel.x + dx / dl * rate, vel.z + dz / dl * rate end

    local vy = vel.y
    if grounded and Input.GetKeyPressed(Key.Space) then vy = JUMP end
    rb:SetLinearVelocity(Vec3.new(vx, vy, vz))

    local t = camera:GetTransform()
    t:SetLocalPosition(Vec3.new(pos.x, pos.y + EYE, pos.z))
    t:SetLocalOrientation(Quat.new(pitch, yaw, 0.0))

    fireTimer = fireTimer - dt
    if Input.GetKeyPressed(Key.J) or (Input.GetMouseHeld(MouseButton.Left) and fireTimer <= 0.0) then
        throwBall()
        fireTimer = 0.25
    end
end

function OnRelease()
    Input.SetMouseCaptured(false)
end
