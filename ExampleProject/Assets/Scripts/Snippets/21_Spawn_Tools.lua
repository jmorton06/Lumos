-- ============================================================================
-- 21_Spawn_Tools — mouse picking, screen-to-world, AOE select
-- ============================================================================

-- ===== Snippet: Click to spawn at terrain hit ===============================
function OnUpdate(dt)
    if Input.GetMouseClicked(MouseButton.Left) then
        -- For a real picking ray you need camera matrices; cheap fallback:
        -- raycast straight down from a high Y using terrain query.
        local mp = Input.GetMousePosition()
        local ss = Input.GetScreenSize()
        local nx = (mp.x / ss.x - 0.5) * 40   -- world X spread
        local nz = (mp.y / ss.y - 0.5) * 40   -- world Z spread
        local y  = TerrainHeightAt(nx, nz) or 0
        AddDecorSphere(scene, "Pin", Vec3.new(nx, y + 0.3, nz), 0.3, Vec4.new(1, 1, 0, 1))
    end
end
-- ============================================================================


-- ===== Snippet: Click to raycast hit a body =================================
-- Reuses Raycast (returns RaycastHit). Build the ray from the camera.
local function CameraRay()
    local camE
    for e in EachEntity() do if e:HasCamera() then camE = e; break end end
    if not camE then return nil end
    local p = camE:GetTransform():GetWorldPosition()
    local f = camE:GetTransform():GetForwardDirection()
    return p, f
end

function OnUpdate(dt)
    if Input.GetMouseClicked(MouseButton.Left) then
        local origin, fwd = CameraRay()
        if origin then
            local hit = Raycast(origin, fwd, 100.0)
            if hit:Hit() then
                DebugPoint(hit.point, 0.15, Vec4.new(0, 1, 0, 1))
            end
        end
    end
end
-- ============================================================================


-- ===== Snippet: AOE select nearby entities ==================================
local Math = require("util/Math")
local function Nearby(centre, radius)
    local out = {}
    for e in EachEntity() do
        if e:HasTransform() then
            local p = e:GetTransform():GetWorldPosition()
            if Math.Length(p - centre) <= radius then table.insert(out, e) end
        end
    end
    return out
end
-- ============================================================================


-- ===== Snippet: Snap-place along ray ========================================
function OnUpdate(dt)
    if Input.GetMouseHeld(MouseButton.Left) then
        local hit = Raycast(Vec3.new(0, 50, 0), Vec3.new(0, -1, 0), 100.0)
        if hit:Hit() then
            DebugPoint(hit.point, 0.1, Vec4.new(1, 1, 1, 0.6))
        end
    end
end
-- ============================================================================
