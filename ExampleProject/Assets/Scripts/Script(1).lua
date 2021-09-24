--Default Lua Script

local entity = {}
local phys = {}
local cameraTransform = {}
local speed = 0.4

function OnInit()
entity = LuaComponent:GetCurrentEntity()
phys = entity:GetPhysics3DComponent():GetRigidBody()

children = entity:GetChildren()
cameraTransform = children[1]:GetTransform()
end

function OnUpdate(dt)
if Input.GetKeyHeld( Key.Space ) then
phys:SetLinearVelocity(phys:GetLinearVelocity() + Vector3.new(0.0,0.4,0.0))
end

if Input.GetKeyHeld( Key.W ) then
local forward = cameraTransform:GetForwardDirection() * speed
forward.y = 0.0
phys:SetLinearVelocity(phys:GetLinearVelocity() - forward)
end

if Input.GetKeyHeld( Key.S ) then
local forward = cameraTransform:GetForwardDirection() * speed
forward.y = 0.0
phys:SetLinearVelocity(phys:GetLinearVelocity() + forward)
end

if Input.GetKeyHeld( Key.A ) then
local right= cameraTransform:GetRightDirection() * speed
right.y = 0.0
phys:SetLinearVelocity(phys:GetLinearVelocity() - right)
end

if Input.GetKeyHeld( Key.D ) then
local right= cameraTransform:GetRightDirection() * speed
right.y = 0.0
phys:SetLinearVelocity(phys:GetLinearVelocity() + right)
end

end

function OnCleanUp()
end








