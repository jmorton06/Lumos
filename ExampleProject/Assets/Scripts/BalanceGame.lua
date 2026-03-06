-- 3D Balance Platformer Game
-- Control a sphere balancing on platforms

local player = nil
local playerBody = nil
local camera = nil
local cameraEntity = nil
local moveSpeed = 50.0
local jumpForce = 10.0
local cameraDistance = 20.0
local cameraHeight = 15.0
local initialized = false
local airControlFactor = 0.3
local maxSpeed = 15.0
local lastCameraPos = nil

-- Game state variables
local score = 0
local lives = 3
local gameTime = 0
local gameState = "playing" -- "playing", "paused", "gameover", "victory"
local playerInAir = false
local platforms = {}
local collectibles = {}
local obstacles = {}
local lastHitTime = 0
local hitCooldown = 2.0
local checkpoints = {}
local currentCheckpoint = nil
local goalPlatform = nil

function OnInit()
    print("Balance Game Initialized!")
    print("Controls: WASD/Arrow Keys to move, Space to jump")
end

function IsGrounded(playerPos)
    -- Check if within platform bounds horizontally and close vertically
    for i, platform in ipairs(platforms) do
        local dx = playerPos.x - platform.pos.x
        local dy = playerPos.y - platform.pos.y
        local dz = playerPos.z - platform.pos.z

        -- Check if within platform bounds horizontally and close vertically
        if math.abs(dx) < platform.scale.x and
           math.abs(dz) < platform.scale.z and
           dy > 0 and dy < 1.2 then
            return true
        end
    end

    return false
end

function OnUpdate(dt)
    -- Initialize on first update (after scene is fully loaded)
    if not initialized then
        Initialize()
        initialized = true
        return
    end

	OnImGui()
    -- Update game time
    if gameState == "playing" then
        gameTime = gameTime + dt
    end

    -- Handle pause
    if Input.GetKeyPressed(Key.Escape) then
        if gameState == "playing" then
            gameState = "paused"
            print("Game paused")
        elseif gameState == "paused" then
            gameState = "playing"
            print("Game resumed")
        end
    end

    -- Handle restart
    if Input.GetKeyPressed(Key.R) and gameState == "gameover" then
        ResetGame()
        return  -- Early exit to prevent accessing invalid entities
    end

    -- Handle replay after victory
    if Input.GetKeyPressed(Key.Space) and gameState == "victory" then
        ResetGame()
        return  -- Early exit to prevent accessing invalid entities
    end

    if player == nil or not player:Valid() then
        return
    end

    local playerComp = player:GetRigidBody3DComponent()
    if playerComp == nil then
        return
    end

    playerBody = playerComp:GetRigidBody()
    if playerBody == nil then
        return
    end

    -- Get player position
    local playerPos = playerBody:GetPosition()

    -- Only process gameplay when in playing state
    if gameState == "playing" then
        -- Check if grounded for air control
        local isGrounded = IsGrounded(playerPos)
        local currentAirControl = isGrounded and 1.0 or airControlFactor

        -- Handle input for movement
        local force = Vec3.new(0, 0, 0)

        -- WASD or Arrow keys for movement
        if Input.GetKeyHeld(Key.W) or Input.GetKeyHeld(Key.Up) then
            force.z = force.z - moveSpeed * currentAirControl
        end
        if Input.GetKeyHeld(Key.S) or Input.GetKeyHeld(Key.Down) then
            force.z = force.z + moveSpeed * currentAirControl
        end
        if Input.GetKeyHeld(Key.A) or Input.GetKeyHeld(Key.Left) then
            force.x = force.x - moveSpeed * currentAirControl
        end
        if Input.GetKeyHeld(Key.D) or Input.GetKeyHeld(Key.Right) then
            force.x = force.x + moveSpeed * currentAirControl
        end

        -- Apply movement force
        if force.x ~= 0 or force.z ~= 0 then
            playerBody:SetForce(force)
        end

        -- Clamp maximum speed
        local vel = playerBody:GetLinearVelocity()
        local horizontalSpeed = math.sqrt(vel.x * vel.x + vel.z * vel.z)
        if horizontalSpeed > maxSpeed then
            local scale = maxSpeed / horizontalSpeed
            vel.x = vel.x * scale
            vel.z = vel.z * scale
            playerBody:SetLinearVelocity(vel)
        end

        -- Jump
        if Input.GetKeyPressed(Key.Space) and IsGrounded(playerPos) then
            local velocity = playerBody:GetLinearVelocity()
            velocity.y = jumpForce
            playerBody:SetLinearVelocity(velocity)
        end

        -- Reset player if fallen off
        if playerPos.y < -20 then
            RespawnPlayer()
        end

        -- Detect landing for particle effects
        local velocity = playerBody:GetLinearVelocity()
        local wasInAir = playerInAir
        playerInAir = math.abs(velocity.y) > 0.5

        if wasInAir and not playerInAir then
            -- Just landed
            SpawnParticleEffect(playerPos, Vec4.new(0.7, 0.7, 0.9, 1.0), 10)
        end

        -- Note: Collectibles, obstacles, and checkpoints temporarily disabled
        -- CheckCollectibles(playerPos)
        -- UpdateObstacles(dt, playerPos)
        -- CheckCheckpoints(playerPos)

        -- Check goal
        CheckGoal(playerPos)
    end

    -- Update camera to follow player (even when paused)
    UpdateCamera(playerPos)
end

function Initialize()
    -- Create platforms first
    CreatePlatforms()

    -- Create the player sphere using the helper function
    -- We'll modify its physics after creation
    local playerPos = Vec3.new(0, 10, 0)
    AddSphereEntity(scene, playerPos, Vec3.new(0, 0, 0))

    -- Get the player entity (it's the last one created)
    local registry = scene:GetEntityManager():GetRegistry()
    local view = registry:view_RigidBody3DComponent()

    -- Get the front entity from the view (most recently created sphere)
    local entityHandle = view:front()
    if entityHandle then
        player = Entity.new(entityHandle, scene)
        if player:Valid() then
            local playerComp = player:GetRigidBody3DComponent()
            if playerComp then
                playerBody = playerComp:GetRigidBody()
                -- Reset velocity to zero (AddSphereEntity adds initial velocity)
                playerBody:SetLinearVelocity(Vec3.new(0, 0, 0))
                playerBody:SetPosition(playerPos)
                playerBody:SetFriction(0.8)
            end
        end
    end

    -- Setup camera
    SetupCamera()

    -- Note: Collectibles, obstacles, and checkpoints temporarily disabled
    -- These features require additional Lua bindings that aren't available yet

    -- Create goal platform (position-based, no entity needed)
    CreateGoalPlatform()

    print("Game initialized - use WASD/Arrows to move, Space to jump!")
end

function ResetGame()
    score = 0
    lives = 3
    gameTime = 0
    gameState = "playing"
    currentCheckpoint = nil
    lastHitTime = 0

    -- Reset checkpoints
    for i, cp in ipairs(checkpoints) do
        cp.activated = false
    end

    -- Reset collectibles (respawn them)
    for i, collectible in ipairs(collectibles) do
        if not collectible.active then
            collectible.active = true
            -- Respawn the collectible entity would require recreating it
            -- For simplicity, we'll note this limitation
        end
    end

    -- Validate player entity before accessing
    if player and player:Valid() then
        local playerComp = player:GetRigidBody3DComponent()
        if playerComp then
            playerBody = playerComp:GetRigidBody()
            if playerBody then
                playerBody:SetPosition(Vec3.new(0, 10, 0))
                playerBody:SetLinearVelocity(Vec3.new(0, 0, 0))
                playerBody:SetAngularVelocity(Vec3.new(0, 0, 0))
            end
        end
    else
        -- Player entity is invalid, need to reinitialize
        print("Player entity invalid, reinitializing...")
        initialized = false
    end

    print("Game reset!")
end

function CreatePlatforms()
    -- Store platform data for ground detection
    local platformData = {
        {pos = Vec3.new(0, -1, 0), scale = Vec3.new(10, 0.5, 10)},
        {pos = Vec3.new(0, 0, 0), scale = Vec3.new(5, 0.5, 5)},
        {pos = Vec3.new(8, 1, 0), scale = Vec3.new(4, 0.5, 4)},
        {pos = Vec3.new(8, 2, 8), scale = Vec3.new(3, 0.5, 3)},
        {pos = Vec3.new(0, 3, 12), scale = Vec3.new(4, 0.5, 4)},
        {pos = Vec3.new(-8, 4, 12), scale = Vec3.new(3, 0.5, 3)},
        {pos = Vec3.new(-12, 5, 4), scale = Vec3.new(4, 0.5, 4)},
        {pos = Vec3.new(-8, 6, -4), scale = Vec3.new(3, 0.5, 3)},
        {pos = Vec3.new(0, 7, -8), scale = Vec3.new(5, 0.5, 5)},
        {pos = Vec3.new(4, 2, 4), scale = Vec3.new(2, 0.5, 2)},
        {pos = Vec3.new(-4, 3, 8), scale = Vec3.new(2, 0.5, 2)},
        {pos = Vec3.new(-10, 5, 8), scale = Vec3.new(2, 0.5, 2)},
        {pos = Vec3.new(0, 10, 0), scale = Vec3.new(6, 0.5, 6)}
    }

    -- Create platforms and store data
    for i, data in ipairs(platformData) do
        AddPlatform(scene, data.pos, data.scale)
        table.insert(platforms, data)
    end

    print("Platforms created!")
end

function SetupCamera()
    -- Get or create camera entity
    local registry = scene:GetEntityManager():GetRegistry()
    local cameraView = registry:view_Camera()

    local cameraHandle = cameraView:front()
    if cameraHandle then
        cameraEntity = Entity.new(cameraHandle, scene)
    else
        cameraEntity = scene:GetEntityManager():Create("Camera")
        cameraEntity:AddCamera(45.0, 1000.0)
    end

    camera = cameraEntity:GetCamera()
    print("Camera setup complete!")
end

function UpdateCamera(playerPos)
    if cameraEntity == nil or not cameraEntity:Valid() then
        return
    end

    -- Calculate target camera position (behind and above the player)
    local targetCameraPos = Vec3.new(
        playerPos.x,
        playerPos.y + cameraHeight,
        playerPos.z + cameraDistance
    )

    -- Initialize last camera position if needed
    if lastCameraPos == nil then
        lastCameraPos = targetCameraPos
    end

    -- Smooth camera following with lerp
    local lerpFactor = 0.1
    lastCameraPos = Vec3.new(
        lastCameraPos.x + (targetCameraPos.x - lastCameraPos.x) * lerpFactor,
        lastCameraPos.y + (targetCameraPos.y - lastCameraPos.y) * lerpFactor,
        lastCameraPos.z + (targetCameraPos.z - lastCameraPos.z) * lerpFactor
    )

    -- Set camera transform
    local cameraTransform = cameraEntity:GetTransform()
    cameraTransform:SetLocalPosition(lastCameraPos)

    -- Make camera look at player
    local direction = Vec3.new(
        playerPos.x - lastCameraPos.x,
        playerPos.y - lastCameraPos.y,
        playerPos.z - lastCameraPos.z
    )

    -- Calculate rotation to look at player
    local pitch = math.atan2(direction.y, math.sqrt(direction.x * direction.x + direction.z * direction.z))
    local yaw = math.atan2(direction.x, -direction.z)  -- Fixed: negate Z for correct direction

    -- Create quaternion from pitch and yaw
    local rotation = Quat.new(math.deg(pitch), math.deg(yaw), 0.0)
    cameraTransform:SetLocalOrientation(rotation)
end

function SpawnParticleEffect(position, color, count)
    local particleEntity = scene:GetEntityManager():Create("Particles")
    local transform = particleEntity:GetTransform()
    transform:SetLocalPosition(position)

    local emitter = particleEntity:AddParticleEmitter()
    emitter:SetPosition(position)
    emitter:SetVelocity(Vec3.new(0, 5, 0))
    emitter:SetVelocityVariation(Vec3.new(2, 2, 2))
    emitter:SetStartColor(color)
    emitter:SetEndColor(Vec4.new(color.x, color.y, color.z, 0))
    emitter:SetStartSize(0.3)
    emitter:SetEndSize(0.1)
    emitter:SetParticleLifeTime(0.5)
    emitter:SetSpawnRate(count / 0.5)
end

function SpawnCollectible(position)
    -- Create a small sphere for the collectible
    AddSphereEntity(scene, position, Vec3.new(0, 0, 0))

    -- Get the entity reference
    local view = scene:GetEntityManager():GetRegistry():view_RigidBody3DComponent()
    local handle = view:front()
    local coinEntity = Entity.new(handle, scene)
    local coinBody = coinEntity:GetRigidBody3DComponent():GetRigidBody()

    -- Make it static and positioned correctly
    coinBody:SetIsStatic(true)
    coinBody:SetPosition(position)

    -- Store in collectibles table
    table.insert(collectibles, {
        entity = coinEntity,
        body = coinBody,
        position = position,
        active = true
    })
end

function CheckCollectibles(playerPos)
    for i, collectible in ipairs(collectibles) do
        if collectible.active then
            local dx = playerPos.x - collectible.position.x
            local dy = playerPos.y - collectible.position.y
            local dz = playerPos.z - collectible.position.z
            local distance = math.sqrt(dx*dx + dy*dy + dz*dz)

            if distance < 1.5 then -- Collection radius
                -- Collect the coin
                collectible.active = false
                score = score + 10

                -- Destroy entity
                scene:GetEntityManager():DestroyEntity(collectible.entity)

                -- Spawn particle effect (gold color)
                SpawnParticleEffect(collectible.position, Vec4.new(1, 0.8, 0, 1), 20)

                print("Collected coin! Score: " .. score)
            end
        end
    end
end

function SpawnObstacle(startPos, endPos, speed, damage)
    -- Create a cuboid for the obstacle
    AddCuboidEntity(scene, startPos, Vec3.new(2, 0.5, 2))

    local view = scene:GetEntityManager():GetRegistry():view_RigidBody3DComponent()
    local handle = view:front()
    local obstacleEntity = Entity.new(handle, scene)
    local obstacleBody = obstacleEntity:GetRigidBody3DComponent():GetRigidBody()

    obstacleBody:SetIsStatic(true)

    table.insert(obstacles, {
        entity = obstacleEntity,
        body = obstacleBody,
        startPos = startPos,
        endPos = endPos,
        currentPos = startPos,
        speed = speed,
        direction = 1,
        damage = damage
    })
end

function UpdateObstacles(dt, playerPos)
    for i, obs in ipairs(obstacles) do
        -- Calculate movement direction
        local dx = obs.endPos.x - obs.startPos.x
        local dy = obs.endPos.y - obs.startPos.y
        local dz = obs.endPos.z - obs.startPos.z
        local length = math.sqrt(dx*dx + dy*dy + dz*dz)

        if length > 0 then
            -- Normalize direction
            dx, dy, dz = dx/length, dy/length, dz/length

            -- Move obstacle
            obs.currentPos = Vec3.new(
                obs.currentPos.x + dx * obs.speed * obs.direction * dt,
                obs.currentPos.y + dy * obs.speed * obs.direction * dt,
                obs.currentPos.z + dz * obs.speed * obs.direction * dt
            )

            obs.body:SetPosition(obs.currentPos)

            -- Check if reached end, reverse direction
            local distToEnd = math.sqrt(
                (obs.currentPos.x - obs.endPos.x)^2 +
                (obs.currentPos.y - obs.endPos.y)^2 +
                (obs.currentPos.z - obs.endPos.z)^2
            )
            local distToStart = math.sqrt(
                (obs.currentPos.x - obs.startPos.x)^2 +
                (obs.currentPos.y - obs.startPos.y)^2 +
                (obs.currentPos.z - obs.startPos.z)^2
            )

            if distToEnd < 0.5 then obs.direction = -1 end
            if distToStart < 0.5 then obs.direction = 1 end
        end

        -- Check collision with player (distance-based)
        local distToPlayer = math.sqrt(
            (obs.currentPos.x - playerPos.x)^2 +
            (obs.currentPos.y - playerPos.y)^2 +
            (obs.currentPos.z - playerPos.z)^2
        )

        if distToPlayer < 2.0 and (gameTime - lastHitTime) > hitCooldown then
            OnPlayerHit(obs.damage)
            lastHitTime = gameTime
        end
    end
end

function OnPlayerHit(damage)
    lives = lives - damage
    print("Hit! Lives: " .. lives)

    -- Spawn red particle effect at player position
    if playerBody then
        local pos = playerBody:GetPosition()
        SpawnParticleEffect(pos, Vec4.new(1, 0, 0, 1), 15)
    end

    if lives <= 0 then
        gameState = "gameover"
        print("Game Over!")
    else
        -- Small knockback effect
        if playerBody then
            local vel = playerBody:GetLinearVelocity()
            vel.y = 5.0
            playerBody:SetLinearVelocity(vel)
        end
    end
end

function CreateCheckpoint(position, index)
    -- Create a cuboid for the checkpoint
    AddCuboidEntity(scene, position, Vec3.new(1, 2, 1))

    local view = scene:GetEntityManager():GetRegistry():view_RigidBody3DComponent()
    local handle = view:front()
    local checkpointEntity = Entity.new(handle, scene)
    local checkpointBody = checkpointEntity:GetRigidBody3DComponent():GetRigidBody()

    checkpointBody:SetIsStatic(true)

    table.insert(checkpoints, {
        entity = checkpointEntity,
        body = checkpointBody,
        position = position,
        index = index,
        activated = false
    })
end

function CheckCheckpoints(playerPos)
    for i, cp in ipairs(checkpoints) do
        if not cp.activated then
            local dist = math.sqrt(
                (playerPos.x - cp.position.x)^2 +
                (playerPos.y - cp.position.y)^2 +
                (playerPos.z - cp.position.z)^2
            )

            if dist < 2.0 then
                cp.activated = true
                currentCheckpoint = cp
                print("Checkpoint " .. cp.index .. " activated!")
                SpawnParticleEffect(cp.position, Vec4.new(0, 1, 0, 1), 30)
            end
        end
    end
end

function RespawnPlayer()
    if not playerBody then
        return
    end

    local spawnPos = currentCheckpoint and currentCheckpoint.position or Vec3.new(0, 10, 0)
    playerBody:SetPosition(spawnPos)
    playerBody:SetLinearVelocity(Vec3.new(0, 0, 0))
    playerBody:SetAngularVelocity(Vec3.new(0, 0, 0))
end

function CreateGoalPlatform()
    local goalPos = Vec3.new(0, 10, 0) -- Top platform position
    goalPlatform = {
        position = goalPos,
        radius = 6.0
    }
    print("Goal platform created at top!")
end

function CheckGoal(playerPos)
    if goalPlatform then
        local dist = math.sqrt(
            (playerPos.x - goalPlatform.position.x)^2 +
            (playerPos.y - goalPlatform.position.y)^2 +
            (playerPos.z - goalPlatform.position.z)^2
        )

        if dist < goalPlatform.radius then
            gameState = "victory"
            print("Level Complete! Score: " .. score .. " Time: " .. string.format("%.1f", gameTime))
        end
    end
end

function OnImGui()
    -- Game HUD
    imgui.beginWindow("Game HUD") -- 35 = NoTitleBar | NoResize | NoMove
    imgui.setWindowPos(imgui.ImVec2.new(10, 10), 2) -- 2 = Always
    imgui.setWindowSize(imgui.ImVec2.new(250, 120), 2)

    imgui.text("Score: " .. score)
    imgui.text("Lives: " .. lives)
    imgui.text("Time: " .. string.format("%.1f", gameTime))

    if gameState == "paused" then
        imgui.text("PAUSED - Press ESC to Resume")
    elseif gameState == "gameover" then
        imgui.text("GAME OVER - Press R to Restart")
    elseif gameState == "victory" then
        imgui.text("VICTORY! Final Score: " .. score)
        imgui.text("Press SPACE to Replay")
    end

    imgui.endWindow()
end
