
local random = math.random
local insert = table.insert
local remove = table.remove

-----------------------------------------------------------------------------------
-- - - - - - - - - - - - - - - Global Help Functions - - - - - - - - - - - - - - -- 
-----------------------------------------------------------------------------------

-- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- 

function getAdjacentPos(row, col)
  -- returns table containing all adjacent positions [1]:row [2]:col to given position
  -- INCLUDING SELF. to change this:
  -- add if (not (dx == 0 and dy == 0)) then ... end
  
  local result = {}
  for dx =-1,1 do
    for dy=-1,1 do 
      result[#result+1] = { row+dy, col+dx }
    end  
  end
  for i=1,#result do
  end
  return result
end

-- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- 

function findNext(start, goal)
  -- Finds next step from start position to goal position in a matrix
  if start == goal then return goal end
  row, col = start[1], start[2]
  local adj = getAdjacentPos(start[1], start[2])
  dist = getDist(start, goal)

  for i=1,#adj do
    local adjT = adj[i]
    if (getDist(adjT, goal) < dist) and
        i%2==0        -- not picking diagonals (would cause too narrow corridors)
    then
      nextPos = adjT
      dist = getDist(nextPos, goal)
      --break           -- uncomment for more rectangular paths!
    end
  end
  return nextPos
end

-- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- 

function getRandNeighbour(row, col, notDiag)
  if notDiag then 
    local f = {-1, 1}
    local d = f[random(1,2)]
    if random()>0.5 then
      return row+d, col
    else
      return row, col+d
    end
  else
    local dir={ random(-1,1), random(-1,1) }
    return row+dir[1], col+dir[2]
  end
end

-- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- 

function withinBounds(r, c, height, width)
  return (r<height and r>0 and c<width and c>0)
end

-- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- 

function prims(unvisited)
  len = #unvisited
  local root=remove(unvisited, 1)
  if #unvisited==0 then return root, root end
  
  local visited={}
  insert(visited, root)
  repeat
    local dist = 1e309    -- ~inf
    for i=1,#visited do
      for j=1,#unvisited do
        if (unvisited[j]:distanceTo(visited[i]) < dist) then
          dist = unvisited[j]:distanceTo(visited[i])
          v0 = visited[i]
          endIndex=j
        end
      end
    end
    v1 = remove(unvisited, endIndex)
    v0:addNeighbour(v1)
    insert(visited, v1)
until #visited == len

return visited[1], visited[#visited]
end

-- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- 

-- source: https://stackoverflow.com/questions/2705793/how-to-get-number-of-entries-in-a-lua-table
function tablelength(T)
  local count = 0
  for _ in pairs(T) do count = count + 1 end
  return count
end

-- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- 

function table.clone(org)
  local newTable = {}
  for k,v in pairs(org) do
      newTable[k] = v
  end
  return newTable
end

-- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- 

function getDist(start, goal)
  return math.sqrt(
    math.pow(math.abs(goal[1]-start[1]),2)+
    math.pow(math.abs(goal[2]-start[2]),2)
    )
end  

-- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- ##### -- 
