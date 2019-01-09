cmake_minimum_required(VERSION 2.6)
PROJECT(XMAKE_TEST_FRAMEWORK1)

# Search paths
xmake_import_search_paths(DYNAMIC_LIB2)
xmake_import_search_paths(STATIC_LIB1)

# Declaration of the framework
set(SRCS framework1.cpp)
set(HEADERS framework1.h)

if (APPLE)
    xmake_create_framework(FRAMEWORK1 framework1 1.0.0 1.0.0 ${SRCS} ${HEADERS} framework1.mm)
    set_target_properties(framework1 PROPERTIES PUBLIC_HEADER  ${HEADERS})
    target_link_libraries(framework1 "-framework Cocoa" "-framework Carbon")
else()
    xmake_create_dynamic_library(FRAMEWORK1 framework1 1.0.0 1.0.0 ${SRCS})
endif()

xmake_add_to_list_property(FRAMEWORK1 COMPILE_DEFINITIONS "FRAMEWORK1_INTERNAL")

# Link used libraries
xmake_project_link(FRAMEWORK1 DYNAMIC_LIB2)
xmake_project_link(FRAMEWORK1 STATIC_LIB1)

# Exportations
xmake_export_link_flags(FRAMEWORK1 "-framework Cocoa" "-framework Carbon")
xmake_export_compile_definitions(FRAMEWORK1 "FRAMEWORK1_IMPORT")
