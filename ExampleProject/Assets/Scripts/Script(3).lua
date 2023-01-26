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
    texture = LoadTextureWithParams("icon", "//Textures/TappyPlane/PNG/Planes/planeBlue1.png", TextureFilter.Linear, TextureWrap.ClampToEdge)

    player = entityManager:Create()
    player:AddSprite(Vector2.new(-0.9, -0.8), Vector2.new(1.7, 1.5), Vector4.new(1.0,1.0,1.0,1.0)):SetTexture(texture)

    local params = RigidBodyParameters.new()
    params.position = Vector3.new( 1.0, 1.0, 1.0)
    params.scale = Vector3.new(1.7 / 2.0, 1.5 / 2.0, 1.0)
	params.shape = Shape.Circle
	params.isStatic = false

    player:GetTransform():SetLocalPosition(Vector3.new(1.0,1.0,1.0))
    player:AddRigidBody2DComponent(params):GetRigidBody():SetForce(Vector2.new(200.0,0.0))
    player:GetRigidBody2DComponent():GetRigidBody():SetIsStatic(true)
end

function OnInit()
    entityManager = scene:GetEntityManager()

    camera = entityManager:Create()
	camera:AddTransform()

	local screenSize = GetAppInstance():GetWindowSize()
    camera:AddCamera(screenSize.x / screenSize.y, 10.0, 1.0)

	SetB2DGravity(Vector2.new(0.0, -18.0))
    CreatePlayer()
end


function PlayerJump()

	local phys = player:GetRigidBody2DComponent()
	local vel = phys:GetRigidBody():GetLinearVelocity()

	vel.y = VerticalSpeed
	vel.x = SPEED

    phys:GetRigidBody():SetLinearVelocity(vel)
end


function OnUpdate(dt)

    phys = player:GetRigidBody2DComponent()

    up = Vector3.new(0, 1, 0)
    right = Vector3.new(1, 0, 0)

    if Input.GetKeyHeld( Key.Space ) or Input.GetMouseClicked(MouseButton.Left) then
        PlayerJump()
    end

    pos = player:GetTransform():GetWorldPosition()
    pos.y = 0.0

    camera:GetTransform():SetLocalPosition(pos)

end

function OnCleanUp()
end



