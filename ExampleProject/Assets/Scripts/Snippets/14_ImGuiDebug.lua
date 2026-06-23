-- ============================================================================
-- 14_ImGuiDebug — debug-only tweak panels via Dear ImGui
-- ============================================================================

-- ===== Snippet: Basic tweak window ==========================================
local val   = 0.0
local flag  = false
local color = { 1.0, 0.5, 0.2 }
function OnImGui()
    if imgui.beginWindow("Debug") then
        imgui.text("hello debug")
        imgui.sliderFloat("val", val, 0, 1, function(v) val = v end)
        imgui.checkbox("flag", flag, function(b) flag = b end)
        imgui.colourEdit3("colour", color[1], color[2], color[3],
            function(r,g,b) color = {r,g,b} end)
        if imgui.button("Reset") then val = 0.0 end
    end
    imgui.endWindow()
end
-- NOTE: many setups call OnImGui from OnUpdate; if your build calls it directly,
--       drop the call below.
function OnUpdate(dt) OnImGui() end
-- ============================================================================


-- ===== Snippet: Plot a frame-graph ==========================================
local samples, idx = {}, 1
for i=1,120 do samples[i] = 0 end
function OnUpdate(dt)
    samples[idx] = dt * 1000.0    -- ms
    idx = (idx % #samples) + 1
    if imgui.beginWindow("Perf") then
        imgui.text(string.format("dt: %.2f ms", dt * 1000))
        imgui.plotLines("frame ms", samples, "", 0.0, 33.0, 200, 60)
    end
    imgui.endWindow()
end
-- ============================================================================


-- ===== Snippet: Tweak entity transform ======================================
local e
function OnInit() e = LuaComponent:GetCurrentEntity() end
function OnUpdate(dt)
    if not (e and e:Valid()) then return end
    local t = e:GetTransform()
    local p = t.LocalPosition
    if imgui.beginWindow("Entity") then
        imgui.dragFloat("x", p.x, function(v) p.x = v; t:SetLocalPosition(p) end)
        imgui.dragFloat("y", p.y, function(v) p.y = v; t:SetLocalPosition(p) end)
        imgui.dragFloat("z", p.z, function(v) p.z = v; t:SetLocalPosition(p) end)
    end
    imgui.endWindow()
end
-- ============================================================================


-- ===== Snippet: Combo (dropdown) ============================================
local pick = "alpha"
local list = { "alpha", "beta", "gamma" }
function OnUpdate(dt)
    if imgui.beginWindow("Combo") then
        if imgui.beginCombo("mode", pick) then
            for _, v in ipairs(list) do
                if imgui.selectable(v, v == pick) then pick = v end
            end
            imgui.endCombo()
        end
    end
    imgui.endWindow()
end
-- ============================================================================


-- ===== Snippet: Demo / metrics windows (ImGui built-ins) ====================
function OnUpdate(dt)
    imgui.showDemoWindow()
    imgui.showMetricsWindow()
end
-- ============================================================================
