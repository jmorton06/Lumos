cmake_minimum_required(VERSION 2.6)
PROJECT(XMAKE_TEST)

set(CMAKE_MODULE_PATH "${XMAKE_TEST_SOURCE_DIR}/..")
include(XMake)

message(STATUS "XMAKE_VERSION:          ${XMAKE_VERSION}")
message(STATUS "XMAKE_BINARY_DIR:       ${XMAKE_BINARY_DIR}")
message(STATUS "XMAKE_DEPENDENCIES_DIR: ${XMAKE_DEPENDENCIES_DIR}")


xmake_add_subdirectory(static_lib1)
xmake_add_subdirectory(static_lib2)
xmake_add_subdirectory(dynamic_lib1)
xmake_add_subdirectory(dynamic_lib2)
xmake_add_subdirectory(dynamic_lib3)
xmake_add_subdirectory(dynamic_lib4)
xmake_add_subdirectory(executable1)
xmake_add_subdirectory(executable2)
xmake_add_subdirectory(executable3)
xmake_add_subdirectory(empty)

if (NOT WIN32)
    xmake_add_subdirectory(framework1)
    xmake_add_subdirectory(executable4)
endif()


xmake_display(STATIC_LIB1)
xmake_display(STATIC_LIB2)
xmake_display(DYNAMIC_LIB1)
xmake_display(DYNAMIC_LIB2)
xmake_display(DYNAMIC_LIB3)
xmake_display(DYNAMIC_LIB4)

if (NOT WIN32)
    xmake_display(FRAMEWORK1)
endif()

xmake_display(EXECUTABLE1)
xmake_display(EXECUTABLE2)
xmake_display(EXECUTABLE3)

if (NOT WIN32)
    xmake_display(EXECUTABLE4)
endif()


# Tests
if (NOT "${XMAKE_STATIC_LIB1_COMPILE_DEFINITIONS}" STREQUAL "STATIC_LIB1_IMPORT;STATIC_LIB1_UNUSED")
    message(FATAL_ERROR "For XMAKE_STATIC_LIB1_COMPILE_DEFINITIONS expected: 'STATIC_LIB1_IMPORT;STATIC_LIB1_UNUSED', got: '${XMAKE_STATIC_LIB1_COMPILE_DEFINITIONS}'")
endif()

get_target_property(PROPERTY_VALUE ${XMAKE_STATIC_LIB1_TARGET} COMPILE_DEFINITIONS)
if (NOT "${PROPERTY_VALUE}" STREQUAL "STATIC_LIB1_INTERNAL;STATIC_LIB1_UNUSED")
    message(FATAL_ERROR "For property COMPILE_DEFINITIONS of STATIC_LIB1 expected: 'STATIC_LIB1_INTERNAL;STATIC_LIB1_UNUSED', got: '${PROPERTY_VALUE}'")
endif()
