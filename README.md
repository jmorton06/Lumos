# Lumos

**Game Engine written in C++ using OpenGL and Vulkan.**

[![Build Status](https://img.shields.io/endpoint.svg?url=https%3A%2F%2Factions-badge.atrox.dev%2Fjmorton06%2FLumos%2Fbadge&style=flat-square&label=build&branch=master&event=push)](https://actions-badge.atrox.dev/jmorton06/Lumos/goto)  ![](https://img.shields.io/github/repo-size/jmorton06/Lumos?style=flat-square)  ![](https://img.shields.io/github/stars/jmorton06/Lumos?style=social)

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

## Third Party
 * [imgui](https://github.com/ocornut/imgui) : Dear ImGui: Bloat-free Immediate Mode Graphical User interface for C++ with minimal dependencies.
 * [imguizmo](https://github.com/CedricGuillemet/ImGuizmo) : Immediate mode 3D gizmo for scene editing and other controls based on Dear Imgui.
 * [entt](https://github.com/skypjack/entt) : Fast and reliable entity-component system (ECS) 
 * [glfw](https://github.com/glfw/glfw) : A multi-platform library for OpenGL, OpenGL ES, Vulkan, window and input.
 * [spdlog](https://github.com/gabime/spdlog) : Fast C++ logging library.
 * [stb](https://github.com/nothings/stb) : Single-file public domain (or MIT licensed) libraries for C/C++.
 * [tinygltf](https://github.com/syoyo/tinygltf) : Header only C++11 tiny glTF 2.0 library
 * [tinyobjloader](https://github.com/syoyo/tinyobjloader) : Tiny but powerful single file wavefront obj loader
 * [volk](https://github.com/zeux/volk) : Meta loader for Vulkan API.
 * [glad](https://github.com/Dav1dde/glad) : Meta loader for OpenGL API.
 * [Box2D](https://github.com/erincatto/Box2D) : 2D physics engine.
 * [catch](https://github.com/catchorg/Catch2) : A testing framework.
 * [sol2](https://github.com/ThePhD/sol2) : C++ <-> Lua API wrapper
 * [IconFontCppHeaders](https://github.com/juliettef/IconFontCppHeaders) : C, C++ headers and C# classes for icon fonts
 * [jsonhpp](https://github.com/nlohmann/json) : JSON for Modern C++
