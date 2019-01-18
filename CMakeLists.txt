cmake_minimum_required(VERSION 3.7)

if(WIN32)
		message("++ Building for windows")
elseif(ANDROID)
		message("++ Building for Android")
elseif(UNIX AND NOT APPLE)
		message("++ Building for Linux")
elseif(APPLE)
	IF(IOS)
		message("++ Building for iOS")
		set(CMAKE_MACOSX_RPATH 1)
	ELSE(IOS)
		message("++ Building for MacOS")
		set(CMAKE_MACOSX_RPATH 1)
	ENDIF(IOS)
else()
		message(FATAL_ERROR "Unknown system")
endif()

project (LumosEngine)
enable_language(CXX)
enable_language(C)

message ("CMAKE_CXX_COMPILER_ID:" ${CMAKE_CXX_COMPILER_ID})

set(CMAKE_CXX_STANDARD 17)
set(BUILD_SHARED_LIBS TRUE)

set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/Scripts/CMake/Modules")

INCLUDE(FindSSE)
FindSSE()
IF(SSE4_1_FOUND)
    SET(CXX_DFLAGS -msse4.1)
    ADD_DEFINITIONS(${CXX_DFLAGS})
    MESSAGE("SSE 4.1 FOUND")
ENDIF(SSE4_1_FOUND)

set( USECOTIRE OFF CACHE BOOL "USE COTIRE (PCH)" )

if(USECOTIRE)
include(cotire)
endif(USECOTIRE)

if (CMAKE_CXX_COMPILER_ID MATCHES "[cC][lL][aA][nN][gG]")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -stdlib=libc++")
	message("++ Set Clang c++ lib")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "[gG][nN][uU]")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
	message("++ Set GCC c++ version")
endif ()

if (APPLE)
    cmake_policy(SET CMP0042 NEW)

    if(CMAKE_VERSION VERSION_LESS 3.0.0)
        include(CMakeForceCompiler)
        CMAKE_FORCE_C_COMPILER(clang GNU)
        CMAKE_FORCE_CXX_COMPILER(clang++ GNU)
    endif()
    SET(CMAKE_SIZEOF_VOID_P 4)
    set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++17")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")

    # otherwise apple defines a macro named check that conflicts with boost
    add_definitions(-D__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=0)

    if(NOT IOS)
        if (NOT XMAKE_OSX_ARCHITECTURES)
            set(XMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "" FORCE)
	        message("Setting architecture to x86_64")
        endif()
    else()
        set(CMAKE_EXE_LINKER_FLAGS "-framework OpenGLES -framework Foundation -framework CoreGraphics -framework QuartzCore -framework UIKit")
        set(XCODE_ATTRIBUTE_GCC_UNROLL_LOOPS "YES")
        set(XCODE_ATTRIBUTE_LLVM_VECTORIZE_LOOPS "YES")
        set(XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer")
        set(XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER "YES")
        set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourcompany.\${PRODUCT_NAME:rfc1034identifier}")

        string (REPLACE "-msse" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    endif(NOT IOS)
endif ()

if(MSVC)
	# visual studio compile flags
	set(CMAKE_CXX_FLAGS_DEBUG "/W3 /DDEBUG_BUILD /MDd /Zi /Od /RTC1 /GR- /MP")
    set(CMAKE_CXX_FLAGS_RELEASE "/W3 /MD /O2 /GR- /MP")
    set(CMAKE_CXX_FLAGS "/W3 /MDd /Od /GR- /MP /EHsc")
else()
	#MinGW and g++ compile flags
	set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -Wall -fpermissive -w -std=c++17 -pipe -fPIC ")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS} -Wall -std=c++17 -g -pipe -fPIC")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS} -Wall -std=c++17 -O3 -pipe -fPIC")

	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		add_definitions(-DDEBUG_BUILD)
	endif()
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

message ("cxx Flags:" ${CMAKE_CXX_FLAGS})
message ("c Flags:"   ${CMAKE_C_FLAGS})
message ("cxx Flags debug:" ${CMAKE_CXX_FLAGS_DEBUG})
message ("cxx Flags release:" ${CMAKE_CXX_FLAGS_RELEASE})

add_subdirectory(Dependencies)
add_subdirectory(Lumos)
add_subdirectory(Sandbox)

IF(NOT IOS)
add_dependencies(Lumos glfw)
add_dependencies(Lumos glad)

IF(OpenAL)
add_dependencies(Lumos OpenAL)
ENDIF(OpenAL)

ENDIF(NOT IOS)

add_dependencies(Lumos box2d)
add_dependencies(Lumos lua)
add_dependencies(Sandbox Lumos)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Sandbox)
