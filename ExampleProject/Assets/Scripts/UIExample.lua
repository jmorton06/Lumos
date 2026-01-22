-- ui_example.lua
-- Minimal illustrative Lua UI script

local value = 0.5
local enabled = false

function UpdateUI()

    UIBeginPanel("Lua Demo")
        UILabel("Label1", "Hello from Lua")

        local btn = UIButton("Click Me")
        if btn and btn.clicked then
            print("Lua: Button clicked")
        end

        UISlider("Volume", value, 0.0, 1.0)
        UIToggle("Enabled", enabled)
    UIEndPanel()

end

function OnUpdate()
	UpdateUI()
end

