#pragma once

#if LUMOS_PROFILE
#ifdef LUMOS_PLATFORM_WINDOWS
#define TRACY_CALLSTACK 1
#endif
#include <Tracy/Tracy.hpp>
#define LUMOS_PROFILE_SCOPE(name) ZoneScopedN(name)
#define LUMOS_PROFILE_FUNCTION() ZoneScoped
#define LUMOS_PROFILE_FRAMEMARKER() FrameMark
#define LUMOS_PROFILE_LOCK(type, var, name) TracyLockableN(type, var, name)
#define LUMOS_PROFILE_LOCKMARKER(var) LockMark(var)
#define LUMOS_PROFILE_SETTHREADNAME(name) tracy::SetThreadName(name)
#else
#define LUMOS_PROFILE_SCOPE(name)
#define LUMOS_PROFILE_FUNCTION()
#define LUMOS_PROFILE_FRAMEMARKER()
#define LUMOS_PROFILE_LOCK(type, var, name) type var
#define LUMOS_PROFILE_LOCKMARKER(var)
#define LUMOS_PROFILE_SETTHREADNAME(name)

#endif
