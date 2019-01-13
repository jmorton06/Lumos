# Unititled Engine

**Game Engine written in C++ using OpenGL and Vulkan.**

[![Build status](https://img.shields.io/appveyor/ci/jmorton06/UntitledEngine/dev.svg?style=flat&label=Windows)](https://ci.appveyor.com/project/jmorton06/UntitledEngine/branch/dev) [![Build status]( https://img.shields.io/travis/jmorton06/UntitledEngine/dev.svg?style=flat&label=Linux%20macOS)](https://travis-ci.org/jmorton06/UntitledEngine)
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