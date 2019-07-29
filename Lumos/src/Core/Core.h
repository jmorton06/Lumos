#pragma once

#ifndef LUMOS_PLATFORM_WINDOWS
#include <signal.h>
#endif

#include <stdint.h>

#ifdef LUMOS_PLATFORM_WINDOWS

#define MEM_ALIGNMENT 16
#define MEM_ALIGN __declspec(align(MEM_ALIGNMENT))

#else

#define MEM_ALIGNMENT 16
#define MEM_ALIGN  __attribute__((aligned(MEM_ALIGNMENT)))

#endif

#ifdef LUMOS_PLATFORM_WINDOWS
#pragma warning (disable:4251)
#ifdef LUMOS_DYNAMIC
#ifdef LUMOS_ENGINE
#define LUMOS_EXPORT __declspec(dllexport)
#else
#define LUMOS_EXPORT __declspec(dllimport)
#endif
#else
#define LUMOS_EXPORT
#endif
#define LUMOS_HIDDEN
#else
#define LUMOS_EXPORT __attribute__ ((visibility ("default")))
#define LUMOS_HIDDEN __attribute__ ((visibility ("hidden")))
#endif

#define IMGUI_API LUMOS_EXPORT

#define BIT(x) (1 << x)

#define DEBUGDRAW_FLAGS_CONSTRAINT			1
#define DEBUGDRAW_FLAGS_MANIFOLD			2
#define DEBUGDRAW_FLAGS_COLLISIONVOLUMES	4
#define DEBUGDRAW_FLAGS_COLLISIONNORMALS	8
#define DEBUGDRAW_FLAGS_AABB				16
#define DEBUGDRAW_FLAGS_LINEARVELOCITY		32
#define DEBUGDRAW_FLAGS_LINEARFORCE			64
#define DEBUGDRAW_FLAGS_BROADPHASE			128
#define DEBUGDRAW_FLAGS_BROADPHASE_PAIRS	256
#define DEBUGDRAW_FLAGS_BOUNDING_RADIUS		512
#define DEBUGDRAW_FLAGS_ENTITY_COMPONENTS	1024

#define NUMBONES 64
#define INPUT_BUF_SIZE 128

#define SAFE_DELETE(mem) { if(mem) { delete mem; mem = NULL; } }
#define SAFE_UNLOAD(mem, ...) { if(mem) { mem->Unload(__VA_ARGS__); delete mem; mem = NULL; } }

#ifdef LUMOS_DEBUG
#define LUMOS_DEBUG_METHOD(x) x;
#define LUMOS_DEBUG_METHOD_CALL(x) x;
#else
#define LUMOS_DEBUG_METHOD(x) x {}
#define LUMOS_DEBUG_METHOD_CALL(x);
#endif

#define MAX_OBJECTS 2048

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)
#define ROOT_DIR STRINGIZE(LUMOS_ROOT_DIR)

#if LUMOS_PLATFORM_WINDOWS
#define LUMOS_BREAK() __debugbreak(); 
#else
#define LUMOS_BREAK() raise(SIGTRAP);
#endif

#ifdef LUMOS_DEBUG
	#define LUMOS_ENABLE_ASSERTS
#endif

#ifdef LUMOS_ENABLE_ASSERTS
	#define LUMOS_ASSERT(x, ...)                               		\
	{                                                       		\
		if (!(x))                                           		\
		{                                                   		\
			LUMOS_CORE_ERROR("Assertion failed: {0}", __VA_ARGS__); \
			LUMOS_BREAK();                                 			\
		}                                                   		\
	}
	#define LUMOS_CORE_ASSERT(x, ...)                               \
	{                                                            	\
		if (!(x))                                                	\
		{                                                        	\
			LUMOS_CORE_ERROR("Assertion failed: {0}", __VA_ARGS__); \
			LUMOS_BREAK();                                     		\
		}                                                        	\
	}
#else
	#define LUMOS_ASSERT(x, ...)
	#define LUMOS_CORE_ASSERT(x, ...)
#endif

#define UNIMPLEMENTED	 													\
	{																		\
		LUMOS_CORE_ERROR("Unimplemented : {0} : {1}", __FILE__, __LINE__); 	\
		LUMOS_BREAK();  													\
	}																		\
