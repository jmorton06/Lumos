--Add flickering effect to light
            
local entity = {}
local lastIntensity = 2.0

function OnInit()
entity = LuaComponent:GetCurrentEntity()
end

function OnUpdate(dt)
local light = entity:GetLight()
local randomNumber = Rand(-0.1, 0.1)
lastIntensity = math.max(0.0, lastIntensity + randomNumber)

light.Intensity = lastIntensity
end

function OnCleanUp()
end







