cmake_minimum_required(VERSION 3.1.0)
project(lua C)

set(BUILD_SHARED_LIBS ON)

add_library(lua
	src/lapi.c
	src/lapi.h
	src/lauxlib.c
	src/lauxlib.h
	src/lbaselib.c
	src/lbitlib.c
	src/lcode.c
	src/lcode.h
	src/lcorolib.c
	src/lctype.c
	src/lctype.h
	src/ldblib.c
	src/ldebug.c
	src/ldebug.h
	src/ldo.c
	src/ldo.h
	src/ldump.c
	src/lfunc.c
	src/lfunc.h
	src/lgc.c
	src/lgc.h
	src/linit.c
	src/liolib.c
	src/llex.c
	src/llex.h
	src/llimits.h
	src/lmathlib.c
	src/lmem.c
	src/lmem.h
	src/loadlib.c
	src/lobject.c
	src/lobject.h
	src/lopcodes.c
	src/lopcodes.h
	src/loslib.c
	src/lparser.c
	src/lparser.h
	src/lprefix.h
	src/lstate.c
	src/lstate.h
	src/lstring.c
	src/lstring.h
	src/lstrlib.c
	src/ltable.c
	src/ltable.h
	src/ltablib.c
	src/ltm.c
	src/ltm.h
	src/luaconf.h
	src/lualib.h
	src/lua.h
	src/lua.hpp
	src/lundump.c
	src/lundump.h
	src/lutf8lib.c
	src/lvm.c
	src/lvm.h
	src/lzio.c
	src/lzio.h)

target_include_directories(lua PUBLIC "src/")


if(WIN32)
	target_compile_definitions(lua PRIVATE -D_CRT_SECURE_NO_WARNINGS)
	if(BUILD_SHARED_LIBS)
		target_compile_definitions(lua PRIVATE -DLUA_BUILD_AS_DLL)
	endif()
elseif(CMAKE_SYSTEM_NAME MATCHES "AIX")
	target_compile_definitions(lua PUBLIC -DLUA_USE_POSIX -DLUA_USE_DLOPEN)
	target_link_libraries(lua dl)
elseif(CMAKE_SYSTEM_NAME MATCHES "DragonFly" OR CMAKE_SYSTEM_NAME MATCHES "OpenBSD" OR CMAKE_SYSTEM_NAME MATCHES "NetBSD")
	target_compile_definitions(lua PUBLIC -DLUA_USE_POSIX -DLUA_USE_DLOPEN)
elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD" OR CMAKE_SYSTEM_NAME MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "Android")
	target_compile_definitions(lua PUBLIC -DLUA_USE_LINUX)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	target_compile_definitions(lua PUBLIC -DLUA_USE_MACOSX)
elseif(CMAKE_SYSTEM_NAME MATCHES "SunOS")
	target_compile_definitions(lua PUBLIC -DLUA_USE_POSIX -DLUA_USE_DLOPEN -D_REENTRANT)
	target_link_libraries(lua dl)
endif()

if(BUILD_SHARED_LIBS)
	install(TARGETS lua
		RUNTIME DESTINATION ./
		LIBRARY DESTINATION ./)
endif()
