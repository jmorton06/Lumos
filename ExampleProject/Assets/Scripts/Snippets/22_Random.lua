-- ============================================================================
-- 22_Random — seeded rand, weighted pick, jitter, shuffle
-- ============================================================================

-- ===== Snippet: Engine random ===============================================
-- Rand(a, b) is bound; returns float in [a, b]
local r = Rand(0.0, 1.0)
local n = math.floor(Rand(1, 7))  -- 1..6
-- ============================================================================


-- ===== Snippet: Seedable PRNG (deterministic, replay-safe) ==================
local function SeededRng(seed)
    local s = seed % 2147483647
    if s <= 0 then s = s + 2147483646 end
    return function()
        s = s * 16807 % 2147483647
        return s / 2147483647   -- [0, 1)
    end
end

local rng = SeededRng(42)
local val = rng()
-- ============================================================================


-- ===== Snippet: Weighted pick ===============================================
local function PickWeighted(list)   -- list = {{item=..., w=2}, ...}
    local total = 0
    for _, e in ipairs(list) do total = total + e.w end
    local r = math.random() * total
    local acc = 0
    for _, e in ipairs(list) do
        acc = acc + e.w
        if r <= acc then return e.item end
    end
    return list[#list].item
end

local prize = PickWeighted({
    { item = "common",   w = 70 },
    { item = "rare",     w = 25 },
    { item = "legendary",w = 5  },
})
-- ============================================================================


-- ===== Snippet: Jitter a vector =============================================
local function Jitter(v, amount)
    return Vec3.new(
        v.x + (math.random() * 2 - 1) * amount,
        v.y + (math.random() * 2 - 1) * amount,
        v.z + (math.random() * 2 - 1) * amount)
end
-- ============================================================================


-- ===== Snippet: Shuffle (Fisher-Yates) ======================================
local function Shuffle(t)
    for i = #t, 2, -1 do
        local j = math.random(i)
        t[i], t[j] = t[j], t[i]
    end
end
-- ============================================================================


-- ===== Snippet: Random point in sphere / on unit sphere =====================
local function RandPointInSphere(r)
    while true do
        local x, y, z = math.random()*2-1, math.random()*2-1, math.random()*2-1
        if x*x + y*y + z*z <= 1 then return Vec3.new(x*r, y*r, z*r) end
    end
end
local function RandOnUnitSphere()
    local p = RandPointInSphere(1)
    return p:Normalise()
end
-- ============================================================================


-- ===== Snippet: Daily seed (date string) ====================================
local function DailySeed()
    local d = os.date("*t")
    return d.year * 10000 + d.month * 100 + d.day
end
-- ============================================================================
