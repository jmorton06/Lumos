# Dungeon Generator

This is a procedural dungeon generator written in Lua 5.3.5 for roguelike games.

## Example Output (default settings)
<img src="https://github.com/vronc/DungeonGen/blob/master/Images/defaultEx.png" alt="drawing" width="500"/>

## Example Output (customized settings)
<img src="https://github.com/vronc/DungeonGen/blob/master/Images/customizedEx.png" alt="drawing" width="500"/>

## How to use

In the root folder there is a file [ExampleMain.lua](https://github.com/vronc/DungeonGen/blob/master/ExampleMain.lua) which 
includes two examples of how the generator can be used. 
- __main()__: Using the Dungeon class (which holds many levels) and default generation settings for levels.
- __mainCustomizedLevel()__: Generating one level with customized settings.

## Default algorithm for levels

- Creates matrix of given height/width
- Places non-overlapping rooms of random sizes across matrix.
- Creates minimal spanning tree of rooms using Prim's algorithm (to ensure whole dungeon is reachable). 
- Builds tile corridors between rooms by doing DFS through room tree.
- Adds staircases and doors randomly but with certain constraints.

## Available functionalities

### Settings that can be modified:

- Height/width
- Maximum room size
- Maximum number of rooms
- ScatteringFactor: an attribute that changes randomness when building corridors between rooms.
- Spawn rate of mineral veins "\*" / soil "%".

### Some useful functions to customize level:

- __Level:buildCorridor(from, to)__: builds a corridor from one room to another.
- __Level:addCycles()__: Builds cycles randomly between rooms.
- __Level:buildRandomTiles(r,c)__: Builds random floor tiles around given tile.
- __Level:getRandRoom()__: returns random room.
- __Level:getRoot()__: returns root room.
- __Level:getEnd()__: returns last leaf added to tree.
- __Level:addDoors()__
- __Level:addStaircases()/Level:getStaircases()__

