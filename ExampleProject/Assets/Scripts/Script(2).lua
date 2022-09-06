--Default Lua Script
local rotation = 0.0
                
function OnInit()
end

function OnUpdate(dt)

   local orientation = LuaComponent:GetCurrentEntity():GetTransform():LocalOrientation()
	orientation.y = orientation.y + 0.01
	orientation:Normalise(orientation)
   LuaComponent:GetCurrentEntity():GetTransform():SetLocalOrientation(orientation)
-- rotation = rotation + 0.1
end

function OnCleanUp()
end






