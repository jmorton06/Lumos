cmake_minimum_required(VERSION 2.6)
PROJECT(XMAKE_TEST_DYNAMIC_LIB4)

# Search paths
xmake_import_search_paths(DYNAMIC_LIB2)
xmake_import_search_paths(DYNAMIC_LIB1)

# Declaration of the library
set(SRCS dynamic_lib4.cpp)

xmake_create_dynamic_library(DYNAMIC_LIB4 dynamic_lib4 1.0.0 1.0.0 ${SRCS})
xmake_add_to_list_property(DYNAMIC_LIB4 COMPILE_DEFINITIONS "DYNAMIC_LIB4_INTERNAL")

xmake_disable_vs_warning(DYNAMIC_LIB4 4306 NO)
xmake_disable_vs_warning(DYNAMIC_LIB4 4662 YES)

# Link used libraries
xmake_project_link(DYNAMIC_LIB4 DYNAMIC_LIB1)
xmake_project_link(DYNAMIC_LIB4 DYNAMIC_LIB2)

# Exportations
xmake_export_include_paths(DYNAMIC_LIB4 "${XMAKE_TEST_DYNAMIC_LIB4_SOURCE_DIR}")
xmake_export_compile_definitions(DYNAMIC_LIB4 "DYNAMIC_LIB4_IMPORT")
