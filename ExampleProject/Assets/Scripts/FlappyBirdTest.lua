pillars = {}

-- Game constants
INITIAL_SPEED = 7
INITIAL_GAP_SIZE = 4.0
INITIAL_VERTICAL_SPEED = 8
MAX_HEIGHT = 10.0
PILLAR_SPACING = 10.0
INITIAL_PILLAR_TARGET = 35.0
SCORE_UI_OFFSET_Y = 8.0
NUM_PILLAR_PAIRS = 5  -- 10 pillars total (5 pairs)

-- Difficulty scaling
MIN_GAP_SIZE = 2.5
SPEED_INCREASE_PER_SCORE = 0.1
GAP_DECREASE_PER_SCORE = 0.05

-- Screen shake constants
SHAKE_DURATION = 0.4
SHAKE_INTENSITY = 1.0

-- Camera smoothing
CAMERA_SMOOTHING = 8.0

-- Countdown
COUNTDOWN_DURATION = 3.0

-- Combo system
CLOSE_PASS_THRESHOLD = 1.5  -- How close to pillar edge for combo
COMBO_BONUS = 2
COMBO_TIMEOUT = 2.0

-- Parallax layers (z-depth, scroll speed multiplier)
PARALLAX_LAYERS = {
    { z = -9.0, speed = 0.2, count = 20 },  -- Far background
    { z = -7.0, speed = 0.5, count = 30 },  -- Mid background
    { z = -5.0, speed = 0.8, count = 40 },  -- Near background
}

-- Runtime state
speed = INITIAL_SPEED
verticalSpeed = INITIAL_VERTICAL_SPEED
gapSize = INITIAL_GAP_SIZE
furthestPillarPosX = 0
pillarTarget = INITIAL_PILLAR_TARGET
pillarIndex = 1

GameStates = {
    Running = 0,
    GameOver = 1,
    Start = 2,
    Paused = 3,
    Countdown = 4
}

gameState = GameStates.Start
entityManager = nil

player = nil
camera = nil
scoreEntity = nil
score = 0
highScore = 0
iconTexture = nil
gameOverTexture = nil
gameOverEntity = nil
gameOverScale = 1.0
totalTime = 0.0
gameOverSize = Vec2.new(30, 4)
showImguiWindow = false

-- Screen shake state
shakeTimer = 0.0
shakeOffset = Vec3.new(0, 0, 0)

-- Camera smoothing state
cameraTargetX = 0.0
cameraBaseX = 0.0  -- Camera position without shake

-- Countdown state
countdownTimer = 0.0
countdownEntity = nil

-- Combo state
comboCount = 0
comboTimer = 0.0
lastPillarPassed = -1

-- Score popup state
scorePopups = {}

-- Parallax backgrounds (multiple layers)
parallaxLayers = {}

-- Screen shake function
function StartScreenShake()
    shakeTimer = SHAKE_DURATION
end

function UpdateScreenShake(dt)
    if shakeTimer > 0 then
        shakeTimer = shakeTimer - dt
        local intensity = SHAKE_INTENSITY * (shakeTimer / SHAKE_DURATION)
        shakeOffset = Vec3.new(
            Rand(-intensity, intensity),
            Rand(-intensity, intensity),
            0
        )
    else
        shakeOffset = Vec3.new(0, 0, 0)
    end
end

-- Score popup functions
function CreateScorePopup(pos, text, color)
    local popup = {
        entity = entityManager:Create(),
        timer = 1.0,
        startY = pos.y
    }
    popup.entity:AddTransform():SetLocalPosition(pos)
    popup.entity:AddTextComponent()
    popup.entity:GetTextComponent().TextString = text
    popup.entity:GetTextComponent().Colour = color or Vec4.new(1.0, 1.0, 0.0, 1.0)
    popup.entity:GetTransform():SetLocalScale(Vec3.new(1.5, 1.5, 1.5))
    table.insert(scorePopups, popup)
end

function UpdateScorePopups(dt)
    for i = #scorePopups, 1, -1 do
        local popup = scorePopups[i]
        popup.timer = popup.timer - dt
        if popup.timer <= 0 then
            if popup.entity and popup.entity:Valid() then
                popup.entity:Destroy()
            end
            table.remove(scorePopups, i)
        else
            -- Float upward and fade
            local pos = popup.entity:GetTransform():LocalPosition()
            pos.y = popup.startY + (1.0 - popup.timer) * 2.0
            popup.entity:GetTransform():SetLocalPosition(pos)
            local alpha = popup.timer
            local color = popup.entity:GetTextComponent().Colour
            popup.entity:GetTextComponent().Colour = Vec4.new(color.x, color.y, color.z, alpha)
        end
    end
end

function EndGame()
    gameState = GameStates.GameOver

    -- Start screen shake
    StartScreenShake()

    -- Update high score
    if score > highScore then
        highScore = score
    end

    if gameOverEntity == nil then
        gameOverEntity = entityManager:Create()
        gameOverEntity:AddSprite(Vec2.new(0.0, 0.0), gameOverSize, Vec4.new(1.0, 1.0, 1.0, 1.0)):SetTexture(gameOverTexture)
        gameOverEntity:GetTransform():SetLocalScale(Vec3.new(0.2, 0.2, 0.2))
    end
    if player then
        local gameOverPos = player:GetTransform():GetWorldPosition() + Vec3.new(15.0, 2.0, 1.0)
        gameOverEntity:GetTransform():SetLocalPosition(gameOverPos)
    end
end

function beginContact(a, b, approachSpeed)
    EndGame()
end

-- Start countdown before game begins
function StartCountdown()
    gameState = GameStates.Countdown
    countdownTimer = COUNTDOWN_DURATION

    if countdownEntity == nil then
        countdownEntity = entityManager:Create()
        countdownEntity:AddTransform()
        countdownEntity:AddTextComponent()
        countdownEntity:GetTextComponent().Colour = Vec4.new(1.0, 1.0, 1.0, 1.0)
        countdownEntity:GetTransform():SetLocalScale(Vec3.new(4.0, 4.0, 4.0))
    end
end

function UpdateCountdown(dt)
    countdownTimer = countdownTimer - dt

    if countdownEntity then
        local pos = camera:GetTransform():GetWorldPosition()
        countdownEntity:GetTransform():SetLocalPosition(Vec3.new(pos.x - 1.0, pos.y + 2.0, 1.0))

        local displayNum = math.ceil(countdownTimer)
        if displayNum > 0 then
            countdownEntity:GetTextComponent().TextString = tostring(displayNum)
        else
            countdownEntity:GetTextComponent().TextString = "GO!"
        end
    end

    if countdownTimer <= -0.5 then
        -- Countdown finished, start game
        if countdownEntity and countdownEntity:Valid() then
            countdownEntity:Destroy()
            countdownEntity = nil
        end
        gameState = GameStates.Running
        player:GetRigidBody2DComponent():GetRigidBody():SetIsStatic(false)
        PlayerJump()
    end
end

function CreatePlayer()
    local texture = LoadTextureWithParams("icon", "//Assets/Textures/TappyPlane/PNG/Planes/planeBlue1.png", TextureFilter.Linear, TextureWrap.ClampToEdge)

    player = entityManager:Create()
    player:AddSprite(Vec2.new(-0.9, -0.8), Vec2.new(1.7, 1.5), Vec4.new(1.0,1.0,1.0,1.0)):SetTexture(texture)

    local params = RigidBodyParameters.new()
    params.position = Vec3.new(1.0, 1.0, 1.0)
    params.scale = Vec3.new(1.7 / 2.0, 1.5 / 2.0, 1.0)
    params.shape = Shape.Circle
    params.isStatic = false

    player:GetTransform():SetLocalPosition(Vec3.new(1.0,1.0,1.0))
    player:AddRigidBody2DComponent(params):GetRigidBody():SetForce(Vec2.new(0.0,0.0))
    player:GetRigidBody2DComponent():GetRigidBody():SetIsStatic(true)
    player:GetRigidBody2DComponent():GetRigidBody():SetLinearDamping(0.1)
    local emitter = player:AddParticleEmitter()
    emitter:SetParticleCount(512)
    emitter:SetParticleRate(0.1)
    emitter:SetNumLaunchParticles(100)
    emitter:SetParticleLife(0.9)
    emitter:SetParticleSize(0.2)
    emitter:SetGravity(Vec3.new(0.0, -1.0, 0.0))
    emitter:SetSpread(Vec3.new(1.0, 0.4, 0.0))
    emitter:SetVelocitySpread(Vec3.new(0.0, 0.8, 0.0))
    emitter:SetInitialVelocity(Vec3.new(-3.0, 0.0, 0.0))
	emitter:SetInitialColour(Vec4.new(0.8,0.8,0.8,0.6))
	emitter:SetFadeOut(0.9)
	emitter:SetFadeIn(2.0)
    SetCallback(beginContact, player:GetRigidBody2DComponent():GetRigidBody():GetB2Body())
end

function CreatePillar(index, offset)

    pillars[index] = entityManager:Create()
    pillars[index + 1] = entityManager:Create()

    local centre = Rand(-6.0, 6.0);
    local width = Rand(1.0, 2.0)

    local topY = MAX_HEIGHT;
	local bottomY = centre + (gapSize / 2.0);

	local pos = Vec3.new(offset / 2.0, ((topY - bottomY )/ 2.0) + bottomY, 0.0);
	local scale = Vec3.new(width, (topY - bottomY) / 2.0, 1.0);

    pillars[index]:AddSprite(Vec2.new(-scale.x, -scale.y), Vec2.new(scale.x * 2.0, scale.y * 2.0), Vec4.new(1.0,1.0,1.0,1.0)):SetTexture(iconTexture)

    local params = RigidBodyParameters.new()
	params.position = pos
	params.scale = scale
	params.shape = Shape.Custom
	params.isStatic = true
	params.customShapePositions = { Vec2.new(-scale.x, -scale.y), Vec2.new(scale.x * 0.25, scale.y * 1.05), Vec2.new(scale.x, -scale.y)}

    pillars[index]:GetTransform():SetLocalPosition(pos)
    pillars[index]:AddRigidBody2DComponent(params):GetRigidBody():SetOrientation(math.pi)

    topY = centre - (gapSize / 2.0)
    bottomY = -MAX_HEIGHT;
    width = Rand(1.0, 2.0)
	pos = Vec3.new(offset / 2.0, ((topY - bottomY) / 2.0) + bottomY, 0.0)
	scale = Vec3.new(width, (topY - bottomY) / 2.0, 1.0)

    pillars[index + 1]:AddSprite(Vec2.new(-scale.x, -scale.y), Vec2.new(scale.x * 2.0, scale.y * 2.0), Vec4.new(1.0,1.0,1.0,1.0)):SetTexture(iconTexture)

	params.position = pos
	params.scale = scale
	params.shape = Shape.Custom
	params.isStatic = true
	params.customShapePositions = { Vec2.new(-scale.x, -scale.y), Vec2.new(scale.x * 0.25, scale.y * 1.05), Vec2.new(scale.x, -scale.y)}

    pillars[index + 1]:GetTransform():SetLocalPosition(pos)
    pillars[index + 1]:AddRigidBody2DComponent(params)

    if pos.x > furthestPillarPosX then
        furthestPillarPosX = pos.x;
    end
end

function CreateBackground(layerIndex, index, z)
    local layer = parallaxLayers[layerIndex]
    if not layer then
        parallaxLayers[layerIndex] = { entities = {}, speed = PARALLAX_LAYERS[layerIndex].speed }
        layer = parallaxLayers[layerIndex]
    end

    local entity = entityManager:Create()
    entity:AddTransform():SetLocalPosition(Vec3.new(index * 20.0 - 40.0, 0.0, z))

    -- Vary tint slightly per layer for depth
    local tint = 0.6 + (0.4 * (1.0 - PARALLAX_LAYERS[layerIndex].speed))
    entity:AddSprite(Vec2.new(-10.0, -10.0), Vec2.new(20.0, 20.0), Vec4.new(tint, tint, tint, 1.0)):SetTexture(backgroundTexture)

    layer.entities[index] = entity
end

function UpdateParallax(cameraX)
    for layerIndex, layer in ipairs(parallaxLayers) do
        local parallaxX = cameraX * layer.speed
        for _, entity in pairs(layer.entities) do
            if entity and entity:Valid() then
                local pos = entity:GetTransform():LocalPosition()
                -- Keep background tiled by wrapping
                local baseX = pos.x - parallaxX
                entity:GetTransform():SetLocalPosition(Vec3.new(pos.x, pos.y, pos.z))
            end
        end
    end
end

backgrounds = {}

function OnInit()
    iconTexture = LoadTextureWithParams("icon", "//Assets/Textures/TappyPlane/PNG/rock.png", TextureFilter.Linear, TextureWrap.ClampToEdge)
    gameOverTexture = LoadTextureWithParams("gameOver", "//Assets/Textures/TappyPlane/PNG/UI/textGameOver.png", TextureFilter.Linear, TextureWrap.ClampToEdge)

    entityManager = scene:GetEntityManager()

    SetB2DGravity(Vec2.new(0.0, -9.81 * 2.0))

    camera = entityManager:Create()
    camera:AddTransform()

    scoreEntity = entityManager:Create()
    scoreEntity:AddTransform()
    scoreEntity:AddTextComponent()
    scoreEntity:GetTextComponent().Colour = Vec4.new(0.4, 0.1, 0.9, 1.0)
    scoreEntity:GetTextComponent().TextString = ""
    scoreEntity:GetTransform():SetLocalPosition(Vec3.new(-4.0, SCORE_UI_OFFSET_Y, 0.0))
    scoreEntity:GetTransform():SetLocalScale(Vec3.new(2.0, 2.0, 2.0))

    local screenSize = GetAppInstance():GetWindowSize()
    camera:AddCamera(screenSize.x / screenSize.y, 10.0, 1.0)

    CreatePlayer()

    for i = 1, NUM_PILLAR_PAIRS * 2, 2 do
        CreatePillar(i, (i + 2) * PILLAR_SPACING)
    end

    backgroundTexture = LoadTextureWithParams("background", "//Assets/Textures/TappyPlane/PNG/background.png", TextureFilter.Linear, TextureWrap.ClampToEdge)

    -- Create parallax background layers
    for layerIndex, layerConfig in ipairs(PARALLAX_LAYERS) do
        for i = 1, layerConfig.count do
            CreateBackground(layerIndex, i, layerConfig.z)
        end
    end
end

function PlayerJump()

	local phys = player:GetRigidBody2DComponent()
	local vel = phys:GetRigidBody():GetLinearVelocity()

    vel.y = verticalSpeed
    vel.x = speed

    phys:GetRigidBody():SetLinearVelocity(vel)
end

function OnUpdate(dt)
    tracy.ZoneBegin()

    -- Update screen shake
    UpdateScreenShake(dt)

    -- Update score popups
    UpdateScorePopups(dt)

    if Input.GetKeyPressed(Key.M) then
        tracy.ZoneEnd()
        SwitchScene()
        return
    end

    -- Pause toggle (Escape key)
    if Input.GetKeyPressed(Key.Escape) then
        if gameState == GameStates.Running then
            gameState = GameStates.Paused
        elseif gameState == GameStates.Paused then
            gameState = GameStates.Running
        end
    end

    if Input.GetKeyPressed(Key.K) then
        speed = math.max(0, speed - 1)
    end
    if Input.GetKeyPressed(Key.L) then
        speed = speed + 1
    end
    if Input.GetKeyPressed(Key.O) then
        gapSize = math.max(MIN_GAP_SIZE, gapSize - 0.5)
    end
    if Input.GetKeyPressed(Key.P) then
        gapSize = gapSize + 0.5
    end
    if Input.GetKeyPressed(Key.R) then
        Reset()
    end

    -- Handle countdown state
    if gameState == GameStates.Countdown then
        UpdateCountdown(dt)

    elseif gameState == GameStates.Paused then
        -- Game is paused, don't update physics

    elseif gameState == GameStates.Running then
        if Input.GetKeyHeld(Key.Space) or Input.GetMouseClicked(MouseButton.Left) then
            PlayerJump()
        end

        local pos = player:GetTransform():GetWorldPosition()

        if pos.y > MAX_HEIGHT or pos.y < -MAX_HEIGHT then
            EndGame()
        end

        -- Smooth camera follow (track base position separately from shake)
        cameraTargetX = pos.x
        cameraBaseX = cameraBaseX + (cameraTargetX - cameraBaseX) * CAMERA_SMOOTHING * dt
        local camPos = Vec3.new(cameraBaseX + shakeOffset.x, shakeOffset.y, 0.0)
        camera:GetTransform():SetLocalPosition(camPos)

        -- Update combo timer
        if comboTimer > 0 then
            comboTimer = comboTimer - dt
            if comboTimer <= 0 then
                comboCount = 0
            end
        end

        local newScore = math.max(math.floor((pos.x - 5) / PILLAR_SPACING), 0)
        if newScore > score then
            local pillarNum = newScore

            -- Check for close pass (combo system)
            if pillarNum > lastPillarPassed then
                lastPillarPassed = pillarNum

                -- Check distance to nearest pillar edge
                local playerY = player:GetTransform():GetWorldPosition().y
                local closestDist = MAX_HEIGHT

                -- Find closest pillar gap center at current x
                for i = 1, NUM_PILLAR_PAIRS * 2, 2 do
                    local p = pillars[i]
                    if p and p:Valid() then
                        local pillarPos = p:GetTransform():LocalPosition()
                        if math.abs(pillarPos.x - pos.x) < 5 then
                            -- This is a nearby pillar pair
                            local gapCenter = pillarPos.y - (MAX_HEIGHT - pillarPos.y) / 2
                            closestDist = math.min(closestDist, math.abs(playerY))
                        end
                    end
                end

                -- Award combo for close pass
                if closestDist < CLOSE_PASS_THRESHOLD then
                    comboCount = comboCount + 1
                    comboTimer = COMBO_TIMEOUT
                    local bonus = COMBO_BONUS * comboCount
                    score = newScore + bonus

                    -- Show combo popup
                    local popupPos = Vec3.new(pos.x + 2, pos.y + 1, 0.5)
                    CreateScorePopup(popupPos, "CLOSE! +" .. bonus .. " (x" .. comboCount .. ")", Vec4.new(1.0, 0.8, 0.0, 1.0))
                else
                    score = newScore
                    comboCount = 0

                    -- Show normal score popup
                    local popupPos = Vec3.new(pos.x + 2, pos.y + 1, 0.5)
                    CreateScorePopup(popupPos, "+1", Vec4.new(1.0, 1.0, 1.0, 1.0))
                end
            end

            -- Difficulty scaling
            speed = INITIAL_SPEED + (score * SPEED_INCREASE_PER_SCORE)
            gapSize = math.max(MIN_GAP_SIZE, INITIAL_GAP_SIZE - (score * GAP_DECREASE_PER_SCORE))
        end

        scoreEntity:GetTransform():SetLocalPosition(Vec3.new(cameraBaseX, SCORE_UI_OFFSET_Y, 0.0))
        local scoreText = "Score: " .. tostring(score)
        if comboCount > 1 then
            scoreText = scoreText .. "  Combo: x" .. tostring(comboCount)
        end
        if highScore > 0 then
            scoreText = scoreText .. "  Best: " .. tostring(highScore)
        end
        scoreEntity:GetTextComponent().TextString = scoreText

        if pos.x > pillarTarget then
            local p1 = pillars[pillarIndex]
            local p2 = pillars[pillarIndex + 1]
            if p1 and p1:Valid() then
                p1:Destroy()
            end
            if p2 and p2:Valid() then
                p2:Destroy()
            end

            CreatePillar(pillarIndex, furthestPillarPosX * 2.0 + (PILLAR_SPACING * 2))

            pillarIndex = pillarIndex + 2
            if pillarIndex > NUM_PILLAR_PAIRS * 2 then
                pillarIndex = pillarIndex - (NUM_PILLAR_PAIRS * 2)
            end

            pillarTarget = pillarTarget + PILLAR_SPACING
        end

    elseif gameState == GameStates.GameOver then
        -- Apply screen shake to camera during game over (use base position + shake)
        local camPos = Vec3.new(cameraBaseX + shakeOffset.x, shakeOffset.y, 0.0)
        camera:GetTransform():SetLocalPosition(camPos)

        totalTime = totalTime + dt * 2
        gameOverScale = 0.2 + (math.sin(totalTime) + 1.0) / 10.0
        if gameOverEntity then
            gameOverEntity:GetTransform():SetLocalScale(Vec3.new(gameOverScale, gameOverScale, gameOverScale))
            -- Position game over text relative to base camera (not shaken position)
            gameOverEntity:GetTransform():SetLocalPosition(Vec3.new(cameraBaseX - (gameOverScale * gameOverSize.x) / 2, -(gameOverScale * gameOverSize.y) / 2, 2.0))
        end

    elseif gameState == GameStates.Start then
       scoreEntity:GetTransform():SetLocalPosition(Vec3.new(-4.0, SCORE_UI_OFFSET_Y, 0.0))

       cameraBaseX = player:GetTransform():GetWorldPosition().x
       camera:GetTransform():SetLocalPosition(Vec3.new(cameraBaseX, 0.0, 0.0))
    end

    -- ImGui Panel
    if gui and showImguiWindow then
        if gui.beginWindow("Tuning") then
            gui.text("Runtime tuning:")
            gui.inputFloat("Speed", speed, 0.0, 30.0, function(v) speed = v end)
            gui.inputFloat("Gap Size", gapSize, 1.0, 12.0, function(v) gapSize = v end)
            gui.inputFloat("Vertical Speed", verticalSpeed, 0.0, 30.0, function(v) verticalSpeed = v end)
            if gui.button("Reset") then
                Reset()
            end
        end
        gui.endWindow()
    end

    if GetUIState then
        UIPushStyle(StyleVar.BackgroundColor, Vec4.new(0.08, 0.08, 0.08, 0.7))
        UIPushStyle(StyleVar.BorderColor, Vec4.new(0.4, 0.4, 0.4, 0.8))
        UIPushStyle(StyleVar.TextColor, Vec4.new(1.0, 1.0, 1.0, 1.0))

        if gameState == GameStates.Start or gameState == GameStates.GameOver then
            UIBeginPanel("FlappyMenu", WidgetFlags.StackVertically | WidgetFlags.CentreX | WidgetFlags.CentreY)

            if gameState == GameStates.GameOver then
                UILabel("Title", "Game Over!")
                UILabel("FinalScore", "Score: " .. tostring(score))
                if score >= highScore and score > 0 then
                    UILabel("NewBest", "NEW BEST!")
                end
            else
                UILabel("Title", "Flappy Test")
            end

            local startBtn = UIButton(gameState == GameStates.GameOver and "Play Again" or "Start")
            if startBtn.clicked then
                if gameState == GameStates.GameOver then
                    Reset()
                end
                StartCountdown()
            end

            local resetBtn = UIButton("Reset")
            if resetBtn.clicked then
                Reset()
            end

            showImguiWindow = UIToggle("Show ImGui Window", showImguiWindow)

            local exitBtn = UIButton("Exit")
            if exitBtn.clicked then
                ExitApp()
            end

            UIEndPanel()

        elseif gameState == GameStates.Paused then
            UIBeginPanel("PauseMenu", WidgetFlags.StackVertically | WidgetFlags.CentreX | WidgetFlags.CentreY)

            UILabel("PauseTitle", "PAUSED")

            local resumeBtn = UIButton("Resume")
            if resumeBtn.clicked then
                gameState = GameStates.Running
            end

            local restartBtn = UIButton("Restart")
            if restartBtn.clicked then
                Reset()
            end

            local exitBtn = UIButton("Exit")
            if exitBtn.clicked then
                ExitApp()
            end

            UIEndPanel()
        end

        UIPopStyle(StyleVar.TextColor)
        UIPopStyle(StyleVar.BorderColor)
        UIPopStyle(StyleVar.BackgroundColor)
    end

    tracy.ZoneEnd()
end

function Reset()
    gameState = GameStates.Start
    local phys = player:GetRigidBody2DComponent():GetRigidBody()

    phys:SetPosition(Vec2.new(0.0, 0.0))
    phys:SetForce(Vec2.new(0.0,0.0))
    phys:SetLinearVelocity(Vec2.new(0.0, 0.0))
    phys:SetOrientation(0.0)
    phys:SetAngularVelocity(0.0)
    phys:SetIsStatic(true)

    -- Re-register collision callback after physics reset
    SetCallback(beginContact, phys:GetB2Body())

    -- Reset game state
    furthestPillarPosX = 0.0
    pillarTarget = INITIAL_PILLAR_TARGET
    pillarIndex = 1
    score = 0

    -- Reset difficulty to initial values
    speed = INITIAL_SPEED
    gapSize = INITIAL_GAP_SIZE
    verticalSpeed = INITIAL_VERTICAL_SPEED

    -- Reset camera
    cameraTargetX = 0.0
    cameraBaseX = 0.0
    shakeTimer = 0.0
    shakeOffset = Vec3.new(0, 0, 0)

    -- Reset combo
    comboCount = 0
    comboTimer = 0.0
    lastPillarPassed = -1

    -- Clean up score popups
    for _, popup in ipairs(scorePopups) do
        if popup.entity and popup.entity:Valid() then
            popup.entity:Destroy()
        end
    end
    scorePopups = {}

    -- Clean up countdown entity
    if countdownEntity and countdownEntity:Valid() then
        countdownEntity:Destroy()
        countdownEntity = nil
    end

    player:GetTransform():SetLocalPosition(Vec3.new(0.0,0.0,0.0))
    camera:GetTransform():SetLocalPosition(Vec3.new(0.0, 0.0, 0.0))

    for i = 1, NUM_PILLAR_PAIRS * 2, 2 do
        local p1 = pillars[i]
        local p2 = pillars[i + 1]
        if p1 and p1:Valid() then
            p1:Destroy()
        end
        if p2 and p2:Valid() then
            p2:Destroy()
        end
        CreatePillar(i, (i + 2) * PILLAR_SPACING)
    end

    if gameOverEntity then
        gameOverEntity:Destroy()
        gameOverEntity = nil
    end

    collectgarbage()
end

function OnCleanUp()
    backgroundTexture = nil
    iconTexture = nil
    gameOverTexture = nil

    -- Clean up popups
    for _, popup in ipairs(scorePopups) do
        if popup.entity and popup.entity:Valid() then
            popup.entity:Destroy()
        end
    end
    scorePopups = {}
    parallaxLayers = {}
end

function OnRelease()
    OnCleanUp()
end

