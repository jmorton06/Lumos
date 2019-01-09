cmake_minimum_required(VERSION 2.6)
PROJECT(XMAKE_TEST_STATIC_LIB2)

# Search paths
xmake_import_search_paths(STATIC_LIB1)

# Declaration of the library
set(SRCS static_lib2.cpp)

xmake_create_static_library(STATIC_LIB2 static_lib2 ${SRCS})
xmake_add_to_list_property(STATIC_LIB2 COMPILE_DEFINITIONS "STATIC_LIB2_INTERNAL")

# Link used libraries
xmake_project_link(STATIC_LIB2 STATIC_LIB1)

# Exportations
xmake_export_include_paths(STATIC_LIB2 "${XMAKE_TEST_STATIC_LIB2_SOURCE_DIR}")
xmake_export_compile_definitions(STATIC_LIB2 "STATIC_LIB2_IMPORT")
