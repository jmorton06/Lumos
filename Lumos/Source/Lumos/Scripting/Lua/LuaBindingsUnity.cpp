// Unity build for Lua bindings to reduce Sol2 template instantiation overhead
// Combines all Lua binding .cpp files into one translation unit
// This allows the compiler to share template instantiations across all bindings

#include "Precompiled.h"
#include "Sol2Config.h"  // Configure Sol2 before any includes
#include "MathsLua.cpp"
#include "ImGuiLua.cpp"
#include "PhysicsLua.cpp"
// Note: LuaManager.cpp excluded as it has other non-binding code
// and BindECSLua function is called from LuaManager methods
