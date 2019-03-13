# Lumos

**Game Engine written in C++ using OpenGL and Vulkan.**

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/dd716348d0124620abb2bbcaedfa9cb3)](https://app.codacy.com/app/jmorton06/Lumos?utm_source=github.com&utm_medium=referral&utm_content=jmorton06/Lumos&utm_campaign=Badge_Grade_Settings)
[![Build status](https://img.shields.io/appveyor/ci/jmorton06/Lumos.svg?style=flat-square&label=Windows)](https://ci.appveyor.com/project/jmorton06/Lumos) [![Build status]( https://img.shields.io/travis/jmorton06/Lumos.svg?style=flat-square&label=Linux%20macOS)](https://travis-ci.org/jmorton06/Lumos)

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

[![License](http://img.shields.io/:license-mit-blue.svg?style=flat-square)](http://doge.mit-license.org)
<br>