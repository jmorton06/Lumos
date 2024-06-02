
function OnInit()
	--SetPhysicsDebugFlags(PhysicsDebugFlags.BROADPHASE | PhysicsDebugFlags.COLLISIONVOLUMES | PhysicsDebugFlags.BROADPHASE_PAIRS)
end

local ProjectileChargeAmount = 0.0

function OnUpdate()
	if Input.GetKeyPressed( Key.M ) then
		SwitchScene()
	end
	
	if  Input.GetKeyPressed( Key.J ) then
		local cameraTransform = Entity.new(scene:GetEntityManager():GetRegistry():view_Camera(y):front(), scene):GetTransform()
		AddSphereEntity(scene, cameraTransform:GetWorldPosition(),-cameraTransform:GetForwardDirection())
	end

	if  Input.IsControllerButtonPressed(0, 7) then
		ProjectileChargeAmount =  ProjectileChargeAmount + 0.1
	elseif ProjectileChargeAmount > 0.0 then 
		local cameraTransform = Entity.new(scene:GetEntityManager():GetRegistry():view_Camera(y):front(), scene):GetTransform()
		AddSphereEntity(scene, cameraTransform:GetWorldPosition(), ProjectileChargeAmount * -cameraTransform:GetForwardDirection())
		ProjectileChargeAmount = 0.0
	end

	if  Input.GetKeyPressed( Key.K ) then
		local cameraTransform = Entity.new(scene:GetEntityManager():GetRegistry():view_Camera(y):front(), scene):GetTransform()
		AddPyramidEntity(scene, cameraTransform:GetWorldPosition(),-cameraTransform:GetForwardDirection())
	end

	if  Input.GetKeyPressed( Key.L ) then
		local cameraTransform = Entity.new(scene:GetEntityManager():GetRegistry():view_Camera(y):front(), scene):GetTransform()
		AddLightCubeEntity(scene, cameraTransform:GetWorldPosition(),-cameraTransform:GetForwardDirection())
	end
end





