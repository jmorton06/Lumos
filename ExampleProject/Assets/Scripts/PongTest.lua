entityManager = nil

leftPaddle = nil
rightPaddle = nil
ball = nil
camera = nil
scoreEntity = nil
leftScore = 0
rightScore = 0

paddleSpeed = 12.0
ballSpeed = 10.0

arenaHalfWidth = 16.0
arenaHalfHeight = 9.0

function CreatePaddle(isLeft)
    local e = entityManager:Create()
    e:AddTransform()
    local w = 0.5
    local h = 3.0
    e:AddSprite(Vec2.new(-w, -h), Vec2.new(w * 2.0, h * 2.0), Vec4.new(1.0,1.0,1.0,1.0))

    local params = RigidBodyParameters.new()
    params.position = Vec3.new((isLeft and -arenaHalfWidth + 1.0) or (arenaHalfWidth - 1.0), 0.0, 1.0)
    params.scale = Vec3.new(w, h, 1.0)
    params.shape = Shape.Square
    params.isStatic = true

    e:GetTransform():SetLocalPosition(params.position)
    e:AddRigidBody2DComponent(params)

    return e
end

function BallContactCallback(a, b, approachSpeed)
    local body = ball:GetRigidBody2DComponent():GetRigidBody()
    body:SetLinearVelocity( -1.0 * body:GetLinearVelocity())
end

function CreateBall()
    local e = entityManager:Create()
    e:AddTransform()
    local r = 0.3
    e:AddSprite(Vec2.new(-r, -r), Vec2.new(r * 2.0, r * 2.0), Vec4.new(1.0,1.0,1.0,1.0))

    local params = RigidBodyParameters.new()
    params.position = Vec3.new(0.0, 0.0, 1.0)
    params.scale = Vec3.new(r, r, 1.0)
    params.shape = Shape.Circle
    params.isStatic = false
    params.damping = 0.0
    params.friction = 0.0
    params.elasticity = 1.0
    e:GetTransform():SetLocalPosition(params.position)
    e:AddRigidBody2DComponent(params):GetRigidBody():SetLinearDamping(0.0)
    SetCallback(BallContactCallback, e:GetRigidBody2DComponent():GetRigidBody():GetB2Body())
    return e
end

function CreateBorder()
    local thickness = 0.5
    local colour = Vec4.new(1.0, 1.0, 1.0, 1.0)

    local top = entityManager:Create()
    top:AddTransform()
    top:AddSprite(Vec2.new(-arenaHalfWidth, -thickness * 0.5), Vec2.new(arenaHalfWidth * 2.0, thickness), colour)
    local topParams = RigidBodyParameters.new()
    topParams.position = Vec3.new(0.0, arenaHalfHeight + thickness * 0.5, 1.0)
    topParams.scale = Vec3.new(arenaHalfWidth * 2.0, thickness, 1.0)
    topParams.shape = Shape.Square
    topParams.isStatic = true
    top:GetTransform():SetLocalPosition(topParams.position)
    top:AddRigidBody2DComponent(topParams)

    local bottom = entityManager:Create()
    bottom:AddTransform()
    bottom:AddSprite(Vec2.new(-arenaHalfWidth, -thickness * 0.5), Vec2.new(arenaHalfWidth * 2.0, thickness), colour)
    local bottomParams = RigidBodyParameters.new()
    bottomParams.position = Vec3.new(0.0, -arenaHalfHeight - thickness * 0.5, 1.0)
    bottomParams.scale = Vec3.new(arenaHalfWidth * 2.0, thickness, 1.0)
    bottomParams.shape = Shape.Square
    bottomParams.isStatic = true
    bottom:GetTransform():SetLocalPosition(bottomParams.position)
    bottom:AddRigidBody2DComponent(bottomParams)

    local left = entityManager:Create()
    left:AddTransform()
    left:AddSprite(Vec2.new(-thickness * 0.5, -arenaHalfHeight), Vec2.new(thickness, arenaHalfHeight * 2.0 + thickness), colour)
    left:GetTransform():SetLocalPosition(Vec3.new(-arenaHalfWidth - thickness * 0.5, 0.0, 1.0))

    local right = entityManager:Create()
    right:AddTransform()
    right:AddSprite(Vec2.new(-thickness * 0.5, -arenaHalfHeight), Vec2.new(thickness, arenaHalfHeight * 2.0 + thickness), colour)
    right:GetTransform():SetLocalPosition(Vec3.new(arenaHalfWidth + thickness * 0.5, 0.0, 1.0))
end

function Serve(direction)
    if not ball then return end
    local phys = ball:GetRigidBody2DComponent():GetRigidBody()
    phys:SetIsStatic(false)
    local vx = ballSpeed * (direction or (math.random() > 0.5 and 1 or -1))
    local vy = (math.random() - 0.5) * 6.0
    phys:SetLinearVelocity(Vec2.new(vx, vy))
end

function ResetBall(toRight)
    if not ball then return end
    local phys = ball:GetRigidBody2DComponent():GetRigidBody()
    phys:SetPosition(Vec2.new(0.0, 0.0))
    phys:SetLinearVelocity(Vec2.new(0.0, 0.0))
    phys:SetOrientation(0.0)
    phys:SetAngularVelocity(0.0)
    phys:SetIsStatic(true)
    -- small delay before serving; serve immediately here
    Serve(toRight and 1 or -1)
end

function OnInit()
    entityManager = scene:GetEntityManager()

    SetB2DGravity(Vec2.new(0.0, 0.0))

    camera = entityManager:Create()
    camera:AddTransform()
    local screenSize = GetAppInstance():GetWindowSize()
    camera:AddCamera(screenSize.x / screenSize.y, 20.0, 1.0)

    scoreEntity = entityManager:Create()
    scoreEntity:AddTransform()
    scoreEntity:AddTextComponent()
    scoreEntity:GetTextComponent().Colour = Vec4.new(1.0,1.0,1.0,1.0)
    scoreEntity:GetTransform():SetLocalPosition(Vec3.new(0.0, 8.0, 0.0))
    scoreEntity:GetTransform():SetLocalScale(Vec3.new(1.5,1.5,1.5))

    -- create paddles and ball
    leftPaddle = CreatePaddle(true)
    rightPaddle = CreatePaddle(false)
    ball = CreateBall()

    -- create visible/playable borders
    CreateBorder()

    leftScore = 0
    rightScore = 0
    scoreEntity:GetTextComponent().TextString = tostring(leftScore) .. " - " .. tostring(rightScore)

    -- random seed
    math.randomseed(os.time())
end

function UpdateScore()
    if scoreEntity then
        scoreEntity:GetTextComponent().TextString = tostring(leftScore) .. " - " .. tostring(rightScore)
    end
end

function OnUpdate(dt)
    tracy.ZoneBegin()

    if Input.GetKeyHeld(Key.W) then
        local p = leftPaddle:GetTransform():LocalPosition()
        p.y = math.min(arenaHalfHeight - 1.5, p.y + paddleSpeed * dt)
        leftPaddle:GetTransform():SetLocalPosition(p)
        local rb = leftPaddle:GetRigidBody2DComponent():GetRigidBody()
        rb:SetPosition(Vec2.new(p.x, p.y))
    end
    if Input.GetKeyHeld(Key.S) then
        local p = leftPaddle:GetTransform():LocalPosition()
        p.y = math.max(-arenaHalfHeight + 1.5, p.y - paddleSpeed * dt)
        leftPaddle:GetTransform():SetLocalPosition(p)
        local rb = leftPaddle:GetRigidBody2DComponent():GetRigidBody()
        rb:SetPosition(Vec2.new(p.x, p.y))
    end

    if Input.GetKeyHeld(Key.Up) then
        local p = rightPaddle:GetTransform():LocalPosition()
        p.y = math.min(arenaHalfHeight - 1.5, p.y + paddleSpeed * dt)
        rightPaddle:GetTransform():SetLocalPosition(p)
        local rb = rightPaddle:GetRigidBody2DComponent():GetRigidBody()
        rb:SetPosition(Vec2.new(p.x, p.y))
    end
    if Input.GetKeyHeld(Key.Down) then
        local p = rightPaddle:GetTransform():LocalPosition()
        p.y = math.max(-arenaHalfHeight + 1.5, p.y - paddleSpeed * dt)
        rightPaddle:GetTransform():SetLocalPosition(p)
        local rb = rightPaddle:GetRigidBody2DComponent():GetRigidBody()
        rb:SetPosition(Vec2.new(p.x, p.y))
    end

    if Input.GetKeyPressed(Key.Space) then
        ResetBall(true)
    end

    if ball then
        local pos = ball:GetTransform():GetWorldPosition()
        if pos.y > arenaHalfHeight or pos.y < -arenaHalfHeight then
            local phys = ball:GetRigidBody2DComponent():GetRigidBody()
            local vel = phys:GetLinearVelocity()
            vel.y = -vel.y
            phys:SetLinearVelocity(vel)
        end

        if pos.x > arenaHalfWidth then
            leftScore = leftScore + 1
            UpdateScore()
            ResetBall(-1)
        elseif pos.x < -arenaHalfWidth then
            rightScore = rightScore + 1
            UpdateScore()
            ResetBall(1)
        end
    end

    if camera and ball then
        local bpos = ball:GetTransform():GetWorldPosition()
        bpos.y = 0.0
        camera:GetTransform():SetLocalPosition(Vec3.new(0.0, 0.0, 0.0))
    end

    tracy.ZoneEnd()
end

function Reset()
    leftPaddle:GetTransform():SetLocalPosition(Vec3.new(-arenaHalfWidth + 1.0, 0.0, 1.0))
    leftPaddle:GetRigidBody2DComponent():GetRigidBody():SetPosition(Vec2.new(-arenaHalfWidth + 1.0, 0.0))
    rightPaddle:GetTransform():SetLocalPosition(Vec3.new(arenaHalfWidth - 1.0, 0.0, 1.0))
    rightPaddle:GetRigidBody2DComponent():GetRigidBody():SetPosition(Vec2.new(arenaHalfWidth - 1.0, 0.0))
    if ball then
        local phys = ball:GetRigidBody2DComponent():GetRigidBody()
        phys:SetPosition(Vec2.new(0.0, 0.0))
        phys:SetLinearVelocity(Vec2.new(0.0, 0.0))
        phys:SetIsStatic(true)
    end
    leftScore = 0
    rightScore = 0
    UpdateScore()
end

function OnCleanUp()
    leftPaddle = nil
    rightPaddle = nil
    ball = nil
end

function OnRelease()
    OnCleanUp()
end
