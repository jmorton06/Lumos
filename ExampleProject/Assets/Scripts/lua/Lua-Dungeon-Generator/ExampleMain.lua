local DungeonModule = require("Lua-Dungeon-Generator/src/Dungeon")
local LevelModule = require("Lua-Dungeon-Generator/src/Level")
local FuncModule = require("MainHelpFunc")

-----------------------------------------------------------------------------------
-- - - - - - - - - - - - - - - - Examples of main - - - - - - - - - - - - - - - - -
-----------------------------------------------------------------------------------

-- Example of generating a dungeon with: 
--  * default settings by using generateDungeon.
--  * "Advanced" settings with max nr of rooms, room size and scattering factor.

function main()

  -- Settings for level sizes and number of levels in dungeon.
  height=40
  width=60
  nrOfLevels=5

  dungeon = Dungeon:new(nrOfLevels, height, width)
  
  -- generate with default settings
  dungeon:generateDungeon()
  
  -- generate with advanced settings, 
  -- params: (advanced, maxRooms, maxRoomSize, scatteringFactor)
  -- dungeon:generateDungeon(true, 30, 10, 30)
  
  -- inits a player in level 1, a boss in last level
  initPlayer(dungeon.levels[1])
  initBoss(dungeon.levels[#dungeon.levels])

  dungeon:printDungeon()
end

main()

-- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- 

-- Example of generating one level with customized settings.

-- Some targeted functions to play with:
--  * setMaxRooms: maximum nr of rooms.
--  * setMaxRoomSize: keep it lower than height/width-3 though!
--  * setScatteringFactor: increases random scattering of tiles when forming corridors.
--  * addCycles: adds random cycles between rooms up to parameter value.

function mainCustomizedLevel()
  height = 40
  width = 60
  level = Level:new(height, width)
  level:setMaxRooms(30)
  level:setMaxRoomSize(5)
  level:setScatteringFactor(10)
  Level.veinSpawnRate=0.4
  level:initMap(level.height, level.width)
  level:generateRooms()
  root=level:getRoomTree()
  level:buildCorridors(root)
  level:buildCorridor(root, level:getEnd())
  level:addCycles(10)
  level:addStaircases(10)
  level:addDoors()
  initBoss(level)
  initPlayer(level)
  level:printLevel()
end
  
--mainCustomizedLevel()
