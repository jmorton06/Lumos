-- ============================================================================
-- 18_Save — persistent save (uses game/Save.lua patterns + raw file fallback)
-- ============================================================================

-- ===== Snippet: Use game/Save.lua (recommended) =============================
local Save = require("game/Save")

function OnInit()
    Save.Load()
    Log.Info("high score: " .. (Save.Get("highScore") or 0))
end

local function RecordScore(s)
    if (Save.Get("highScore") or 0) < s then
        Save.Set("highScore", s)
        Save.Flush()
    end
end
-- ============================================================================


-- ===== Snippet: Raw write/read (no schema) ==================================
local PATH = "save.dat"
local function Write(tbl)
    -- naive serialisation; replace with proper one for production
    local parts = {}
    for k, v in pairs(tbl) do
        parts[#parts+1] = string.format("%s=%s", tostring(k), tostring(v))
    end
    WriteFile(PATH, table.concat(parts, "\n"))
end
local function Read()
    local s = ReadFile(PATH) or ""
    local out = {}
    for line in s:gmatch("[^\n]+") do
        local k, v = line:match("([^=]+)=(.+)")
        if k then out[k] = v end
    end
    return out
end
-- ============================================================================


-- ===== Snippet: High score table (top N) ====================================
local Save = require("game/Save")

local function Top(n)
    local list = Save.Get("scores") or {}
    table.sort(list, function(a, b) return a > b end)
    while #list > n do table.remove(list) end
    return list
end
local function Submit(score)
    local list = Save.Get("scores") or {}
    table.insert(list, score)
    table.sort(list, function(a, b) return a > b end)
    while #list > 10 do table.remove(list) end
    Save.Set("scores", list)
    Save.Flush()
end
-- ============================================================================


-- ===== Snippet: Achievements ================================================
local Save = require("game/Save")
local function Unlock(key)
    if Save.HasAchievement and Save.HasAchievement(key) then return false end
    Save.UnlockAchievement(key)
    Log.Info("achievement: " .. key)
    return true
end
-- ============================================================================


-- ===== Snippet: Per-level best ==============================================
local Save = require("game/Save")
local function SaveBest(level, time)
    local best = Save.Get("best_" .. level)
    if not best or time < best then
        Save.Set("best_" .. level, time)
        Save.Flush()
        return true
    end
    return false
end
-- ============================================================================


-- ===== Snippet: Settings shortcut ===========================================
local Save = require("game/Save")
local vol = Save.GetSetting("volume", 0.8)
Save.SetSetting("volume", 0.6)         -- auto-flushed
-- ============================================================================
