set(EXPORT_INCLUDES "")
set(EXPORT_LIBS "")

add_subdirectory(Box2D)
add_subdirectory(lua)

set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "GLFW" )
set( GLFW_BUILD_TESTS OFF CACHE BOOL "GLFW" )
set( GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW" )

IF(NOT IOS)
add_subdirectory(GLFW)
add_subdirectory(glad)
ENDIF(NOT IOS)


# On iOS, OpenAL comes preinstalled
if (NOT IOS)
include(${CMAKE_SOURCE_DIR}/Scripts/CMake/Modules/FindOpenAL.cmake)
if(NOT OPENAL_FOUND)
message("++ OpenAL not found - Building Source")
add_subdirectory(OpenAL)
else()
message("++ OpenAL found")
endif()
endif()

#export includes and libs
SET_PROPERTY(GLOBAL PROPERTY ENGINE_INCLUDES ${EXPORT_INCLUDES})
SET_PROPERTY(GLOBAL PROPERTY ENGINE_LIBS ${EXPORT_LIBS})
