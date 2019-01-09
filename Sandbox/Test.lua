
function printMessage(a)
	print("JM :	[lua] - ",a)
end

printMessage("Hello From Lua")

debugout("Loading Lua file Test.lua")

test = nil

test = Entity("Test Entity")
test.PrintName();

local vec1 = Vector4(1.0,0.0,0.0,1.0)
local vec2 = Vector4(0.0,0.0,1.0,0.0)
local vector2 = Vector2(0.0,1.0)
local vector3 = Vector3(0.0,2.0,4.0)

local mat4 = Matrix4(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15)
mat4.Print()
vector2.Print()
vector3.Print()

vec3 = vec1.Add(vec1,vec2)
vec3.Print()

camera = MayaCamera(45.0,0.1,100.0)
camera.SetPosition(Vector3(-3.0, 10.0, 15.0))
camera.SetYaw(-20.0)
camera.SetPitch(-40.0)
camera.Print()

currentScene.SetCamera(camera)
currentScene.AddEntity(test)

function Init()
	printMessage("Init")
end

function update(dt)
	--printMessage(dt)
end

runProcess(function()
while(true) do
	update(getDeltaTime())
	fixedupdate()
end
end)

sumNumbers = function(a,b)
    printMessage("Adding Numbers In Lua")
    return a + b
end