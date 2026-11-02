-- ============================================================================
-- Dungeon - a small roguelike example
--
-- Five floors made by Lua-Dungeon-Generator (Scripts/lua/Lua-Dungeon-Generator),
-- drawn as sprites cut from the Kenney roguelike sheet. Movement is on the grid
-- (no physics): a step is only taken if the tile allows it. Only the floor you
-- are on exists as entities; it is rebuilt when you take the stairs.
--
--   WASD / arrows   move (hold to keep walking)
--   touch           hold the pad (bottom left) to walk, or swipe anywhere for a step
--   >  stairs       down a floor, < back up
--   R               a new dungeon
--
-- Find the gold on each floor and the guardian at the bottom.
-- ============================================================================

require("Lua-Dungeon-Generator/src/Dungeon")
require("Lua-Dungeon-Generator/MainHelpFunc")

local SHEET  = "//Assets/Textures/Dungeon/roguelikeDungeon_transparent.png"
local HERO   = "//Assets/Textures/GoldenSpriteSheet.png"
local FLOORS = 5
local ROWS, COLS = 30, 44
local SIGHT = 6.5       -- tiles you can see around you
local STEP_TIME = 0.11  -- seconds between steps while a key is held

-- On a phone / tablet; DUNGEON_TOUCH=1 shows the touch controls on a desktop (the mouse plays the finger).
local IS_TOUCH = (GetPlatformName ~= nil and (GetPlatformName() == "iOS" or GetPlatformName() == "Android"))
    or (os.getenv ~= nil and os.getenv("DUNGEON_TOUCH") ~= nil)

-- cells of the sheet (16 px tiles, 1 px apart)
local CELL = {
    ["#"] = { 9, 2 },  ["."] = { 16, 14 }, ["<"] = { 16, 2 }, [">"] = { 20, 2 },
    ["%"] = { 13, 11 }, ["*"] = { 10, 12 }, ["'"] = { 22, 4 }, ["+"] = { 22, 6 },
}

local em, camera, hero, guardian
local guardianAt -- { r, c } of the guardian on the bottom floor
local sheet, heroTex
local dungeon
local depth = 1
local tiles = {}      -- [r][c] = entity, nil for empty space
local seen = {}       -- [depth][r][c] = true once in sight
local gold = {}       -- [depth] = { {r, c, entity}, ... }
local row, col = 0, 0
local drawRow, drawCol = 0.0, 0.0   -- eased position the hero is drawn at
local moveTimer = 0.0
local facing = 1
local walkTime = 0.0
local coins, steps = 0, 0
local won = false
local message, messageTime = "", 0.0

local function say(text)
    message, messageTime = text, 3.0
end

local function level() return dungeon.levels[depth] end
local function tileAt(r, c)
    local l = level()
    if r < 0 or c < 0 or r > l.height + 1 or c > l.width + 1 then return nil end
    return l.matrix[r][c]
end

local function isOpen(r, c)
    local t = tileAt(r, c)
    if not t then return false end
    local k = t.class
    return k == "." or k == "<" or k == ">" or k == "'" or k == "@" or k == "B"
end

-- world position of a tile: columns go right, rows go down
local function at(r, c, z) return Vec3.new(c, -r, z or 0.0) end

-- --- building a floor -------------------------------------------------------

local function clearFloor()
    for _, rowTiles in pairs(tiles) do
        for _, e in pairs(rowTiles) do
            if e:Valid() then e:Destroy() end
        end
    end
    tiles = {}
    for _, g in ipairs(gold[depth] or {}) do
        if g.entity and g.entity:Valid() then g.entity:Destroy() end
        g.entity = nil
    end
    if guardian and guardian:Valid() then guardian:Destroy() end
    guardian = nil
end

local function setCell(sprite, cell)
    sprite:SetSpriteSheet(Vec2.new(cell[1] + 0.03, cell[2] + 0.03), Vec2.new(16, 16), Vec2.new(15.5, 15.5), 1.0)
end

local function makeTile(r, c, class)
    local cell = CELL[class] or CELL["."]
    local e = em:Create()
    e:AddTransform():SetLocalPosition(at(r, c, -1.0))
    -- a touch over a tile wide so neighbours overlap (no seams), and the picture
    -- cut a little inside its cell so the sheet's 1 px gaps never bleed in
    local s = e:AddSprite(Vec2.new(-0.51, -0.51), Vec2.new(1.02, 1.02), Vec4.new(0, 0, 0, 1))
    s:SetTexture(sheet)
    setCell(s, cell)
    return e
end

local function scatterGold(l)
    local list = {}
    for _ = 1, 6 do
        for _ = 1, 40 do -- a few tries for an empty floor tile in a room
            local r, c = math.random(1, l.height), math.random(1, l.width)
            if l.matrix[r][c].class == "." and l:isRoom(r, c) then
                list[#list + 1] = { r = r, c = c }
                break
            end
        end
    end
    return list
end

local function buildFloor()
    local l = level()
    tiles = {}
    for r = 0, l.height + 1 do
        tiles[r] = {}
        for c = 0, l.width + 1 do
            local class = l.matrix[r][c].class
            if class == "@" or class == "B" then class = "." end
            if class ~= " " then tiles[r][c] = makeTile(r, c, class) end
        end
    end

    gold[depth] = gold[depth] or scatterGold(l)
    for _, g in ipairs(gold[depth]) do
        if not g.taken then
            g.entity = em:Create()
            g.entity:AddTransform():SetLocalPosition(at(g.r, g.c, -0.5))
            g.entity:AddSprite(Vec2.new(-0.18, -0.18), Vec2.new(0.36, 0.36), Vec4.new(1.0, 0.8, 0.2, 1.0))
        end
    end

    -- the guardian waits on the bottom floor
    if depth == FLOORS then
        for r = 0, l.height + 1 do
            for c = 0, l.width + 1 do
                if l.matrix[r][c].class == "B" then
                    guardian = em:Create()
                    guardian:AddTransform():SetLocalPosition(at(r, c, -0.4))
                    local s = guardian:AddSprite(Vec2.new(-0.9, -0.64), Vec2.new(1.8, 1.28), Vec4.new(1.0, 0.35, 0.35, 1.0))
                    s:SetTexture(heroTex)
                    s:SetSpriteSheet(Vec2.new(0, 0), Vec2.new(45, 32), Vec2.new(45, 32), 0.0)
                    guardianAt = { r = r, c = c }
                end
            end
        end
    end
    seen[depth] = seen[depth] or {}
end

-- start tile of a floor: where we came from or the generator's player tile
local function findTile(class)
    local l = level()
    for r = 0, l.height + 1 do
        for c = 0, l.width + 1 do
            if l.matrix[r][c].class == class then return r, c end
        end
    end
end

local function enterFloor(newDepth, arriveOn)
    clearFloor()
    depth = newDepth
    buildFloor()
    local r, c = findTile(arriveOn)
    if not r then r, c = findTile("@") end
    if not r then r, c = level():getRoot().center[1], level():getRoot().center[2] end
    row, col = r, c
    drawRow, drawCol = r, c
    say(depth == FLOORS and "The bottom floor. Something is waiting." or ("Floor " .. depth))
end

-- --- sight ------------------------------------------------------------------

-- Bright where you can see, dim where you have been, black elsewhere.
-- Only tiles near the hero change, so a step touches a few dozen sprites.
local function updateSight()
    local reach = math.ceil(SIGHT) + 2
    local memory = seen[depth]
    for r = row - reach, row + reach do
        local rowTiles = tiles[r]
        if rowTiles then
            memory[r] = memory[r] or {}
            for c = col - reach, col + reach do
                local e = rowTiles[c]
                if e then
                    local d = math.sqrt((r - row) ^ 2 + (c - col) ^ 2)
                    local lit = 0.0
                    if d <= SIGHT then
                        memory[r][c] = true
                        lit = 1.0 - (d / SIGHT) * 0.55
                    elseif memory[r][c] then
                        lit = 0.28
                    end
                    e:GetSprite():SetColour(Vec4.new(lit, lit, lit * 1.05, 1.0))
                end
            end
        end
    end
    for _, g in ipairs(gold[depth]) do
        if g.entity then
            local d = math.sqrt((g.r - row) ^ 2 + (g.c - col) ^ 2)
            g.entity:GetSprite():SetColour(Vec4.new(1.0, 0.8, 0.2, d <= SIGHT and 1.0 or 0.0))
        end
    end
    if guardian then
        local d = math.sqrt((guardianAt.r - row) ^ 2 + (guardianAt.c - col) ^ 2)
        guardian:GetSprite():SetColour(Vec4.new(1.0, 0.35, 0.35, d <= SIGHT and 1.0 or 0.0))
    end
end

-- --- moving -----------------------------------------------------------------

local function tryStep(dr, dc)
    if dc ~= 0 then facing = dc end
    local r, c = row + dr, col + dc
    local t = tileAt(r, c)
    if not t then return end
    if t.class == "+" then
        -- a closed door opens instead of being walked into
        t.class = "'"
        local cell = CELL["'"]
        setCell(tiles[r][c]:GetSprite(), cell)
        updateSight()
        return
    end
    if not isOpen(r, c) then return end
    row, col = r, c
    steps = steps + 1

    for _, g in ipairs(gold[depth]) do
        if not g.taken and g.r == row and g.c == col then
            g.taken = true
            coins = coins + 1
            if g.entity then g.entity:Destroy() end
            g.entity = nil
            say("Gold! (" .. coins .. ")")
        end
    end

    if guardian and guardianAt.r == row and guardianAt.c == col then
        won = true
        say("You found the guardian.")
    elseif t.class == ">" and depth < FLOORS then
        enterFloor(depth + 1, "<")
    elseif t.class == "<" and depth > 1 then
        enterFloor(depth - 1, ">")
    end
    updateSight()
end

local function newDungeon()
    clearFloor()
    dungeon = Dungeon:new(FLOORS, ROWS, COLS)
    dungeon:generateDungeon()
    initPlayer(dungeon.levels[1])
    initBoss(dungeon.levels[FLOORS])
    seen, gold = {}, {}
    coins, steps, won = 0, 0, false
    enterFloor(1, "@")
    updateSight()
end

-- --- engine callbacks -------------------------------------------------------

function OnInit()
    em = scene:GetEntityManager()
    SetB2DGravity(Vec2.new(0.0, 0.0))
    -- unexplored space is black, not sky
    Render.SetBool("Skybox", false)
    if SetSkyColour then SetSkyColour(Vec3.new(0.0, 0.0, 0.0)) end

    sheet = LoadTextureWithParams("dungeonSheet", SHEET, TextureFilter.Nearest, TextureWrap.ClampToEdge)
    heroTex = LoadTextureWithParams("dungeonHero", HERO, TextureFilter.Linear, TextureWrap.ClampToEdge)

    camera = em:Create("Camera")
    camera:AddTransform()
    local size = GetAppInstance():GetWindowSize()
    camera:AddCamera(size.x / math.max(size.y, 1.0), 7.0, 1.0)

    -- fingers: a button fires when lifted on it, so a drag that starts on one does not press it
    if IS_TOUCH and UISetClickOnRelease then UISetClickOnRelease(true) end

    hero = em:Create("Hero")
    hero:AddTransform()
    local s = hero:AddSprite(Vec2.new(-1.1, -0.78), Vec2.new(2.2, 1.56), Vec4.new(1, 1, 1, 1))
    s:SetTexture(heroTex)

    newDungeon()
end

-- A rounded overlay with no title bar, pinned to a corner / the middle of the view.
local function beginOverlay(id, anchor, mx, my, centred)
    local d = Input.GetDPIScale()
    UIPushStyle(StyleVar.Padding, Vec2.new(16 * d, 10 * d))
    UIPushStyle(StyleVar.CornerRadius, 8.0 * d)
    UIPushStyle(StyleVar.BackgroundColor, Vec4.new(0.05, 0.07, 0.12, 0.85))
    UIPushStyle(StyleVar.Border, Vec2.new(0.0, 0.0))
    UIBeginOverlay(id, SizeKind.MaxChild, 1, SizeKind.ChildSum, 1,
        centred and (WidgetFlags.StackVertically | WidgetFlags.CentreX) or WidgetFlags.StackVertically)
    UIWindowAnchor(anchor, mx or 0, my or 0)
    UIPopStyle(StyleVar.Border)
    UIPopStyle(StyleVar.BackgroundColor)
end

local function endOverlay()
    UIPopStyle(StyleVar.CornerRadius)
    UIPopStyle(StyleVar.Padding)
    UIEndPanel()
end

local function held(a, b)
    return Input.GetKeyHeld(a) or Input.GetKeyHeld(b)
end

-- --- touch controls ---------------------------------------------------------


local PAD_BUTTON = 66   -- logical px, one arrow button
local PAD_MARGIN = 34   -- from the bottom-left corner
local SWIPE_MIN  = 28   -- logical px before a drag counts as a swipe
local swipe = nil       -- { x, y } where a finger landed outside the pad
local padDir = { 0, 0 } -- what the pad is pressed towards this frame

-- every finger / the mouse, in logical px from the top-left
local function pointers()
    local d, list = Input.GetDPIScale(), {}
    if Input.GetTouchCount() > 0 then
        for _, t in ipairs(Input.GetTouches()) do list[#list + 1] = { x = t.x / d, y = t.y / d } end
    elseif Input.GetMouseHeld(MouseButton.Left) then
        local m = Input.GetMousePosition()
        list[1] = { x = m.x / d, y = m.y / d }
    end
    return list
end

local function padCentre()
    local d = Input.GetDPIScale()
    local size = Input.GetScreenSize()
    return PAD_MARGIN + PAD_BUTTON * 1.5, size.y / d - PAD_MARGIN - PAD_BUTTON * 1.5
end

-- the pad: a finger in one of the four arrow squares walks that way
local function padDirection(p)
    local cx, cy = padCentre()
    local dx, dy = p.x - cx, p.y - cy
    local reach = PAD_BUTTON * 1.5 + 12
    if math.abs(dx) > reach or math.abs(dy) > reach then return nil end
    if math.abs(dx) < PAD_BUTTON * 0.35 and math.abs(dy) < PAD_BUTTON * 0.35 then return { 0, 0 } end -- centre: rest
    if math.abs(dx) > math.abs(dy) then return { 0, dx > 0 and 1 or -1 } end
    return { dy > 0 and 1 or -1, 0 }
end

-- Returns a held direction (pad) and a one-off step (swipe), as { dr, dc }.
local function touchInput()
    padDir = { 0, 0 }
    local step = nil
    local list = pointers()
    local onPad = false
    for _, p in ipairs(list) do
        local dir = padDirection(p)
        if dir then
            onPad = true
            if dir[1] ~= 0 or dir[2] ~= 0 then padDir = dir end
        elseif not swipe then
            swipe = { x = p.x, y = p.y, lastX = p.x, lastY = p.y }
        end
    end
    if swipe then
        local p = nil
        for _, q in ipairs(list) do
            if not padDirection(q) then p = q end
        end
        if p then
            swipe.lastX, swipe.lastY = p.x, p.y
        else
            -- finger lifted: a long enough drag is one step that way
            local dx, dy = swipe.lastX - swipe.x, swipe.lastY - swipe.y
            if math.max(math.abs(dx), math.abs(dy)) >= SWIPE_MIN then
                if math.abs(dx) > math.abs(dy) then step = { 0, dx > 0 and 1 or -1 }
                else step = { dy > 0 and 1 or -1, 0 } end
            end
            swipe = nil
        end
    end
    return (padDir[1] ~= 0 or padDir[2] ~= 0) and padDir or nil, step
end

local function drawPad()
    local cx, cy = padCentre()
    local b = PAD_BUTTON
    local cells = { { 0, -1, -1, 0 }, { 0, 1, 1, 0 }, { -1, 0, 0, -1 }, { 1, 0, 0, 1 } } -- x, y offset in buttons; dr, dc
    for i, cell in ipairs(cells) do
        local x, y = cx + cell[1] * b - b * 0.5, cy + cell[2] * b - b * 0.5
        local on = padDir[1] == cell[3] and padDir[2] == cell[4]
        UIRect("dungeonPad" .. i, x + 3, y + 3, b - 6, b - 6, Vec4.new(1, 1, 1, on and 0.42 or 0.16), 12)
        -- a short bar on the outer side shows which way the button goes
        local bar = 0.36 * b
        if cell[1] ~= 0 then
            UIRect("dungeonPadMark" .. i, x + (cell[1] > 0 and b - 18 or 12), y + (b - bar) * 0.5, 6, bar, Vec4.new(1, 1, 1, 0.75), 3)
        else
            UIRect("dungeonPadMark" .. i, x + (b - bar) * 0.5, y + (cell[2] > 0 and b - 18 or 12), bar, 6, Vec4.new(1, 1, 1, 0.75), 3)
        end
    end
    UIRing("dungeonPadRing", cx, cy, b * 0.22, Vec4.new(1, 1, 1, 0.22), 2)
end

function OnUpdate(dt)
    dt = math.min(dt, 0.05)
    messageTime = math.max(0.0, messageTime - dt)

    if Input.GetKeyPressed(Key.R) then newDungeon() end

    if not won then
        local dr, dc = 0, 0
        if IS_TOUCH then
            local hold, step = touchInput()
            if hold then dr, dc = hold[1], hold[2] end
            if step then tryStep(step[1], step[2]) end
        end
        if dr ~= 0 or dc ~= 0 then
            -- the pad
        elseif held(Key.W, Key.Up) then dr = -1
        elseif held(Key.S, Key.Down) then dr = 1
        elseif held(Key.A, Key.Left) then dc = -1
        elseif held(Key.D, Key.Right) then dc = 1 end

        moveTimer = moveTimer - dt
        if (dr ~= 0 or dc ~= 0) and moveTimer <= 0.0 then
            tryStep(dr, dc)
            moveTimer = STEP_TIME
        elseif dr == 0 and dc == 0 then
            moveTimer = 0.0 -- a fresh press steps at once
        end
    end

    -- ease the drawn position onto the grid one, and walk-cycle while it moves
    local k = 1.0 - math.exp(-18.0 * dt)
    drawRow = drawRow + (row - drawRow) * k
    drawCol = drawCol + (col - drawCol) * k
    local moving = math.abs(row - drawRow) + math.abs(col - drawCol) > 0.02
    walkTime = moving and walkTime + dt or 0.0
    local frame = moving and (math.floor(walkTime * 12.0) % 6) or 0
    local s = hero:GetSprite()
    s:SetSpriteSheet(Vec2.new(frame, 0), Vec2.new(45, 32), Vec2.new(45, 32), 0.0)
    hero:GetTransform():SetLocalPosition(at(drawRow, drawCol, 0.0))
    hero:GetTransform():SetLocalScale(Vec3.new(facing, 1.0, 1.0))

    camera:GetTransform():SetLocalPosition(Vec3.new(drawCol, -drawRow, 0.0))

    if IS_TOUCH and not won then drawPad() end

    -- HUD
    beginOverlay("dungeonHud", UIAnchor.TopLeft, 16, 16)
    UILabel("dungeonDepth", string.format("Floor %d / %d     Gold %d     Steps %d", depth, FLOORS, coins, steps))
    if IS_TOUCH then
        UILabel("dungeonKeys", "Hold the pad or swipe to move")
        if UIButton("New dungeon###dungeonnew").clicked then newDungeon() end
    else
        UILabel("dungeonKeys", "WASD move   R new dungeon")
    end
    endOverlay()

    if messageTime > 0.0 and not won then
        beginOverlay("dungeonMsg", UIAnchor.BottomCenter, 0, 32)
        UILabel("dungeonMsgText", message)
        endOverlay()
    end

    if won then
        beginOverlay("dungeonWin", UIAnchor.MiddleCenter, 0, 0, true)
        UILabel("dungeonWinTitle", "You reached the guardian!")
        UILabel("dungeonWinStats", string.format("Gold %d     Steps %d", coins, steps))
        if UIButton("New dungeon###dungeonagain").clicked then newDungeon() end
        endOverlay()
    end
end

function OnRelease()
    -- the scene is being torn down with its entities: nothing to destroy here
end
