--Default Lua Script

local entity = {}
local phys = {}
local cameraEntitiy = {}
local speed = 10
local movementX = 0.0
local movementY = 0.0
local jumpTimer = 0.0

 function GetMovementAxis(axis, deadzone)
local value = Input.GetControllerAxis(0, axis);
local returnValue = value

if ( math.abs(value) < deadzone) then
returnValue = 0.0
end
return returnValue;
end

  function GetHorizontalMovementAxis(deadzone) --0.2f

	 --Dpad
	 local hat = Input.GetControllerHat(0, 0);
if ((hat & 2) ~= 0) then
		return 1.0;
	end
if ((hat & 8) ~= 0) then
		return -1.0;
	end
	
	 --Analogue stick
	return GetMovementAxis(0, deadzone);
end

 function GetVerticalMovementAxis(deadzone)

	 --Dpad
	 local hat = Input.GetControllerHat(0, 0);
if ((hat & 4) ~= 0) then
		return 1.0;
	end
		
if ((hat & 1) ~= 0) then
		return -1.0;
	end
	
	 --Analogue stick
	return GetMovementAxis(1, deadzone);
end

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

movementX = 0.0
movementY = 0.0

movementX = GetHorizontalMovementAxis(0.2);
movementY = GetVerticalMovementAxis(0.2);

jumpTimer = jumpTimer + dt
if Input.GetKeyPressed( Key.Space ) or Input.IsControllerButtonPressed(0,0) and jumpTimer > 0.5 then
phys:GetRigidBody():SetLinearVelocity(phys:GetRigidBody():GetLinearVelocity() + Vector3.new(0.0,7.0,0.0))
jumpTimer =  0.0
elseif Input.GetKeyHeld( Key.W ) then
movementY = -1.0
elseif Input.GetKeyHeld( Key.S ) then
movementY = 1.0
end

if Input.GetKeyHeld( Key.A ) then
movementX = -1.0
elseif Input.GetKeyHeld( Key.D ) then
movementX = 1.0
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






















