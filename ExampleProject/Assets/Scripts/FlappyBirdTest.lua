pillars = {}

maxHeight = 10.0
speed = 7
verticalSpeed = 8
furthestPillarPosX = 0
gapSize = 4.0

pillarTarget = 35.0
pillarIndex = 1

playerTypeId = 1
pillarTypeId = 2

GameStates = {
    Running = 0,
    GameOver = 1,
    Start = 2
}

gameState = GameStates.Start
entityManager = nil

player = nil
camera = nil
scoreEntity = nil
score = 0
iconTexture = nil
gameOverTexture = nil
gameOverEntity = nil
gameOverScale = 1.0
totalTime = 0.0
gameOverSize = Vector2.new(30, 4)
showImguiWindow = false

function EndGame()
    gameState = GameStates.GameOver

    if gameOverEntity == nil then
        gameOverEntity = entityManager:Create()
        gameOverEntity:AddSprite(Vector2.new(0.0, 0.0), gameOverSize, Vector4.new(1.0, 1.0, 1.0, 1.0)):SetTexture(gameOverTexture)
        gameOverEntity:GetTransform():SetLocalScale(Vector3.new(0.2, 0.2, 0.2))
    end
    if player then
        local gameOverPos = player:GetTransform():GetWorldPosition() + Vector3.new(15.0, 2.0, 1.0)
        gameOverEntity:GetTransform():SetLocalPosition(gameOverPos)
    end
end

function beginContact(a, b, approachSpeed)
    EndGame()
end

function CreatePlayer()
    local texture = LoadTextureWithParams("icon", "//Assets/Textures/TappyPlane/PNG/Planes/planeBlue1.png", TextureFilter.Linear, TextureWrap.ClampToEdge)

    player = entityManager:Create()
    player:AddSprite(Vector2.new(-0.9, -0.8), Vector2.new(1.7, 1.5), Vector4.new(1.0,1.0,1.0,1.0)):SetTexture(texture)

    local params = RigidBodyParameters.new()
    params.position = Vector3.new(1.0, 1.0, 1.0)
    params.scale = Vector3.new(1.7 / 2.0, 1.5 / 2.0, 1.0)
    params.shape = Shape.Circle
    params.isStatic = false

    player:GetTransform():SetLocalPosition(Vector3.new(1.0,1.0,1.0))
    player:AddRigidBody2DComponent(params):GetRigidBody():SetForce(Vector2.new(0.0,0.0))
    player:GetRigidBody2DComponent():GetRigidBody():SetIsStatic(true)
    player:GetRigidBody2DComponent():GetRigidBody():SetLinearDamping(0.1)
    local emitter = player:AddParticleEmitter()
    emitter:SetParticleCount(512)
    emitter:SetParticleRate(0.1)
    emitter:SetNumLaunchParticles(100)
    emitter:SetParticleLife(0.9)
    emitter:SetParticleSize(0.2)
    emitter:SetGravity(Vector3.new(0.0, -1.0, 0.0))
    emitter:SetSpread(Vector3.new(1.0, 0.4, 0.0))
    emitter:SetVelocitySpread(Vector3.new(0.0, 0.8, 0.0))
    emitter:SetInitialVelocity(Vector3.new(-3.0, 0.0, 0.0))
	emitter:SetInitialColour(Vector4.new(0.8,0.8,0.8,0.6))
	emitter:SetFadeOut(0.9)
	emitter:SetFadeIn(2.0)
    SetCallback(beginContact, player:GetRigidBody2DComponent():GetRigidBody():GetB2Body())
end

function CreatePillar(index, offset)

    pillars[index] = entityManager:Create()
    pillars[index + 1] = entityManager:Create()

    local centre = Rand(-6.0, 6.0);
    local width = Rand(1.0, 2.0)
    
    local topY = maxHeight;
	local bottomY = centre + (gapSize / 2.0);

	local pos = Vector3.new(offset / 2.0, ((topY - bottomY )/ 2.0) + bottomY, 0.0);
	local scale = Vector3.new(width, (topY - bottomY) / 2.0, 1.0);

    pillars[index]:AddSprite(Vector2.new(-scale.x, -scale.y), Vector2.new(scale.x * 2.0, scale.y * 2.0), Vector4.new(1.0,1.0,1.0,1.0)):SetTexture(iconTexture)

    local params = RigidBodyParameters.new()
	params.position = pos
	params.scale = scale
	params.shape = Shape.Custom
	params.isStatic = true
	params.customShapePositions = { Vector2.new(-scale.x, -scale.y), Vector2.new(scale.x * 0.25, scale.y * 1.05), Vector2.new(scale.x, -scale.y)}

    pillars[index]:GetTransform():SetLocalPosition(pos)
    pillars[index]:AddRigidBody2DComponent(params):GetRigidBody():SetOrientation(3.14)

    topY = centre - (gapSize / 2.0)
    bottomY = -maxHeight;
    width = Rand(1.0, 2.0)
	pos = Vector3.new(offset / 2.0, ((topY - bottomY) / 2.0) + bottomY, 0.0)
	scale = Vector3.new(width, (topY - bottomY) / 2.0, 1.0)

    pillars[index + 1]:AddSprite(Vector2.new(-scale.x, -scale.y), Vector2.new(scale.x * 2.0, scale.y * 2.0), Vector4.new(1.0,1.0,1.0,1.0)):SetTexture(iconTexture)

	params.position = pos
	params.scale = scale
	params.shape = Shape.Custom
	params.isStatic = true
	params.customShapePositions = { Vector2.new(-scale.x, -scale.y), Vector2.new(scale.x * 0.25, scale.y * 1.05), Vector2.new(scale.x, -scale.y)}

    pillars[index + 1]:GetTransform():SetLocalPosition(pos)
    pillars[index + 1]:AddRigidBody2DComponent(params)

    if pos.x > furthestPillarPosX then
        furthestPillarPosX = pos.x;
    end
end

function CreateBackground(index)
    backgrounds[index] = entityManager:Create()
    backgrounds[index]:AddTransform():SetLocalPosition(Vector3.new(index * 20.0 - 40.0, 0.0, -9.0))
    backgrounds[index]:AddSprite(Vector2.new(-10.0, -10.0), Vector2.new(10.0 * 2.0, 10.0 * 2.0), Vector4.new(1.0,1.0,1.0,1.0)):SetTexture(backgroundTexture)
end

function updateSpeed(v)
    speed = v
end

function updateGap(v)
    gapSize = v
end

backgrounds = {}

function OnInit()
    iconTexture = LoadTextureWithParams("icon", "//Assets/Textures/TappyPlane/PNG/rock.png", TextureFilter.Linear, TextureWrap.ClampToEdge)
    gameOverTexture = LoadTextureWithParams("gameOver", "//Assets/Textures/TappyPlane/PNG/UI/textGameOver.png", TextureFilter.Linear, TextureWrap.ClampToEdge)

    entityManager = scene:GetEntityManager()

    SetB2DGravity(Vector2.new(0.0, -9.81 * 2.0))

    camera = entityManager:Create()
    camera:AddTransform()

    scoreEntity = entityManager:Create()
    scoreEntity:AddTransform()
    scoreEntity:AddTextComponent()
    scoreEntity:GetTextComponent().Colour = Vector4.new(0.4, 0.1, 0.9, 1.0)
    scoreEntity:GetTextComponent().TextString = ""
    scoreEntity:GetTransform():SetLocalPosition(Vector3.new(-4.0, 8.0, 0.0))
    scoreEntity:GetTransform():SetLocalScale(Vector3.new(2.0, 2.0, 2.0))

    local screenSize = GetAppInstance():GetWindowSize()
    camera:AddCamera(screenSize.x / screenSize.y, 10.0, 1.0)

    CreatePlayer()

    for i = 1, 10, 2 do
        CreatePillar(i, (i + 2) * 10.0)
    end

    backgroundTexture = LoadTextureWithParams("background", "//Assets/Textures/TappyPlane/PNG/background.png", TextureFilter.Linear, TextureWrap.ClampToEdge)
    for i = 1, 50, 1 do
        CreateBackground(i)
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

    if Input.GetKeyPressed(Key.M) then
        SwitchScene()
    end

    if Input.GetKeyPressed(Key.K) then
        speed = math.max(0, speed - 1)
        updateSpeed(speed)
    end
    if Input.GetKeyPressed(Key.L) then
        speed = speed + 1
        updateSpeed(speed)
    end
    if Input.GetKeyPressed(Key.O) then
        gapSize = math.max(1.0, gapSize - 0.5)
        updateGap(gapSize)
    end
    if Input.GetKeyPressed(Key.P) then
        gapSize = gapSize + 0.5
        updateGap(gapSize)
    end
    if Input.GetKeyPressed(Key.R) then
        Reset()
    end

    if gameState == GameStates.Running then
        local phys = player:GetRigidBody2DComponent()

        if Input.GetKeyHeld(Key.Space) or Input.GetMouseClicked(MouseButton.Left) then
            PlayerJump()
        end

        local pos = player:GetTransform():GetWorldPosition()

        if pos.y > maxHeight or pos.y < -maxHeight then
            EndGame()
        end

        pos.y = 0.0
        camera:GetTransform():SetLocalPosition(pos)

        score = math.max(math.floor((pos.x - 5) / 10), 0)
        scoreEntity:GetTransform():SetLocalPosition(pos + Vector3.new(0.0, 8.0, 0.0))
        scoreEntity:GetTextComponent().TextString = "Score: " .. tostring(score) 

        if pos.x > pillarTarget then
            if pillars[pillarIndex] and pillars[pillarIndex]:Valid() then
                pillars[pillarIndex]:Destroy()
                pillars[pillarIndex + 1]:Destroy()
            else
                Log.Info("Not valid Pillar")
                Log.Info(tostring(pillarIndex))
            end

            CreatePillar(pillarIndex, furthestPillarPosX * 2.0 + 20.0)

            pillarIndex = pillarIndex + 2
            if pillarIndex > 10 then
                pillarIndex = pillarIndex - 10
            end

            pillarTarget = pillarTarget + 10.0
        end

    elseif gameState == GameStates.GameOver then

        totalTime = totalTime + dt * 2
        gameOverScale = 0.2 + (math.sin(totalTime) + 1.0) / 10.0
        if gameOverEntity then
            gameOverEntity:GetTransform():SetLocalScale(Vector3.new(gameOverScale, gameOverScale, gameOverScale))
            gameOverEntity:GetTransform():SetLocalPosition(camera:GetTransform():GetWorldPosition() - Vector3.new((gameOverScale *  gameOverSize.x )/ 2, (gameOverScale *  gameOverSize.y )/ 2, -2.0))
        end

    elseif gameState == GameStates.Start then

       --scoreEntity:GetTextComponent().TextString = "Click to start"
       scoreEntity:GetTransform():SetLocalPosition(Vector3.new(-4.0, 8.0, 0.0))

       pos = player:GetTransform():GetWorldPosition()
       pos.y = 0.0
       camera:GetTransform():SetLocalPosition(pos)
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
        UIPushStyle(StyleVar.BackgroundColor, Vector4.new(0.08, 0.08, 0.08, 0.7))
        UIPushStyle(StyleVar.BorderColor, Vector4.new(0.4, 0.4, 0.4, 0.8))
        UIPushStyle(StyleVar.TextColor, Vector4.new(1.0, 1.0, 1.0, 1.0))

        if gameState == GameStates.Start or gameState == GameStates.GameOver then
            UIBeginPanel("FlappyMenu", WidgetFlags.StackVertically | WidgetFlags.CentreX | WidgetFlags.CentreY)

            UILabel("Title", "Flappy Test")

            local startBtn = UIButton("Start")
            if startBtn.clicked then
                if gameState == GameStates.Start then
                    gameState = GameStates.Running
                    player:GetRigidBody2DComponent():GetRigidBody():SetIsStatic(false)
                    --player:GetRigidBody2DComponent():GetRigidBody():SetForce(Vector2.new(200.0,0.0))
                    PlayerJump()
                else
                    Reset()
                    gameState = GameStates.Running
                    player:GetRigidBody2DComponent():GetRigidBody():SetIsStatic(false)
                   -- player:GetRigidBody2DComponent():GetRigidBody():SetForce(Vector2.new(200.0,0.0))
                    PlayerJump()
                end
            end

            local resetBtn = UIButton("Reset")
            if resetBtn.clicked then
                Reset()
            end

            showImguiWindow = UIToggle("Show ImGui Window", showImguiWindow);

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
    phys = player:GetRigidBody2DComponent():GetRigidBody()

    phys:SetPosition(Vector2.new(0.0, 0.0))
    phys:SetForce(Vector2.new(0.0,0.0))

    phys:SetLinearVelocity(Vector2.new(0.0, 0.0))
	phys:SetOrientation(0.0)
	phys:SetAngularVelocity(0.0)
	phys:SetIsStatic(true)

    furthestPillarPosX = 0.0
    pillarTarget = 35.0
    pillarIndex = 1

	player:GetTransform():SetLocalPosition(Vector3.new(0.0,0.0,0.0))

    for i=1,10, 2 do
        if pillars[i] and pillars[i]:Valid() then
            pillars[i]:Destroy();
            pillars[i + 1]:Destroy();

        	else
        	    Log.Info("Not valid Pillar")
        	    Log.Info(tostring(pillarIndex))
        end

        CreatePillar(i, (i + 2) * 10.0)
    end

    if gameOverEntity then
        gameOverEntity:Destroy()
        gameOverEntity = nil
    end

    collectgarbage()
end

function OnCleanUp()
    backgroundTexture = nil
    texture = nil
    blockPhysics = nil
    blockPhysics2 = nil
	iconTexture = nil
	gameOverTexture = nil
end

function OnRelease()
    OnCleanUp()
end


































































