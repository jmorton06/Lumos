#pragma once

#if LUMOS_PROFILE_ENABLED && !defined(LUMOS_PROFILE)
#define LUMOS_PROFILE 1
#endif
#if LUMOS_PROFILE
#ifdef LUMOS_PLATFORM_WINDOWS
#define TRACY_CALLSTACK 1
#endif

#define LUMOS_TRACK_MEMORY 0
#define LUMOS_PROFILE_LOW 0
#define LUMOS_PROFILE_GPU_TIMINGS 0
#define LUMOS_VULKAN_MARKERS 0 // Disable when using OpenGL

#include <tracy/public/tracy/Tracy.hpp>
#define LUMOS_PROFILE_SCOPE(name) ZoneScopedN(name)
#define LUMOS_PROFILE_FUNCTION() ZoneScoped
#define LUMOS_PROFILE_FRAMEMARKER() FrameMark
#define LUMOS_PROFILE_LOCK(type, var, name) TracyLockableN(type, var, name)
#define LUMOS_PROFILE_LOCKMARKER(var) LockMark(var)
#define LUMOS_PROFILE_SETTHREADNAME(name) tracy::SetThreadName(name)

#if LUMOS_PROFILE_LOW
#define LUMOS_PROFILE_FUNCTION_LOW() ZoneScoped
#define LUMOS_PROFILE_SCOPE_LOW(name) ZoneScopedN(name)
#else
#define LUMOS_PROFILE_FUNCTION_LOW()
#define LUMOS_PROFILE_SCOPE_LOW(name)
#endif

#else
#define LUMOS_PROFILE_SCOPE(name)
#define LUMOS_PROFILE_FUNCTION()
#define LUMOS_PROFILE_FRAMEMARKER()
#define LUMOS_PROFILE_LOCK(type, var, name) type var
#define LUMOS_PROFILE_LOCKMARKER(var)
#define LUMOS_PROFILE_SETTHREADNAME(name)
#define LUMOS_PROFILE_FUNCTION_LOW()
#define LUMOS_PROFILE_SCOPE_LOW(name)

#endif
