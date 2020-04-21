player = {}

function TestFunction ()
    Log.Warn("Test Func")
end

function OnInit()
    Log.Warn("Player Test - Updated - Component OnInit")
end 

function OnUpdate(dt)
    if Input.GetKeyPressed( Key.X ) then
        Log.Critical("Player Test - Updated - Component  OnUpdate")
    end
end