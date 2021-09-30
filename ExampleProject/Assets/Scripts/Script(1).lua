--Default Lua Script

local entity = {}
local phys = {}
local cameraTransform = {}
local speed = 10
local movementX = 0.0
local movementY = 0.0

function OnInit()
entity = LuaComponent:GetCurrentEntity()
phys = entity:GetPhysics3DComponent():GetRigidBody()

children = entity:GetChildren()
cameraTransform = children[1]:GetTransform()
end

function OnUpdate(dt)

if Input.GetKeyHeld( Key.Space ) then
phys:SetLinearVelocity(phys:GetLinearVelocity() + Vector3.new(0.0,0.4,0.0))
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

local movement = cameraTransform:GetRightDirection() * movementX + cameraTransform:GetForwardDirection() * movementY
movement.y = 0.0

if(movement:Length() > 0.0) then
movement:Normalise()
local velocity = movement * speed 

local currentVelocity = phys:GetLinearVelocity() 
velocity.y  = currentVelocity.y
phys:SetLinearVelocity(velocity)
end

end

function OnCleanUp()
end













