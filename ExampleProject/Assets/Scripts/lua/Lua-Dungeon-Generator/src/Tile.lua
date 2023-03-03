---------------------------------------------------------------------------
-- - - - - - - - - - - - - - - - Tile object - - - - - - - - - - - - - - -- 
---------------------------------------------------------------------------

-- Tile objects:

--  * Keeps track of room association, if not in room (default): roomId = 0
--  * Has graphic symbol to represent what kind of tile this is in a level
--    *   " " for empty
--    *   "." for floor
--    *   "#" for wall
--    *   "<" for ascending staircase
--    *   ">" for descending staircase
--    *   "%" for soil
--    *   "*" for mineral vein
--    *   "'" for open door
--    *   "+" for closed door

Tile = {class, roomId}
Tile.__index = Tile

Tile.EMPTY = " "
Tile.FLOOR = "."
Tile.WALL = "#"
Tile.A_STAIRCASE = "<"
Tile.D_STAIRCASE = ">"
Tile.SOIL = "%"
Tile.VEIN = "*"
Tile.C_DOOR = "+"
Tile.O_DOOR = "'"

Tile.PLAYER = "@"
Tile.BOSS = "B"

function Tile:new(t)
  local tile = {}
  tile.class = t
  tile.roomId = 0
  
  setmetatable(tile, Tile)
  
  return tile
  
end

-- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### --

function Tile:isWall() 
  return (
    self.class == Tile.WALL or
    self.class == Tile.SOIL or
    self.class == Tile.VEIN
    )
end