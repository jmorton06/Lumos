local LevelModule = require("Lua-Dungeon-Generator/src/Level")

---------------------------------------------------------------------------
-- - - - - - - - - - - - - - - Dungeon object - - - - - - - - - - - - - - - 
---------------------------------------------------------------------------

-- Dungeon objects have several levels (consisting of Level objects) which
-- together represent a whole dungeon.

Dungeon = {nrOfLevels, height, width, levels}
Dungeon.__index = Dungeon

function Dungeon:new(nrOfLevels, height, width)
  local dungeon = {}
  dungeon.nrOfLevels = nrOfLevels
  dungeon.height = height
  dungeon.width = width
  dungeon.levels = {}
  
  setmetatable(dungeon, Dungeon)
  return dungeon
end

function Dungeon:generateDungeon(advanced, maxRooms, maxRoomSize, scatteringFactor)
  for i=1,self.nrOfLevels do
    newLevel = Level:new(self.height, self.width)
    if advanced then
      newLevel:setMaxRooms(maxRooms)
      newLevel:setMaxRoomSize(maxRoomSize)
      newLevel:setScatteringFactor(scatteringFactor)
    end
    newLevel:generateLevel()
    self.levels[i] = newLevel
  end
end

function Dungeon:printDungeon()
  for i=1,#dungeon.levels do
      local s = "L E V E L  "..i
      local space=""
    for i=1, math.floor((self.width+2)/2-(string.len(s))/4) do
      space = space.."  "
    end
    print(space..s..space)
    dungeon.levels[i]:printLevel()
    print()
  end
  print()
end