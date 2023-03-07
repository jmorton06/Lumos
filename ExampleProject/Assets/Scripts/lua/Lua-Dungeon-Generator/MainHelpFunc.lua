-----------------------------------------------------------------------------------
-- - - - - - - - - - - Help functions for main examples - - - - - - - - - - - - - -
-----------------------------------------------------------------------------------

function initPlayer(level)
  c=level:getRoot().center
  adj = getAdjacentPos(c[1], c[2])
  i=1
  repeat
    endr, endc = adj[i][1], adj[i][2]
    i=i+1
  until level:getTile(endr,endc).class == Tile.FLOOR
  
  level:getTile(endr,endc).class = Tile.PLAYER
end

-- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- 

function initBoss(level) 
  c=level:getEnd().center
  adj = getAdjacentPos(c[1], c[2])
  i=1
  repeat
    endr, endc = adj[i][1], adj[i][2]
    i=i+1
  until level:getTile(endr,endc).class == Tile.FLOOR
  
  level:getTile(endr,endc).class = Tile.BOSS
end

-- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- 