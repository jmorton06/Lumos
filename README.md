<h1 align="center" style="border-bottom: none;">
  <a href="https://github.com/jmorton06/Lumos/">Lumos Engine</a>
</h1>
<h3 align="center">Cross-platform 2D and 3D Game Engine written in C++ that supports both OpenGL and Vulkan.</h3>
<p align="center">
  <a href="#screenshots">Screenshots</a> |
  <a href="#building-🔨">Building</a> |
  <a href="#features">Features</a> |
  <a href="#dependencies">Dependencies</a>
<br/>
<br/>
<a href="https://actions-badge.atrox.dev/jmorton06/Lumos/goto"><img alt="Build" src="https://img.shields.io/endpoint.svg?url=https%3A%2F%2Factions-badge.atrox.dev%2Fjmorton06%2FLumos%2Fbadge&style=flat-square&label=build&branch=master&event=push"/></a>
<a href=""><img alt="platforms" src="https://img.shields.io/badge/Platforms-Windows%20%7C%20Linux%20%7C%20macOS%20%7C%20iOS-blue?style=flat-square"/></a>
<a href="https://github.com/jmorton06/Lumos/blob/master/LICENSE"><img alt="license" src="https://img.shields.io/github/license/jmorton06/Lumos?style=flat-square"/></a>
<br/>
<a href="https://github.com/jmorton06/Lumos/issues"><img alt="Issues" src="https://img.shields.io/github/issues-raw/jmorton06/Lumos.svg?style=flat-square"/></a>
<a href=""><img alt="size" src="https://img.shields.io/github/repo-size/jmorton06/Lumos?style=flat-square"/></a>
<a href=""><img alt="stars" src="https://img.shields.io/github/stars/jmorton06/Lumos?style=social"/></a>
<a href="https://discord.gg/n9PDrNjJwt"><img alt="Discord" src="https://img.shields.io/badge/chat-on_discord-7389D8.svg?logo=discord&logoColor=ffffff&labelColor=6A7EC2"/></a>
<br/>
</p>

#

## Screenshots
![Lumos](/Resources/Screenshot0721.png?raw=true)
#

## Building 🔨

```
git clone https://github.com/jmorton06/Lumos.git
```

#### Linux
```
sudo apt-get install -y g++-8 libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libopenal-dev mesa-common-dev
cd Lumos
Tools/premake5 gmake2
cd build
make -j8
```
#### Windows 
Run Scripts/GenerateVS.bat to generate a visual studio project.
```
cd Lumos
msbuild /p:Platform=x64 /p:Configuration=Release build/Lumos.sln
```
#### Mac
```
cd Lumos
Tools/premake5 xcode4
xcodebuild -project build/Runtime.xcodeproj
```

M1 Macs may need : 
```
cd Lumos
Tools/premake5 xcode4 --arch=arm64 --os=macosx
xcodebuild -project build/Runtime.xcodeproj
```

#### iOS
```
cd Lumos
Tools/premake5 xcode4 --os=ios
xcodebuild -project build/Runtime.xcodeproj CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO
```


To run on apple devices with Vulkan ( MoltenVK ), disable Metal API Validation here : Product > Scheme > Edit Scheme… > Run > Options > Metal API Validation
#

## Features

* Support for Windows, Linux, macOS.
* Support for OpenGL/Vulkan.
* 3D audio using OpenAL.
* Rendering 3D models with deferred PBR shading.
* Debug gui using ImGui
* Custom maths library with simd optimisation.
* 3D collision detection - cuboid/sphere/pyramid.
* 2D collision detection - Box2D.
* Basic lua scripting support.
#
## Contributing

* Contributions are welcome. Both new features and bug fixes. Just open a pull request.
#
## Dependencies
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
 * [sol2](https://github.com/ThePhD/sol2) : C++ <-> Lua API wrapper
 * [cereal](https://github.com/USCiLab/cereal) : A C++11 library for serialization
 * [meshoptimizer](https://github.com/zeux/meshoptimizer) : Mesh optimization library that makes meshes smaller and faster to render