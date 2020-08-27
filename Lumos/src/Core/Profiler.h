#pragma once

#if LUMOS_PROFILE
#include <Tracy/Tracy.hpp>
#define LUMOS_PROFILE_SCOPE(name) ZoneScopedN(name)
#define LUMOS_PROFILE_FUNCTION() ZoneScoped
#define LUMOS_PROFILE_FRAMEMARKER() FrameMark
#define LUMOS_PROFILE_LOCK(type, var, name) TracyLockableN(type, var, name)
#define LUMOS_PROFILE_LOCKMARKER(var) LockMark(var)
#else
#define LUMOS_PROFILE_SCOPE(name)
#define LUMOS_PROFILE_FUNCTION()
#define LUMOS_PROFILE_FRAMEMARKER()
#define LUMOS_PROFILE_LOCK(type, var, name) type var
#define LUMOS_PROFILE_LOCKMARKER(var)
#endif