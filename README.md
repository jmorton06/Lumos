<h1 align="center" style="border-bottom: none;">
  <a href="https://github.com/jmorton06/Lumos/">Lumos Engine</a>
</h1>
<h3 align="center">Cross-platform 2D and 3D Game Engine written in C++.</h3>
<p align="center">
  <a href="#screenshots">Screenshots</a> |
  <a href="#building-🔨">Building</a> |
  <a href="#features">Features</a> |
  <a href="#dependencies">Dependencies</a>
<br/>
<br/>
<a href="https://actions-badge.atrox.dev/jmorton06/Lumos/goto?ref=main"><img alt="Build" src="https://img.shields.io/endpoint.svg?url=https%3A%2F%2Factions-badge.atrox.dev%2Fjmorton06%2FLumos%2Fbadge%3Fref%3Dmain&style=flat-square&event=push"/></a>
<a href="https://github.com/jmorton06/Lumos/releases/latest"><img alt="platforms" src="https://img.shields.io/badge/Platforms-Windows%20%7C%20Linux%20%7C%20macOS%20%7C%20iOS-blue?style=flat-square"/></a>
<a href="https://github.com/jmorton06/Lumos/blob/main/LICENSE"><img alt="license" src="https://img.shields.io/github/license/jmorton06/Lumos?style=flat-square"/></a>
<br/>
<a href=""><img alt="stars" src="https://img.shields.io/github/stars/jmorton06/Lumos?style=flat-square"/></a>
<a href="https://github.com/jmorton06/Lumos/releases/latest"><img alt="release" src="https://img.shields.io/github/v/release/jmorton06/Lumos?style=flat-square"/></a>
<a href="https://discord.gg/n9PDrNjJwt"><img alt="Discord" src="https://img.shields.io/badge/chat-on_discord-7389D8.svg?logo=discord&logoColor=ffffff&labelColor=6A7EC2&style=flat-square"/></a>
<br/>
</p>

#

## Screenshots
![Lumos](/Resources/Screenshot0424-2.png?raw=true)
![Lumos](/Resources/Screenshot0424.png?raw=true)
![Lumos](/Resources/Screenshot1022.png?raw=true)
![Lumos](/Resources/Screenshot0923.png?raw=true)
#

## Building 🔨

```
git clone https://github.com/jmorton06/Lumos.git
git submodule update --init --recursive
```

Install Vulkan SDK (https://vulkan.lunarg.com/)

#### Linux
```
sudo apt-get install -y g++-11 libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libopenal-dev mesa-common-dev
cd Lumos
Tools/linux/premake5 gmake2
make -j8 # config=release
```
#### Windows
Run Scripts/GenerateVS.bat to generate a visual studio project.
```
cd Lumos
msbuild /p:Platform=x64 /p:Configuration=Release Lumos.sln
```
#### Mac
```
cd Lumos
Tools/premake5 xcode4
xcodebuild -project Runtime.xcodeproj
```

M1/M2/M3 Macs may need :
```
cd Lumos
Tools/premake5 xcode4 --arch=arm64 --os=macosx
xcodebuild -project Runtime.xcodeproj
```

#### iOS
```
cd Lumos
Tools/premake5 xcode4 --os=ios
xcodebuild -project Runtime.xcodeproj CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO
```

To run on apple devices with Vulkan ( MoltenVK ), disable Metal API Validation here : Product > Scheme > Edit Scheme… > Run > Options > Metal API Validation
#

## Features

* Support for Windows, Linux, macOS.
* Vulkan
* 3D audio using OpenAL.
* Rendering 3D models with PBR shading.
* Debug gui using ImGui
* Custom 3D collision detection - cuboid/sphere/pyramid.
* 2D collision detection - Box2D.
* Basic lua scripting support.
#
## Contributing

* Contributions are welcome. Both new features and bug fixes. Just open a pull request.
* Some ideas for things that need added/fixed are in Todo.txt
#
## Dependencies
 * [imgui](https://github.com/ocornut/imgui) : Dear ImGui: Bloat-free Immediate Mode Graphical User interface for C++ with minimal dependencies.
 * [imguizmo](https://github.com/CedricGuillemet/ImGuizmo) : Immediate mode 3D gizmo for scene editing and other controls based on Dear Imgui.
 * [entt](https://github.com/skypjack/entt) : Fast and reliable entity-component system (ECS)
 * [glfw](https://github.com/glfw/glfw) : A multi-platform library for OpenGL, OpenGL ES, Vulkan, window and input.
 * [stb](https://github.com/nothings/stb) : Single-file public domain (or MIT licensed) libraries for C/C++.
 * [tinygltf](https://github.com/syoyo/tinygltf) : Header only C++11 tiny glTF 2.0 library
 * [tinyobjloader](https://github.com/syoyo/tinyobjloader) : Tiny but powerful single file wavefront obj loader
 * [volk](https://github.com/zeux/volk) : Meta loader for Vulkan API.
 * [Box2D](https://github.com/erincatto/Box2D) : 2D physics engine.
 * [sol2](https://github.com/ThePhD/sol2) : C++ <-> Lua API wrapper
 * [cereal](https://github.com/USCiLab/cereal) : A C++11 library for serialization
 * [meshoptimizer](https://github.com/zeux/meshoptimizer) : Mesh optimization library that makes meshes smaller and faster to render
