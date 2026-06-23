-- ============================================================================
-- 16_Timers — cooldowns, delays, every-N-seconds, debounce
-- ============================================================================

-- ===== Snippet: Cooldown (fire-rate limiter) ================================
local cd = 0.0
function OnUpdate(dt)
    cd = math.max(0, cd - dt)
    if Input.GetKeyHeld(Key.F) and cd <= 0 then
        cd = 0.25   -- 4 shots/sec
        Log.Info("fire")
    end
end
-- ============================================================================


-- ===== Snippet: One-shot delay ==============================================
local delay = nil
local function Delay(sec, fn) delay = { t = sec, fn = fn } end

function OnUpdate(dt)
    if delay then
        delay.t = delay.t - dt
        if delay.t <= 0 then delay.fn(); delay = nil end
    end
    if Input.GetKeyPressed(Key.G) then Delay(2.0, function() Log.Info("boom") end) end
end
-- ============================================================================


-- ===== Snippet: Every-N-seconds tick ========================================
local tick, accum = 1.0, 0.0
function OnUpdate(dt)
    accum = accum + dt
    while accum >= tick do
        accum = accum - tick
        -- spawn an enemy, drain hp, etc.
    end
end
-- ============================================================================


-- ===== Snippet: Multiple parallel timers ====================================
local timers = {}
local function After(sec, fn) table.insert(timers, { t = sec, fn = fn }) end

function OnUpdate(dt)
    for i = #timers, 1, -1 do
        timers[i].t = timers[i].t - dt
        if timers[i].t <= 0 then
            timers[i].fn()
            table.remove(timers, i)
        end
    end
end
-- ============================================================================


-- ===== Snippet: Debounce ====================================================
local lastEvent = -1.0
local function Debounce(sec, fn)
    local now = os.clock()
    if now - lastEvent < sec then return end
    lastEvent = now
    fn()
end
-- Debounce(0.5, function() Log.Info("once per 500ms max") end)
-- ============================================================================


-- ===== Snippet: Throttle (uniform interval) =================================
local last = -math.huge
local function Throttle(sec, fn)
    local now = os.clock()
    if now - last >= sec then last = now; fn() end
end
-- ============================================================================


-- ===== Snippet: Frame counter ===============================================
local frame = 0
function OnUpdate(dt) frame = frame + 1 end
-- ============================================================================
