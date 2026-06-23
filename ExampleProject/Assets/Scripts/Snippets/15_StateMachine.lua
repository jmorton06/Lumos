-- ============================================================================
-- 15_StateMachine — FSM patterns
-- ============================================================================

-- ===== Snippet: Simple FSM (numeric phases) =================================
local PHASE = { Idle=0, Run=1, Jump=2, Dead=3 }
local phase, phaseT = PHASE.Idle, 0.0

local function SetPhase(p)
    if phase ~= p then phase = p; phaseT = 0.0 end
end

function OnUpdate(dt)
    phaseT = phaseT + dt
    if phase == PHASE.Idle then
        if Input.GetKeyHeld(Key.W)     then SetPhase(PHASE.Run)  end
        if Input.GetKeyPressed(Key.Space) then SetPhase(PHASE.Jump) end
    elseif phase == PHASE.Run then
        if not Input.GetKeyHeld(Key.W) then SetPhase(PHASE.Idle) end
    elseif phase == PHASE.Jump then
        if phaseT > 0.8 then SetPhase(PHASE.Idle) end
    end
end
-- ============================================================================


-- ===== Snippet: Table-driven FSM (per-state enter/update) ===================
local fsm = {}
local cur, curT = "Idle", 0.0

fsm.Idle = {
    Enter  = function() Log.Info("enter idle") end,
    Update = function(dt)
        if Input.GetKeyHeld(Key.W) then return "Run" end
    end,
}
fsm.Run = {
    Enter  = function() Log.Info("enter run") end,
    Update = function(dt)
        if not Input.GetKeyHeld(Key.W) then return "Idle" end
    end,
}

local function Transition(to)
    if cur == to then return end
    cur, curT = to, 0.0
    if fsm[cur].Enter then fsm[cur].Enter() end
end

function OnInit() if fsm[cur].Enter then fsm[cur].Enter() end end
function OnUpdate(dt)
    curT = curT + dt
    local next = fsm[cur].Update and fsm[cur].Update(dt) or nil
    if next then Transition(next) end
end
-- ============================================================================


-- ===== Snippet: Coroutine-driven flow (cutscene) ============================
local co
local function Wait(sec)
    local t = 0
    while t < sec do
        t = t + coroutine.yield()    -- caller yields dt back in
    end
end

local function Scene()
    Log.Info("step 1")
    Wait(1.5)
    Log.Info("step 2")
    Wait(0.5)
    Log.Info("done")
end

function OnInit() co = coroutine.create(Scene) end
function OnUpdate(dt)
    if co and coroutine.status(co) ~= "dead" then
        local ok, err = coroutine.resume(co, dt)
        if not ok then Log.Error("coroutine: " .. tostring(err)) end
    end
end
-- ============================================================================


-- ===== Snippet: Stack-based state (push/pop) ================================
local stack = { "Play" }
local function Top()    return stack[#stack] end
local function Push(s)  table.insert(stack, s) end
local function Pop()    if #stack > 1 then table.remove(stack) end end

function OnUpdate(dt)
    local s = Top()
    if s == "Play" then
        if Input.GetKeyPressed(Key.Escape) then Push("Pause") end
    elseif s == "Pause" then
        if Input.GetKeyPressed(Key.Escape) then Pop() end
    end
end
-- ============================================================================
