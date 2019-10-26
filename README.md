# Lumos

**Game Engine written in C++ using OpenGL and Vulkan.**

[![Build Status](https://img.shields.io/endpoint.svg?url=https%3A%2F%2Factions-badge.atrox.dev%2Fjmorton06%2FLumos%2Fbadge&style=flat-square&label=build)](https://actions-badge.atrox.dev/jmorton06/Lumos/goto)

## Building  🔨
 
### Premake
```
git clone https://github.com/jmorton06/Lumos.git
cd Lumos

Tools/premake5 gmake

cd build
make
```

## Features

* Support for Windows, Linux, macOS.
* Support for OpenGL/Vulkan.
* 3D audio using OpenAL-Soft.
* Rendering 3D models with deferred PBR shading.
* Debug gui using ImGui
* Custom maths library with simd optimisation.
* 3D collision detection - cuboid/sphere/pyramid.
* 2D collision detection - Box2D.
* Basic lua scripting support - bindings in progress.

## Screenshots

![Lumos](/Resources/Screenshot-1610.png?raw=true)
