--Add flickering effect to light
            
local entity = {}
local lastIntensity = 400000

function OnInit()
entity = LuaComponent:GetCurrentEntity()
end

function OnUpdate(dt)
local light = entity:GetLight()
local randomNumber = Rand(-10000, 10000)
lastIntensity = math.max(0.0, lastIntensity + randomNumber)

light.Intensity = lastIntensity
end

function OnCleanUp()
end









