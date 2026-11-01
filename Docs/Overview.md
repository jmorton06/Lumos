# Lumos Engine Overview

The **Lumos Engine** is a cross-platform 2D and 3D game engine written in C++ that provides a comprehensive framework for game development. It offers:

- **Abstracted Rendering API support**: Vulkan  
- **Component-based entity system** for modular game objects  
- **Physics simulation** (2D & 3D)  
- **Audio subsystem** using OpenAL  
- **Fully‑featured editor** built with Dear ImGui  

---

## 📂 Relevant source files

- Core engine functionality (rendering, ECS, physics, audio)  
- `Lumos/Assets`, `Lumos/Source`, Editor and Runtime modules  
- Extensive binding systems for scripting, ImGui, input, etc.

---

## 🚀 Highlights

**Cross-platform support**  
- Native builds for **Windows**, **macOS**, **Linux**, and **iOS**

**Graphics & Rendering**  
- Supports **Physically Based Rendering (PBR)**  
- Powerful **forward-rendering pipeline** with shadows, post-processing, FXAA, bloom, depth-of-field, and more

**Entity Component System (ECS)**  
- Based on **EnTT** for fast, flexible entity management

**UI & Editor**  
- Integrated **Dear ImGui** and **imguizmo** for in-engine editing tools  
- Live scripting and GUI-driven resource workflows

**Scripting**  
- **Lua** integration via Sol2—attach scripts to entities for gameplay logic  
- Hot‑reload support in editor

---

## ⚙️ Build System

- Uses **Premake5** to generate platform-specific build files  
- Supports **Debug**, **Release**, and **Production** build types  
- Sample CI via **GitHub Actions** with matrix builds across platforms

---

## 📦 Dependencies

- **GLFW** (window/input)  
- **volk** (Vulkan loader)  
- **Dear ImGui**, **imguizmo** (editor UI)  
- **Box2D** (2D physics)  
- OpenAL (audio)  
- **tinygltf**, **tinyobjloader**, **meshoptimizer**, **cereal**  
- **spdlog** (logging)  
- **sol2** (Lua bindings)

---

## ✅ At a glance

| Feature            | Description |
|--------------------|-------------|
| Rendering APIs     | Vulkan (MoltenVK on Apple) |
| Physics            | Box2D (2D), custom (3D) |
| Audio              | OpenAL |
| Scripting          | Lua (Sol2 integration) |
| UI Framework       | Dear ImGui & imguizmo |
| ECS                | entt |
| Build Tooling      | Premake5, platform-native projects |

---
