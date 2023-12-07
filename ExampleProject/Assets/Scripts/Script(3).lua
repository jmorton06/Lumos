local DungeonModule = require("Lua-Dungeon-Generator/src/Dungeon")
local LevelModule = require("Lua-Dungeon-Generator/src/Level")
local FuncModule = require("Lua-Dungeon-Generator/MainHelpFunc")

local MAX_HEIGHT = 10.0
local SPEED = 7
local VerticalSpeed = 8
local m_FurthestPillarPosX = 0
local gapSize = 4.0;

local GameStates = {
    Running=0,
    GameOver=1,
	Start
}

local gameState = GameStates.Start
local entityManager = {}

local player = {}
local camera = {}

function CreatePlayer()
    texture = LoadTextureWithParams("icon", "//Assets/Textures/TappyPlane/PNG/Planes/planeBlue1.png", TextureFilter.Linear, TextureWrap.ClampToEdge)

    player = entityManager:Create()
    player:AddSprite(Vector2.new(-0.4, -0.4), Vector2.new(0.8, 0.8), Vector4.new(1.0,1.0,1.0,1.0)):SetTexture(texture)

    local params = RigidBodyParameters.new()
    params.position = Vector3.new( 2.75, 2.75, 1.0)
    params.scale = Vector3.new(0.4, 0.4,0.4)
	params.shape = Shape.Cube
	params.isStatic = false

    player:GetTransform():SetLocalPosition(Vector3.new(3.0,3.0,4.0))
    player:AddRigidBody2DComponent(params)
	--player:GetRigidBody2DComponent():GetRigidBody():SetForce(Vector2.new(200.0,0.0))
    player:GetRigidBody2DComponent():GetRigidBody():SetIsStatic(true)
	player:GetRigidBody2DComponent():GetRigidBody():SetFriction(0.0)
end

function OnInit()
    entityManager = scene:GetEntityManager()

    camera = entityManager:Create()
	camera:AddTransform()

	local screenSize = GetAppInstance():GetWindowSize()
    camera:AddCamera(screenSize.x / screenSize.y, 5.0, 1.0)

	SetB2DGravity(Vector2.new(0.0,0.0))		---18.0))
    CreatePlayer()


  -- Settings for level sizes and number of levels in dungeon.
  height=40
  width=60
  nrOfLevels=5

  dungeon = Dungeon:new(nrOfLevels, height, width)
  
  -- generate with default settings
  dungeon:generateDungeon()
  
  -- generate with advanced settings, 
  -- params: (advanced, maxRooms, maxRoomSize, scatteringFactor)
  -- dungeon:generateDungeon(true, 30, 10, 30)
  
  -- inits a player in level 1, a boss in last level
  initPlayer(dungeon.levels[1])
  initBoss(dungeon.levels[#dungeon.levels])

	SpriteSheetTexture = LoadTextureWithParams("icon", "//Assets/Textures/Dungeon/roguelikeDungeon_transparent.png", TextureFilter.Linear, TextureWrap.ClampToEdge)
	gameOverTexture = LoadTextureWithParams("gameOver", "//Assets/Textures/TappyPlane/PNG/UI/textGameOver.png", TextureFilter.Linear, TextureWrap.ClampToEdge)

	scoreEntity = entityManager:Create()
	scoreEntity:AddTransform()
	scoreEntity:AddTextComponent()
	scoreEntity:GetTextComponent().Colour = Vector4.new(0.4, 0.1, 0.9, 1.0)
    scoreEntity:GetTextComponent().TextString = "Click to start!"
	scoreEntity:GetTransform():SetLocalPosition(Vector3.new(-4.0, 8.0, 0.0))
	scoreEntity:GetTransform():SetLocalScale(Vector3.new(2.0, 2.0, 2.0))

--    gameOverEntity = entityManager:Create()
--    gameOverEntity:AddSprite(Vector2.new(0.0, 0.0), Vector2.new(30, 4), Vector4.new(1.0, 1.0, 1.0, 1.0)):SetTexture(gameOverTexture)
--	gameOverPos = player:GetTransform():GetWorldPosition() + Vector3.new(15.0, 2.0, 1.0)
--	gameOverEntity:GetTransform():SetLocalScale(Vector3.new(0.2, 0.2, 0.2))

local firstLevel = dungeon.levels[1]

    for k=0,firstLevel.height+1 do
      local row
      for j=0,firstLevel.width+1 do
		row=firstLevel.matrix[k][j].class

	  local tile = entityManager:Create()
--    *   " " for empty
--    *   "." for floor
--    *   "#" for wall
--    *   "<" for ascending staircase
--    *   ">" for descending staircase
--    *   "%" for soil
--    *   "*" for mineral vein
--    *   "'" for open door
--    *   "+" for closed door
		if row == "#" then
			tile:AddSprite(Vector2.new(-0.5, -0.5), Vector2.new(1, 1), Vector4.new(1.0, 1.0, 1.0, 1.0)):SetTexture(SpriteSheetTexture)
			tile:GetSprite():SetSpriteSheet(Vector2.new(9,2), Vector2.new(16, 16), Vector2.new(16, 16), 1)

			local params = RigidBodyParameters.new()
		    params.position = Vector3.new( k, j, -3)
		    params.scale = Vector3.new(0.5, 0.5,0.5)
			params.shape = Shape.Cube
			params.isStatic = false
		
		    tile:AddRigidBody2DComponent(params)
		    tile:GetRigidBody2DComponent():GetRigidBody():SetIsStatic(true)
		elseif row == "." then
			tile:AddSprite(Vector2.new(-0.5, -0.5), Vector2.new(1, 1), Vector4.new(1.0, 1.0, 1.0, 1.0)):SetTexture(SpriteSheetTexture)
			tile:GetSprite():SetSpriteSheet(Vector2.new(16,14), Vector2.new(16, 16), Vector2.new(16, 16), 1)
		elseif row == "<" then
			tile:AddSprite(Vector2.new(-0.5, -0.5), Vector2.new(1, 1), Vector4.new(1.0, 1.0, 1.0, 1.0)):SetTexture(SpriteSheetTexture)
			tile:GetSprite():SetSpriteSheet(Vector2.new(16,2), Vector2.new(16, 16), Vector2.new(16, 16), 1)
		elseif row == ">" then
			tile:AddSprite(Vector2.new(-0.5, -0.5), Vector2.new(1, 1), Vector4.new(1.0, 1.0, 1.0, 1.0)):SetTexture(SpriteSheetTexture)
			tile:GetSprite():SetSpriteSheet(Vector2.new(20,2), Vector2.new(16, 16), Vector2.new(16, 16), 1)
		elseif row == "%" then
			tile:AddSprite(Vector2.new(-0.5, -0.5), Vector2.new(1, 1), Vector4.new(1.0, 1.0, 1.0, 1.0)):SetTexture(SpriteSheetTexture)
			tile:GetSprite():SetSpriteSheet(Vector2.new(13,11), Vector2.new(16, 16), Vector2.new(16, 16), 1)
		elseif row == "*" then
			tile:AddSprite(Vector2.new(-0.5, -0.5), Vector2.new(1, 1), Vector4.new(1.0, 1.0, 1.0, 1.0)):SetTexture(SpriteSheetTexture)
			tile:GetSprite():SetSpriteSheet(Vector2.new(10,12), Vector2.new(16, 16), Vector2.new(16, 16), 1)
		elseif row == "'" then
			tile:AddSprite(Vector2.new(-0.5, -0.5), Vector2.new(1, 1), Vector4.new(1.0, 1.0, 1.0, 1.0)):SetTexture(SpriteSheetTexture)
			tile:GetSprite():SetSpriteSheet(Vector2.new(22,4), Vector2.new(16, 16), Vector2.new(16, 16), 1)
		elseif row == "+" then
			tile:AddSprite(Vector2.new(-0.5, -0.5), Vector2.new(1, 1), Vector4.new(1.0, 1.0, 1.0, 1.0)):SetTexture(SpriteSheetTexture)
			tile:GetSprite():SetSpriteSheet(Vector2.new(22,6), Vector2.new(16, 16), Vector2.new(16, 16), 1)
		else
			tile:AddSprite(Vector2.new(-0.5, -0.5), Vector2.new(1, 1), Vector4.new(1.0, 1.0, 1.0, 1.0)):SetTexture(SpriteSheetTexture)
			tile:GetSprite():SetSpriteSheet(Vector2.new(9,7), Vector2.new(16, 16), Vector2.new(16, 16), 1)
		end
		tile:GetTransform():SetLocalPosition(Vector3.new(k, j, -3))
		tile:GetTransform():SetLocalScale(Vector3.new(1, 1, 1))
      end
  end

end


function PlayerJump()

	local phys = player:GetRigidBody2DComponent()
	local vel = phys:GetRigidBody():GetLinearVelocity()

	vel.y = VerticalSpeed
	vel.x = SPEED

    --phys:GetRigidBody():SetLinearVelocity(vel)
end

    local dx = 0
    local dy = 0
	local speed = 10

function OnUpdate(dt)

    phys = player:GetRigidBody2DComponent()

    up = Vector3.new(0, 1, 0)
    right = Vector3.new(1, 0, 0)
	local pos = phys:GetRigidBody():GetPosition()
	local vel = phys:GetRigidBody():GetLinearVelocity()
    if Input.GetKeyHeld( Key.Space ) or Input.GetMouseClicked(MouseButton.Left) then
        PlayerJump()
	elseif Input.GetKeyHeld( Key.W ) then
		dy= dy + 1
    	--phys:GetRigidBody():SetLinearVelocity(vel)
	elseif Input.GetKeyHeld( Key.S ) then
		dy = dy - 1
    	--phys:GetRigidBody():SetLinearVelocity(vel)
	elseif Input.GetKeyHeld( Key.A ) then
		dx = dx - 1
   	 --phys:GetRigidBody():SetLinearVelocity(vel)
	elseif Input.GetKeyHeld( Key.D ) then
		dx = dx + 1
    	--phys:GetRigidBody():SetLinearVelocity(vel)d
    end

	phys:GetRigidBody():SetLinearVelocity(Vector2.new(vel.x + speed * dx * dt, vel.y + speed * dy * dt))
	phys:GetRigidBody():SetOrientation(0.0)

	local cameraPos = Vector3.new(pos.x, pos.y, -4.0)
    camera:GetTransform():SetLocalPosition(cameraPos)

	dx = dx - speed * 0.8
	if dx < 0 then
		dx = 0
	end

	dy = dy - speed * 0.8
	
	if dy < 0 then
		dy = 0
	end

	if gameState == GameStates.Start then

       scoreEntity:GetTextComponent().TextString = "Click to start"
	   scoreEntity:GetTransform():SetLocalPosition(Vector3.new(-4.0, 8.0, 0.0))

 	   pos = player:GetTransform():GetWorldPosition()
       pos.y = 0.0
 	   camera:GetTransform():SetLocalPosition(pos)
	 	if Input.GetKeyPressed( Key.Space ) or Input.GetMouseClicked(MouseButton.Left) then
			gameState = GameStates.Running
			player:GetRigidBody2DComponent():GetRigidBody():SetIsStatic(false)
			--player:GetRigidBody2DComponent():GetRigidBody():SetForce(Vector2.new(200.0,0.0))
			PlayerJump()
     	end
	end
end

function OnCleanUp()
end


-- Example of generating a dungeon with: 
--  * default settings by using generateDungeon.
--  * "Advanced" settings with max nr of rooms, room size and scattering factor.

function main()



end

main()

-- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- 

-- Example of generating one level with customized settings.

-- Some targeted functions to play with:
--  * setMaxRooms: maximum nr of rooms.
--  * setMaxRoomSize: keep it lower than height/width-3 though!
--  * setScatteringFactor: increases random scattering of tiles when forming corridors.
--  * addCycles: adds random cycles between rooms up to parameter value.

function mainCustomizedLevel()
  height = 40
  width = 60
  level = Level:new(height, width)
  level:setMaxRooms(30)
  level:setMaxRoomSize(5)
  level:setScatteringFactor(10)
  Level.veinSpawnRate=0.4
  level:initMap(level.height, level.width)
  level:generateRooms()
  root=level:getRoomTree()
  level:buildCorridors(root)
  level:buildCorridor(root, level:getEnd())
  level:addCycles(10)
  level:addStaircases(10)
  level:addDoors()
  initBoss(level)
  initPlayer(level)
  level:printLevel()
end
  
--mainCustomizedLevel()








































