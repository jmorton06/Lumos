# Unititled Engine

**Game Engine written in C++ using OpenGL and Vulkan.**

## Building

### CMake

```
git clone https://github.com/jmorton06/UnititledEngine.git
cd UnititledEngine

mkdir build
cd build
cmake ..

```

### Premake
```
git clone https://github.com/jmorton06/UnititledEngine.git
cd UnititledEngine

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

[![License](http://img.shields.io/:license-mit-blue.svg)](http://doge.mit-license.org)
<br>


