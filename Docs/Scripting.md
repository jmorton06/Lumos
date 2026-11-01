
# 📖 Lumos Engine Scripting

---

## 🚀 Overview

The Lumos Engine uses [Lua](https://www.lua.org/) as a lightweight scripting system for building gameplay and UI logic.  
Scripts run inside an isolated environment for each component, making it easy to:

✅ Load per-entity logic (eg. controlling NPCs, UI panels, game rules)  
✅ Call `OnInit`, `OnUpdate`, `OnCollision` callbacks from the C++ side  
✅ Provide hot-reloading by simply replacing the script file

The core is driven by the **`LuaScriptComponent`**, which attaches a Lua script to any Entity in your Scene.

---

## 🧩 How does it work?

When you add a `LuaScriptComponent` to an entity and give it a script file:

```cpp
auto entity = scene->CreateEntity("MyScriptedEntity");
entity.AddComponent<LuaScriptComponent>("Scripts/MyBehavior.lua", scene);
````

it does:

✅ Loads the Lua file in a fresh `sol::environment` (isolated scope)
✅ Searches for functions like:

```lua
function OnInit()
print("Hello from Lua Init")
end

function OnUpdate(dt)
-- runs every frame
end

function OnCollision2DBegin()
-- called on collision begin
end
```

✅ Calls these automatically at runtime:

* `OnInit()` once after loading
* `OnUpdate(dt)` every frame
* `OnCollision2DBegin`, `OnCollision2DEnd`, `OnCollision3DBegin`, `OnCollision3DEnd` on physics callbacks
* `OnRelease()` when the script is destroyed or reloaded

✅ Exposes `CurrentScene` and `LuaComponent` directly to Lua.

---

## ⚙️ How to set up a basic scripted entity

### ✍️ Example Lua script (`Scripts/MyBehavior.lua`)

```lua
function OnInit()
print("MyBehavior OnInit called!")
myValue = 0
end

function OnUpdate(dt)
myValue = myValue + dt
UIBeginFrame()
UILabel("Time alive: " .. myValue)
UIEndFrame()
end

function OnRelease()
print("MyBehavior cleaned up.")
end
```

Add to your entity like this:

```cpp
entity.AddComponent<LuaScriptComponent>("Scripts/MyBehavior.lua", scene);
```

✅ The engine then automatically calls these hooks at the right times.

---

## 🔄 Hot reloading

The `LuaScriptComponent` supports live reloading.

```cpp
entity.GetComponent<LuaScriptComponent>().Reload();
```

This:

* Calls `OnRelease()` on the old script
* Reloads the file
* Calls `OnInit()` again

Perfect for rapid gameplay iteration.

---

## 🚧 Collision callbacks

If your entity has a `LuaScriptComponent` **and** is involved in physics:

* `OnCollision2DBegin()`
* `OnCollision2DEnd()`
* `OnCollision3DBegin()`
* `OnCollision3DEnd()`

will be called automatically.

```lua
function OnCollision2DBegin()
print("Collision started!")
end

function OnCollision2DEnd()
print("Collision ended!")
end
```

---

## 🧰 Available from Lua

| Provided by your `LuaScriptComponent` | Example Usage                     |
| ------------------------------------- | --------------------------------- |
| `CurrentScene`                        | `CurrentScene:GetEntityManager()` |
| `LuaComponent`                        | (call functions or inspect)       |

Also inherits **all bindings from your `LuaManager`**, including:

✅ UI

* `UIBeginFrame`, `UIEndFrame`, `UILabel`, `UIButton`, `UIImage`, `UISlider`, `UIToggle`, `UIBeginPanel`, `UIEndPanel`, `UIPushStyle`, `UIPopStyle`

✅ Utilities

* `Rand(a, b)`
* `SwitchSceneByIndex`, `SwitchSceneByName`, `SwitchScene`
* `SetPhysicsDebugFlags`

✅ Enums for `WidgetFlags`, `SizeKind`, `StyleVar`, etc.

---

# 🔬 Internals: How `LuaScriptComponent` works

### 📂 Initialization

When you call:

```cpp
LuaScriptComponent(const std::string& fileName, Scene* scene);
```

it:

* Creates a new `sol::environment` to sandbox the script
* Loads the Lua file with `script_file`
* Registers:

* `CurrentScene` pointing to the Scene
* `LuaComponent` pointing to itself

---

### 🗂 Caching functions

It immediately looks up common hooks:

```cpp
m_OnInitFunc = (*m_Env)["OnInit"];
m_UpdateFunc = (*m_Env)["OnUpdate"];
m_Phys2DBeginFunc = (*m_Env)["OnCollision2DBegin"];
...
```

so calling them later is very fast.

---

### ⚠️ Error handling

If loading fails (eg. syntax error in Lua), it logs with `LERROR`:

```
Failed to Execute Lua script Scripts/MyScript.lua
Error: [string "Scripts/MyScript.lua"]:5: unexpected symbol near ...
```

Also attempts to parse out `.lua:line: message` for editor error highlighting.

If a callback fails (like `OnUpdate` crashes), it logs the error but continues running.

---

### 🧹 On destruction or reload

When reloading (or destroying):

```cpp
entity.GetComponent<LuaScriptComponent>().Reload();
```

it:

* Calls `OnRelease()` if defined
* Frees the old Lua state
* Loads the script again, calling `OnInit()`.

---

# 🖼 Example advanced script

```lua
function OnInit()
health = 100
print("Player ready with health:", health)
end

function OnUpdate(dt)
UIBeginFrame()
UILabel("Health: " .. health)
if UIButton("Take Damage") then
health = health - 10
end
UIEndFrame()

if health <= 0 then
print("Player died")
end
end

function OnCollision2DBegin()
print("Collided! Losing 5 HP")
health = health - 5
end

function OnRelease()
print("Cleaning up player")
end
```

---

# ✅ Summary: writing your own game

✅ Write your Lua script with any of these functions:

| Lua Function           | Called when                      |
| ---------------------- | -------------------------------- |
| `OnInit()`             | Once when script loads / reloads |
| `OnUpdate(dt)`         | Every frame with delta time      |
| `OnCollision2DBegin()` | On 2D collision start            |
| `OnCollision2DEnd()`   | On 2D collision end              |
| `OnCollision3DBegin()` | On 3D collision start            |
| `OnCollision3DEnd()`   | On 3D collision end              |
| `OnRelease()`          | On reload or destroy             |

✅ Attach to an entity:

```cpp
entity.AddComponent<LuaScriptComponent>("Scripts/MyScript.lua", scene);
```

✅ Use your UI bindings (`UILabel`, `UIButton`, `UIBeginPanel` etc) to build your game’s HUD or menus.

---
