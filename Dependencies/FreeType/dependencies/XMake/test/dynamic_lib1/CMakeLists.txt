cmake_minimum_required(VERSION 2.6)
PROJECT(XMAKE_TEST_DYNAMIC_LIB1)

# Search paths
xmake_import_search_paths(STATIC_LIB1)

# Declaration of the library
set(SRCS dynamic_lib1.cpp)

xmake_create_dynamic_library(DYNAMIC_LIB1 dynamic_lib1 1.0.0 1.0.0 ${SRCS})
xmake_add_to_list_property(DYNAMIC_LIB1 COMPILE_DEFINITIONS "DYNAMIC_LIB1_INTERNAL")

# Link used libraries
xmake_project_link(DYNAMIC_LIB1 STATIC_LIB1)

# Exportations
xmake_export_include_paths(DYNAMIC_LIB1 "${XMAKE_TEST_DYNAMIC_LIB1_SOURCE_DIR}")
xmake_export_compile_definitions(DYNAMIC_LIB1 "DYNAMIC_LIB1_IMPORT")
