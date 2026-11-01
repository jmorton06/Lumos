-- Editor extension: every .lua in <project>/Editor/ is run by the editor when the project opens.
-- Reload after an edit with the "Reload Editor Scripts" command.

local clicks = 0

-- a row in the command palette (Cmd/Ctrl + P)
Editor.RegisterCommand("Example: Log Selection", function()
    local entity = Editor.GetSelected()
    Log.Info(entity and "An entity is selected" or "Nothing selected")
end)

-- a panel of its own, built with the same UI functions a game uses
Editor.RegisterPanel("Example", function(width, height)
    UILabel("##exampletitle", "A panel drawn by Editor/Example.lua")
    UISpacer(6)
    if UIButton("Count###examplecount").clicked then
        clicks = clicks + 1
    end
    UILabel("##exampleclicks", "Pressed " .. clicks .. " times")
    UISpacer(6)
    if UIButton(Editor.IsPlaying() and "Stop###exampleplay" or "Play###exampleplay").clicked then
        if Editor.IsPlaying() then Editor.Stop() else Editor.Play() end
    end
end)
