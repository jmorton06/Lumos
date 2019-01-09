cmake_minimum_required(VERSION 2.6)
PROJECT(XMAKE_TEST_DYNAMIC_LIB3)

# Search paths
xmake_import_search_paths(DYNAMIC_LIB2)

# Declaration of the library
set(SRCS dynamic_lib3.cpp)

xmake_create_dynamic_library(DYNAMIC_LIB3 dynamic_lib3 1.0.0 1.0.0 ${SRCS})
xmake_add_to_list_property(DYNAMIC_LIB3 COMPILE_DEFINITIONS "DYNAMIC_LIB3_INTERNAL")

# Link used libraries
xmake_project_link(DYNAMIC_LIB3 DYNAMIC_LIB2)

# Exportations
xmake_export_include_paths(DYNAMIC_LIB3 "${XMAKE_TEST_DYNAMIC_LIB3_SOURCE_DIR}")
xmake_export_compile_definitions(DYNAMIC_LIB3 "DYNAMIC_LIB3_IMPORT")
