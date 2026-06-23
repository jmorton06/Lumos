Per-planet terrain textures for ArcheryMain.
Referenced by ExampleProject/Assets/Scripts/game/Planets.lua via _Tex("filename.png").

Expected files (all optional — missing files fall back to flat albedoColour):

  earth_grass.png        earth_grass_n.png      earth_grass_r.png
  moon_regolith.png      moon_regolith_n.png
  mars_dust.png          mars_dust_n.png
  jupiter_stone.png      jupiter_stone_n.png
  pluto_ice.png          pluto_ice_n.png

Naming convention:
  <planet>_<type>.png    = albedo
  <planet>_<type>_n.png  = tangent-space normal map (OpenGL convention, +Y up)
  <planet>_<type>_r.png  = roughness (greyscale)

UV tile factor per planet is set in Planets.lua (terrain.uvTile). Default
mesh tiling is 1/16 per cell; uvTile=24 gives ~24x repeats over the world span.

Drop new maps in this folder and the next level load will pick them up
(no engine rebuild needed — Material::SetAlbedoTexture loads at runtime).
