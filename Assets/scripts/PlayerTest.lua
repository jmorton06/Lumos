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

    gui.beginWindow("Lua Test Window")
    gui.text("Player Test Window")
    gui.endWindow()
end