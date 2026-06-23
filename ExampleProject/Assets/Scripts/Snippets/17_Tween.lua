-- ============================================================================
-- 17_Tween — small tween library (no allocations after start)
-- ============================================================================

-- ===== Snippet: Tween library (drop into your script or util/) ==============
local Tween = {}
Tween.__index = Tween
Tween._all = {}

-- ease: function(t01) -> t01'
local function _Linear(t) return t end

function Tween.To(getter, setter, target, secs, ease)
    local from = getter()
    local tw = setmetatable({
        from = from, to = target, secs = secs, t = 0.0,
        getter = getter, setter = setter, ease = ease or _Linear,
        done = false, _next = nil,
    }, Tween)
    table.insert(Tween._all, tw)
    return tw
end

function Tween:Then(nextTween)
    self._next = nextTween
    nextTween._queued = true
    -- pull from active list until predecessor finishes
    for i, t in ipairs(Tween._all) do
        if t == nextTween then table.remove(Tween._all, i); break end
    end
    return nextTween
end

function Tween.Update(dt)
    for i = #Tween._all, 1, -1 do
        local tw = Tween._all[i]
        tw.t = tw.t + dt
        local k = math.min(1.0, tw.t / tw.secs)
        local v = tw.ease(k)
        local from, to = tw.from, tw.to
        if type(from) == "number" then
            tw.setter(from + (to - from) * v)
        else  -- assume Vec3
            tw.setter(Vec3.Lerp(from, to, v))
        end
        if k >= 1.0 then
            tw.done = true
            table.remove(Tween._all, i)
            if tw._next then
                tw._next.from = tw._next.getter()
                table.insert(Tween._all, tw._next)
            end
        end
    end
end

-- Usage:
-- function OnUpdate(dt) Tween.Update(dt) end
-- local t = LuaComponent:GetCurrentEntity():GetTransform()
-- Tween.To(function() return t.LocalPosition end,
--          function(p) t:SetLocalPosition(p) end,
--          Vec3.new(0, 5, 0), 1.2,
--          function(k) return SineInOut(k) end)
-- ============================================================================


-- ===== Snippet: Quick float tween (no lib) ==================================
local from, to, secs, t = 0.0, 10.0, 1.0, 0.0
function OnUpdate(dt)
    if t < secs then
        t = math.min(secs, t + dt)
        local v = from + (to - from) * SineOut(t / secs)
        -- apply v somewhere
    end
end
-- ============================================================================


-- ===== Snippet: Chain (move -> wait -> back) ================================
-- Combine with the Tween library above:
local t = LuaComponent:GetCurrentEntity():GetTransform()
local up = Tween.To(function() return t.LocalPosition end,
                    function(p) t:SetLocalPosition(p) end,
                    Vec3.new(0, 5, 0), 0.8, SineOut)
up:Then(Tween.To(function() return t.LocalPosition end,
                 function(p) t:SetLocalPosition(p) end,
                 Vec3.new(0, 0, 0), 0.8, SineIn))
-- ============================================================================
