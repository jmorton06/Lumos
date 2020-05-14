registry = scene:GetRegistry()

pillars = {}

MAX_HEIGHT = 10.0
SPEED = 3.0
m_FurthestPillarPosX = 0

m_PillarTarget = 35.0
m_PillarIndex = 1

PLAYERTYPEID = 1
PILLARTYPEID = 2

local GameStates = {
    Running=0,
    GameOver=1
}

gameState = GameStates.Running

function beginContact(a, b)
    gameState = GameStates.GameOver
end

function endContact(a, b, coll)
 
end
 
function preSolve(a, b, coll)
 
end
 
function postSolve(a, b, coll, normalimpulse, tangentimpulse)
 
end

player = {}
score = 0

function CreatePlayer()
    colour = Vector4.new(1.0,1.0,1.0,1.0)

    texture = LoadTextureWithParams("icon", "/CoreTextures/panda.png", TextureFilter.Linear, TextureWrap.Clamp)

    player = registry:Create()
    registry:assign_Sprite(player, Vector2.new(-1.0/2.0, -1.0/2.0), Vector2.new(1.15, 1.0), colour)
    registry:get_Sprite(player):SetTexture(texture)

    params = PhysicsObjectParamaters.new()
    params.position = Vector3.new( 1.0, 1.0, 1.0)
    params.scale = Vector3.new(1.0 / 2.0, 1.0 / 2.0, 1.0)
	params.shape = Shape.Circle
	params.isStatic = false
    blockPhysics = CreatePhysics2DSharedWithParams(params)
    blockPhysics:GetB2Body():SetLinearVelocity(b2Vec2(SPEED,0.0))
    --blockPhysics:GetB2Body():SetUserData(1.0)

    SetCallback(beginContact)

    registry:assign_Physics2DComponent(player, blockPhysics)
    registry:assign_Transform(player)
    tran = registry:get_Transform(player)
    tran:SetLocalPosition(Vector3.new(1.0,1.0,1.0))
end

function CreatePillar(index, offset)
	pillars[index] = registry:Create();
	pillars[index + 1] = registry:Create();

	colour = Vector4.new(Rand(0.0, 1.0), Rand(0.0, 1.0), Rand(0.0, 1.0), 1.0);

	gapSize = 4.0;
    centre = Rand(-6.0, 6.0);
    
    --Top Pillar
	topY = MAX_HEIGHT;
	bottomY = centre + (gapSize / 2.0);

	pos = Vector3.new(offset / 2.0, ((topY - bottomY )/ 2.0) + bottomY, 0.0);
	scale = Vector3.new(1.0, (topY - bottomY) / 2.0, 1.0);

    texture = LoadTextureWithParams("icon", "/CoreTextures/icon.png", TextureFilter.Linear, TextureWrap.Clamp)

    registry:assign_Sprite(pillars[index], Vector2.new(-scale.x, -scale.y), Vector2.new(scale.x * 2.0, scale.y * 2.0), colour)

    registry:get_Sprite(pillars[index]):SetTexture(texture)

    params = PhysicsObjectParamaters.new()
	params.position = pos
	params.scale = scale
	params.shape = Shape.Square
	params.isStatic = true
    blockPhysics = CreatePhysics2DSharedWithParams(params)
   -- blockPhysics:GetB2Body():SetUserData(2.0)

    registry:assign_Physics2DComponent(pillars[index], blockPhysics)
    registry:assign_Transform(pillars[index])

    tran = registry:get_Transform(pillars[index])
    tran:SetLocalPosition(pos)

	--Bottom Pillar
	topY = centre - (gapSize / 2.0)
	bottomY = -MAX_HEIGHT;

	pos = Vector3.new(offset / 2.0, ((topY - bottomY) / 2.0) + bottomY, 0.0)
	scale = Vector3.new(1.0, (topY - bottomY) / 2.0, 1.0)

    registry:assign_Sprite(pillars[index + 1], Vector2.new(-scale.x, -scale.y), Vector2.new(scale.x * 2.0, scale.y * 2.0), colour)

	params.position = pos
	params.scale = scale
	params.shape = Shape.Square
	params.isStatic = true
    
    blockPhysics2 = CreatePhysics2DSharedWithParams(params)
    --blockPhysics2:GetB2Body():SetUserData(2.0)
    registry:assign_Physics2DComponent(pillars[index + 1], blockPhysics2)
    registry:assign_Transform(pillars[index + 1])
    tran = registry:get_Transform(pillars[index + 1])
    tran:SetLocalPosition(pos)
    
	if pos.x > m_FurthestPillarPosX then
        m_FurthestPillarPosX = pos.x;
    end

end

CreatePlayer()

for i=1,10, 2 do
    CreatePillar(i, (i + 2) * 10.0)
end

backgroundTexture = LoadTextureWithParams("grass", "/CoreTextures/backgroundColorGrass.png", TextureFilter.Linear, TextureWrap.Clamp)

backgrounds = {}

function CreateBackground(index)
    backgrounds[index] = registry:Create()
    registry:assign_Sprite(backgrounds[index], Vector2.new(-10.0, -10.0), Vector2.new(10.0 * 2.0, 10.0 * 2.0), Vector4.new(1.0,1.0,1.0,1.0))
    registry:get_Sprite(backgrounds[index]):SetTexture(backgroundTexture)
    registry:assign_Transform(backgrounds[index])
    registry:get_Transform(backgrounds[index]):SetLocalPosition(Vector3.new(index * 20.0 - 40.0, 0.0,0.0))
end

for i=1,50, 1 do
    CreateBackground(i)
end

function OnUpdate(dt)
    if gameState == GameStates.Running then
		phys = registry:get_Physics2DComponent(player)

        up = Vector3.new(0, 1, 0)
        right = Vector3.new(1, 0, 0)

	 	cameraSpeed = 1.0
	 	velocity = Vector3.new(0.0, 0.0, 0.0)

        if Input.GetKeyPressed( Key.Space ) then
            velocity = velocity + up * cameraSpeed * 400.0
        end

        phys:GetPhysicsObject():GetB2Body():ApplyForce(b2Vec2(velocity.x,velocity.y), phys:GetPhysicsObject():GetB2Body():GetPosition(), true)

        pos = registry:get_Transform(player):GetWorldPosition()

		if pos.y > MAX_HEIGHT or pos.y < -MAX_HEIGHT then
            gameState = GameStates.GameOver
        end

        pos.y = 0.0

        scene:GetCamera():SetPosition(pos)

        score = math.floor((registry:get_Transform(player):GetWorldPosition().x - 5) / 10)

        if registry:get_Transform(player):GetWorldPosition().x > m_PillarTarget then
            
 			if pillars[m_PillarIndex] and registry:Valid(pillars[m_PillarIndex]) then
				registry:Destroy(pillars[m_PillarIndex])
				registry:Destroy(pillars[m_PillarIndex + 1])
            end

 			CreatePillar(m_PillarIndex, m_FurthestPillarPosX * 2.0 + 20.0)
			m_PillarIndex = m_PillarIndex + 2
		    m_PillarIndex = math.modf(m_PillarIndex, 10)
			m_PillarTarget = m_PillarTarget + 10.0
        end 

        if gameState == GameStates.Running then
            gui.beginWindow("Running", gui.WindowFlags.NoDecoration)
            gui.text("Score : ")
            gui.sameLine()
            gui.text(tostring(score))
            gui.endWindow()
        end
    elseif gameState == GameStates.GameOver then
        gui.beginWindow("GameOver")
        gui.text("GameOver")
        gui.text("Score : ")
        gui.sameLine()
        gui.text(tostring(score))

        if gui.button("Reset") then
            Reset();
        end
        gui.endWindow()
    end
end

function Reset()
    gameState = GameStates.Running
    phys = registry:get_Physics2DComponent(player):GetPhysicsObject()

	phys:SetPosition(Vector2.new(0.0, 0.0))
	phys:SetLinearVelocity(Vector2.new(SPEED, 0.0))
	phys:SetOrientation(0.0)
	phys:SetAngularVelocity(0.0)

	m_FurthestPillarPosX = 0.0
	m_PillarTarget = 35.0
    m_PillarIndex = 1

	registry:get_Transform(player):SetLocalPosition(Vector3.new(0.0,0.0,0.0))
	--registry:get_Transform(player):UpdateMatrices()
    
    for i=1,10, 2 do
        if registry:Valid(pillars[i]) then
            registry:Destroy(pillars[i]);
            registry:Destroy(pillars[i + 1]);
        end

        CreatePillar(i, (i + 2) * 10.0)
    end
end

function OnCleanUp()
    backgroundTexture = nil
    texture = nil
    blockPhysics = nil
    blockPhysics2 = nill

    registry:Destroy(player)
end

