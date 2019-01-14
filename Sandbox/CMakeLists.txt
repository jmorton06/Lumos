cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

project (Sandbox)

if (CMAKE_CXX_COMPILER_ID MATCHES "[cC][lL][aA][nN][gG]")
       set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=1024 -pthread")
endif ()

# Main example
file(GLOB TARGET_INC
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h 
  ${CMAKE_CURRENT_SOURCE_DIR}/Scenes/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Scenes/CubeGame/*.h
)

file(GLOB TARGET_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Scenes/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Scenes/CubeGame/*.cpp
)

include_directories(
	"../Lumos/src/"
	"../Lumos/"
	"../Dependencies/lua/src/"
	"../Dependencies/OpenAL/include/"
	"../Dependencies/FreeType/include/"
	"../Dependencies/Box2D/"
	"/"
	"../Dependencies/GLFW/include/"
	"../Dependencies/vulkan/"
	"../Dependencies/"
	"../"
	"../Lumos/external/"
	"../Lumos/external/stb/"
	)

if(USEOGGVORBIS)
add_definitions(-DOGGVORBIS)
endif(USEOGGVORBIS)

IF(SSE4_1_FOUND)
	add_definitions(-DJM_SSE)
ENDIF(SSE4_1_FOUND)

if(API_VULKAN)
add_definitions(-DJM_RENDER_API_VULKAN)
endif(API_VULKAN)

add_definitions(-DFREEIMAGE_LIB -DJM_OPENAL -D_CRT_SECURE_NO_WARNINGS -DJM_ROOT_DIR=${CMAKE_SOURCE_DIR})

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DJM_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DJM_RELEASE")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

SET(sampleName Sandbox)

if(WIN32)
	add_definitions(-DJM_PLATFORM_WINDOWS -DWIN32_LEAN_AND_MEAN -DJM_RENDER_API_OPENGL -D_DISABLE_EXTENDED_ALIGNED_STORAGE)

	IF(DIRECT3D)
		add_definitions(-DJM_RENDER_API_DIRECT3D)
	ENDIF(DIRECT3D)
endif(WIN32)

if(UNIX AND NOT IOS)
if(APPLE)
	add_definitions(-DJM_PLATFORM_MACOS -DJM_RENDER_API_OPENGL)
else()
	add_definitions(-DJM_PLATFORM_LINUX -DJM_RENDER_API_OPENGL)
endif(APPLE)
endif(UNIX AND NOT IOS)

if(JMCORESHARED)
add_definitions(-DJM_DYNAMIC)
else()
add_definitions(-DJM_STATIC)
endif(JMCORESHARED)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
set(BUILD_SHARED_LIBS TRUE)

SET(TARGET_SRC ${TARGET_SRC} ${TARGET_INC})

add_executable (Sandbox ${TARGET_SRC})

if (IOS)
    add_definitions(-DJM_PLATFORM_IOS -DJM_PLATFORM_MOBILE -DJM_RENDER_API_VULKAN)
    target_link_libraries(Sandbox "-framework Foundation -framework CoreGraphics -framework UIKit -framework OpenGLES -framework QuartzCore -framework OpenAL")
endif(IOS)

target_link_libraries(Sandbox Lumos)
