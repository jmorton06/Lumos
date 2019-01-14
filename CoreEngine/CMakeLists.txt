cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

project (CoreEngine)

set(sampleName CoreEngine)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS FALSE)
set(BUILD_SHARED_LIBS TRUE)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(LIBRARY_OUTPUT_PATH  ${CMAKE_SOURCE_DIR}/bin)

option(API_VULKAN "Use Vulkan" ON)
option(API_DIRECT3D "Use Direct3D" OFF)
option(API_OPENGL "Use OpenGL" ON)
option(JMCORESHARED "JMCORE Shared Library" ON)
option(JMIMGUI "Use IMGUI" ON)

IF(IOS)
	Set(MOBILE true)
	Set(API_OPENGL OFF)
	Set(JMIMGUI OFF)
ENDIF(IOS)
	 
IF(UNIX)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fpermissive -w -g -Wall -fPIC")

	if (CMAKE_CXX_COMPILER_ID MATCHES "[cC][lL][aA][nN][gG]")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=1024")
	endif ()
ENDIF(UNIX)

add_definitions(-DJM_ENGINE -DJM_OPENAL -DFREEIMAGE_LIB -DJM_ROOT_DIR=${CMAKE_SOURCE_DIR})

if(API_OPENGL)
	add_definitions(-DJM_RENDER_API_OPENGL)
endif(API_OPENGL)

if(WIN32)
	add_definitions(-DJM_PLATFORM_WINDOWS -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS -D_DISABLE_EXTENDED_ALIGNED_STORAGE)

	IF(API_DIRECT3D)
		target_link_libraries(CoreEngine D3DCompiler.lib d3d11.lib ws2_32.lib winmm.lib)
		add_definitions(-DJM_RENDER_API_DIRECT3D)
	ENDIF(API_DIRECT3D)
endif(WIN32)

if (IOS)
    add_definitions(-DJM_PLATFORM_IOS -DJM_PLATFORM_MOBILE)
endif(IOS)

if(UNIX AND NOT MOBILE)
	if(APPLE)
		add_definitions(-DJM_PLATFORM_MACOS)
	else()
		add_definitions(-DJM_PLATFORM_LINUX)
	endif(APPLE)
endif(UNIX AND NOT MOBILE)

set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -DJM_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DJM_RELEASE")

set(JM_SOURCES)
set(JM_PLATFORM_SOURCES)
set(JM_RENDER_DEVICE_SOURCES)

file(GLOB JM_SOURCES
  		${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
  		${CMAKE_CURRENT_SOURCE_DIR}/src/App/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/AI/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/AI/*.cpp
  		${CMAKE_CURRENT_SOURCE_DIR}/src/Audio/*.h

  		${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Model/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/API/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/API/Textures/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Camera/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Particles/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Renderers/*.h

  		${CMAKE_CURRENT_SOURCE_DIR}/src/Input/*.h
  		${CMAKE_CURRENT_SOURCE_DIR}/src/Maths/*.h
  		${CMAKE_CURRENT_SOURCE_DIR}/src/Physics/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/Physics/JMPhysicsEngine/*.h
	 	${CMAKE_CURRENT_SOURCE_DIR}/src/Physics/B2PhysicsEngine/*.h

  		${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/*.h
  		${CMAKE_CURRENT_SOURCE_DIR}/src/Entity/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/Entity/Component/*.h
		   
		${CMAKE_CURRENT_SOURCE_DIR}/src/Events/*.h

  		${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/Scripting/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/Scripting/Bindings/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/Scripting/Bindings/CameraBindings/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/Scripting/Bindings/MathsBindings/*.h
  		${CMAKE_CURRENT_SOURCE_DIR}/src/System/*.h
  		${CMAKE_CURRENT_SOURCE_DIR}/src/Utilities/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/AI/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/Network/*.h

		${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  		${CMAKE_CURRENT_SOURCE_DIR}/src/App/*.cpp
  		${CMAKE_CURRENT_SOURCE_DIR}/src/Audio/*.cpp
  		${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Model/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/API/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/API/Textures/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Camera/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Particles/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Renderers/*.cpp

  		${CMAKE_CURRENT_SOURCE_DIR}/src/Input/*.cpp
  		${CMAKE_CURRENT_SOURCE_DIR}/src/Maths/*.cpp
  		${CMAKE_CURRENT_SOURCE_DIR}/src/Physics/*.cpp
	 	${CMAKE_CURRENT_SOURCE_DIR}/src/Physics/JMPhysicsEngine/*.cpp
	 	${CMAKE_CURRENT_SOURCE_DIR}/src/Physics/B2PhysicsEngine/*.cpp

  		${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/*.cpp

  		${CMAKE_CURRENT_SOURCE_DIR}/src/Entity/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/Entity/Component/*.cpp
		   
		${CMAKE_CURRENT_SOURCE_DIR}/src/Events/*.cpp

  		${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/*.cpp
  		${CMAKE_CURRENT_SOURCE_DIR}/src/Scripting/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/Scripting/Bindings/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/Scripting/Bindings/MathsBindings/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/Scripting/Bindings/CameraBindings/*.cpp
  		${CMAKE_CURRENT_SOURCE_DIR}/src/System/*.cpp
  		${CMAKE_CURRENT_SOURCE_DIR}/src/Utilities/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/AI/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/Network/*.cpp

		${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui.h
        ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imconfig.h
		${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_demo.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_draw.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_internal.h
		${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imstb_rectpack.h
		${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imstb_textedit.h
		${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imstb_truetype.h

		${CMAKE_CURRENT_SOURCE_DIR}/external/tinygltf/json.hpp
		${CMAKE_CURRENT_SOURCE_DIR}/external/tinygltf/stb_image_write.h
		${CMAKE_CURRENT_SOURCE_DIR}/external/tinygltf/stb_image.h
		${CMAKE_CURRENT_SOURCE_DIR}/external/tinygltf/tiny_gltf.h
		${CMAKE_CURRENT_SOURCE_DIR}/external/stb/stb_vorbis.c

		${CMAKE_CURRENT_SOURCE_DIR}/external/simplex/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/external/simplex/*.cpp
)

IF(WIN32)
	file(GLOB_RECURSE JM_PLATFORM_WINDOWS_PLATFORM_SOURCE
		${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Windows/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Windows/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/GLFW/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/GLFW/*.cpp
	)
	list(APPEND JM_PLATFORM_SOURCES ${JM_PLATFORM_WINDOWS_PLATFORM_SOURCE})
	IF(API_DIRECT3D)
		file(GLOB_RECURSE JM_RENDER_API_DIRECT3D_SOURCE
		${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/DirectX/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/DirectX/*.cpp
        )
		list(APPEND JM_RENDER_DEVICE_SOURCES ${JM_RENDER_API_DIRECT3D_SOURCE})
	ENDIF(API_DIRECT3D)
ENDIF(WIN32)

IF(API_VULKAN)
	file(GLOB_RECURSE VULKAN_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/GraphicsAPI/Vulkan/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/GraphicsAPI/Vulkan/*.cpp
	)

	IF(APPLE AND NOT IOS)
	set(VULKAN_SOURCES ${VULKAN_SOURCES}
		${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/GraphicsAPI/Vulkan/MakeMetalView.mm
	)
	ENDIF()
	list(APPEND JM_RENDER_DEVICE_SOURCES ${VULKAN_SOURCES})
ENDIF(API_VULKAN)

IF(API_OPENGL)
file(GLOB_RECURSE OPENGL_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/GraphicsAPI/OpenGL/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/GraphicsAPI/OpenGL/Textures/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/GraphicsAPI/OpenGL/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/GraphicsAPI/OpenGL/Textures/*.cpp
    )
	list(APPEND JM_RENDER_DEVICE_SOURCES ${OPENGL_SOURCES})
ENDIF(API_OPENGL)

IF(UNIX)
	file(GLOB_RECURSE UNIX_PLATFORM_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Unix/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Unix/*.cpp
    )
list(APPEND JM_PLATFORM_SOURCES ${UNIX_PLATFORM_SOURCES})
ENDIF(UNIX)

IF(MOBILE)
	file(GLOB_RECURSE MOBILE_PLATFORM_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/GLFM/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/GLFM/*.cpp
	)
	list(APPEND JM_PLATFORM_SOURCES ${MOBILE_PLATFORM_SOURCES})
ELSE()
	file(GLOB_RECURSE DESKTOP_PLATFORM_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/GLFW/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/GLFW/*.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/GraphicsAPI/OpenGL/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/GraphicsAPI/OpenGL/Textures/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/GraphicsAPI/OpenGL/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/GraphicsAPI/OpenGL/Textures/*.cpp
    )
	list(APPEND JM_PLATFORM_SOURCES ${DESKTOP_PLATFORM_SOURCES})
ENDIF(MOBILE)

set(JM_IMGUI_SOURCES)

IF(JMIMGUI)
add_definitions(-DJM_IMGUI)
IF(API_VULKAN)
	set(JM_IMGUI_SOURCES ${JM_IMGUI_SOURCES} 
	${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/examples/imgui_impl_vulkan.h
	${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/examples/imgui_impl_vulkan.cpp
	)
ENDIF(API_VULKAN)
IF(API_OPENGL)
	set(JM_IMGUI_SOURCES ${JM_IMGUI_SOURCES} 
	${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/examples/imgui_impl_opengl3.h
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/examples/imgui_impl_opengl3.cpp
	)
ENDIF(API_OPENGL)
IF(NOT IOS)
	set(JM_IMGUI_SOURCES ${JM_IMGUI_SOURCES} 
	${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/examples/imgui_impl_glfw.h
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/examples/imgui_impl_glfw.cpp
	)
ENDIF(NOT IOS)
ENDIF(JMIMGUI)

if(MOBILE)
set(JMCORESHARED OFF)
endif(MOBILE)

if(JMCORESHARED)
add_library (CoreEngine SHARED ${JM_SOURCES} ${JM_PLATFORM_SOURCES} ${JM_RENDER_DEVICE_SOURCES} ${JM_IMGUI_SOURCES})
add_definitions(-DJM_DYNAMIC)
else()
add_library (CoreEngine STATIC ${JM_SOURCES} ${JM_PLATFORM_SOURCES} ${JM_RENDER_DEVICE_SOURCES} ${JM_IMGUI_SOURCES})
add_definitions(-DJM_STATIC )
endif(JMCORESHARED)

install (FILES ${JM_SOURCE} DESTINATION include/CoreEngine)

IF(API_VULKAN)

	message(STATUS "Using module to find Vulkan")
	find_package(Vulkan)

	IF (NOT Vulkan_FOUND)

		MESSAGE("Using bundled Vulkan library version")

		IF(WIN32)
			if(CMAKE_SIZEOF_VOID_P EQUAL 8)
				# 64 bits
				find_library(Vulkan_LIBRARY NAMES vulkan-1.lib PATHS ${CMAKE_SOURCE_DIR}/Dependencies/vulkan/libs/windows/64bit)
			elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
				# 32 bits
				find_library(Vulkan_LIBRARY NAMES vulkan-1.lib PATHS ${CMAKE_SOURCE_DIR}/Dependencies/vulkan/libs/windows/32bit)
			endif()

			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR -DJM_RENDER_API_VULKAN")
		ENDIF(WIN32)

		IF(APPLE)
			if(IOS)
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DJM_RENDER_API_VULKAN")
				set(Vulkan_LIBRARY ${CMAKE_SOURCE_DIR}/Dependencies/vulkan/libs/iOS/libMoltenVK.dylib)
			else()
				find_library(METAL_LIBRARY Metal)
				find_library(QUARTZCORE_LIBRARY QuartzCore)
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DJM_RENDER_API_VULKAN")
				find_library(Vulkan_LIBRARY NAMES libMoltenVK.dylib HINTS "${CMAKE_SOURCE_DIR}/Dependencies/vulkan/libs/macOS" REQUIRED)
			endif(IOS)
		ELSEIF(UNIX)
				find_library(Vulkan_LIBRARY NAMES libvulkan.so.1 HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/Dependencies/vulkan/libs/linux" REQUIRED)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DJM_RENDER_API_VULKAN -DVK_USE_PLATFORM_XCB_KHR")
		ENDIF(APPLE)

	ELSE()
	MESSAGE("Vulkan Found!")

		IF(WIN32)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR -DJM_RENDER_API_VULKAN")
		ENDIF(WIN32)

		IF(APPLE)
			if(IOS)
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DJM_RENDER_API_VULKAN -DVK_USE_PLATFORM_IOS_MVK")
			else()
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DJM_RENDER_API_VULKAN -DVK_USE_PLATFORM_MACOS_MVK")
			endif(IOS)
		ELSEIF(UNIX)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DJM_RENDER_API_VULKAN -DVK_USE_PLATFORM_XCB_KHR")
		ENDIF(APPLE)
	ENDIF()

	IF (Vulkan_LIBRARY)
		set(Vulkan_FOUND ON)
	ENDIF()

	IF (NOT Vulkan_FOUND)
		message(FATAL_ERROR "Could not find Vulkan library!")
	ELSE()
		message(STATUS "Vulkan lib : " ${Vulkan_LIBRARY})
	ENDIF()
ENDIF(API_VULKAN)
 
if(USEASSIMP)
	add_definitions(-DASSIMP)
	target_link_libraries(CoreEngine assimp ${ASSIMP_LIBRARIES})
endif(USEASSIMP)

if(USEOGGVORBIS)
	add_definitions(-DOGGVORBIS)
	target_link_libraries(CoreEngine ogg 	${OGG_LIBRARIES})
	target_link_libraries(CoreEngine vorbis ${VORBIS_LIBRARIES})
endif(USEOGGVORBIS)

if(USEFREEIMAGE)
	add_definitions(-DFREEIMAGE)
	target_link_libraries(CoreEngine FreeImage  ${FREEIMAGE_LIBRARIES})
endif(USEFREEIMAGE)

target_link_libraries(CoreEngine freetype 	${FREETYPE_LIBRARIES})
target_link_libraries(CoreEngine box2d 		${BOX2D_LIBRARIES})
target_link_libraries(CoreEngine lua 		${LUA_LIBRARIES})

IF(API_VULKAN)
	target_link_libraries(CoreEngine ${Vulkan_LIBRARY})

	if(APPLE)
	link_libraries(CoreEngine ${METAL_LIBRARY})
	link_libraries(CoreEngine ${QUARTZCORE_LIBRARY})
	target_link_libraries(CoreEngine "-framework QuartzCore -framework Metal -framework CoreFoundation -framework Cocoa")

	endif()
ENDIF(API_VULKAN)

IF(SSE4_1_FOUND)
	add_definitions(-DJM_SSE)
ENDIF(SSE4_1_FOUND)

IF(IOS)
    target_link_libraries(CoreEngine "-framework OpenAL")
ELSE()
	IF(OPENAL_LIBRARY)
	target_link_libraries(CoreEngine ${OPENAL_LIBRARY})
	ELSE()
	target_link_libraries(CoreEngine OpenAL ${OPENAL_LIBRARIES})
	ENDIF()
ENDIF(IOS)

IF(NOT MOBILE)
	target_link_libraries(CoreEngine glfw ${GLFW_LIBRARIES})
	IF(API_OPENGL)
		target_link_libraries(CoreEngine glad ${GLAD_LIBRARIES})
	ENDIF(API_OPENGL)
ENDIF(NOT MOBILE)

include_directories(
	"../Dependencies/lua/src/"
	"../Dependencies/GLFW/include/"
	"../Dependencies/glad/include/"
	"../Dependencies/OpenAL/include/"
	"../Dependencies/FreeType/include/"
	"external/stb/"
	"external/"
	"../Dependencies/Box2D/"
	"../Dependencies/vulkan/"
	"../Dependencies/"
	"../"
	"src/"
	"."
	)

if(USECOTIRE)
set_target_properties(CoreEngine PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "src/JM.h")
cotire(CoreEngine)
endif(USECOTIRE)