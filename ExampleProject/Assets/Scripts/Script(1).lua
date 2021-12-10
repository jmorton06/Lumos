--Default Lua Script

local entity = {}
local phys = {}
local cameraEntitiy = {}
local speed = 10
local movementX = 0.0
local movementY = 0.0
local jumpTimer = 0.0

function OnInit()
entity = LuaComponent:GetCurrentEntity()
phys = entity:GetRigidBody3DComponent()

children = entity:GetChildren()
cameraEntity = children[1]

jumpTimer = 1
end

function OnUpdate(dt)

phys = entity:GetRigidBody3DComponent()

children = entity:GetChildren()
cameraEntity = children[1]

jumpTimer = jumpTimer + dt
if Input.GetKeyPressed( Key.Space ) and jumpTimer > 0.5 then
phys:GetRigidBody():SetLinearVelocity(phys:GetRigidBody():GetLinearVelocity() + Vector3.new(0.0,7.0,0.0))
jumpTimer =  0.0
elseif Input.GetKeyHeld( Key.W ) then
movementY = -1.0
elseif Input.GetKeyHeld( Key.S ) then
movementY = 1.0
else
movementY = 0.0
end

if Input.GetKeyHeld( Key.A ) then
movementX = -1.0
elseif Input.GetKeyHeld( Key.D ) then
movementX = 1.0
else
movementX = 0.0
end

local movement = cameraEntity:GetTransform():GetRightDirection() * movementX + cameraEntity:GetTransform():GetForwardDirection() * movementY
movement.y = 0.0

if(movement:Length() > 0.0) then
movement:Normalise()
local velocity = movement * speed 

local currentVelocity = phys:GetRigidBody():GetLinearVelocity() 
velocity.y  = currentVelocity.y
phys:GetRigidBody():SetLinearVelocity(velocity)
end

end

function OnCleanUp()
end





















