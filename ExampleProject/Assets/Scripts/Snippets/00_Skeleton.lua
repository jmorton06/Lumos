-- ============================================================================
-- 00_Skeleton — script lifecycle + module boilerplate
-- ============================================================================

-- ===== Snippet: Minimal script ==============================================
function OnInit()
    Log.Info("hello from lua")
end

function OnUpdate(dt)
    -- per-frame logic
end

function OnRelease()
    -- cleanup on detach / scene unload
end
-- ============================================================================


-- ===== Snippet: With collision hooks (2D) ===================================
function OnInit()  end
function OnUpdate(dt) end

function OnCollision2DBegin()
    Log.Info("2D collision begin")
end

function OnCollision2DEnd()
    Log.Info("2D collision end")
end
-- ============================================================================


-- ===== Snippet: With collision hooks (3D) ===================================
function OnCollision3DBegin(info)
    -- info has hit data; check engine for available fields
    Log.Info("3D collision begin")
end

function OnCollision3DEnd(info)
    Log.Info("3D collision end")
end
-- ============================================================================


-- ===== Snippet: Module-style script (reusable + hot-reload friendly) ========
local M = {}

M._t = 0.0

function M.Init()
    M._t = 0.0
end

function M.Update(dt)
    M._t = M._t + dt
end

function OnInit()    M.Init()     end
function OnUpdate(dt) M.Update(dt) end
-- ============================================================================


-- ===== Snippet: Reuse a sibling module via require ==========================
-- Files in //Assets/Scripts/ are required by path WITHOUT extension.
local Math = require("util/Math")
local Save = require("game/Save")

function OnInit()
    Save.Load()
end

function OnUpdate(dt)
    local x = Math.Damp(0, 10, 4.0, dt)
end
-- ============================================================================


-- ===== Snippet: Cache `this` entity + scene =================================
-- Engine injects two globals into every script env:
--   scene         -> Scene*
--   LuaComponent  -> this LuaScriptComponent; GetCurrentEntity() -> owning entity
local _entity, _scene, _t

function OnInit()
    _scene  = scene
    _entity = LuaComponent:GetCurrentEntity()
    _t      = _entity:GetOrAddTransform()
end

function OnUpdate(dt)
    if _entity and _entity:Valid() then
        _t:SetLocalPosition(Vec3.new(0, math.sin(os.clock()) * 0.5, 0))
    end
end
-- ============================================================================
