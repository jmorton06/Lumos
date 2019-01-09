cmake_minimum_required(VERSION 2.6)
PROJECT(XMAKE_TEST_DYNAMIC_LIB2)

# Declaration of the library
set(SRCS dynamic_lib2.cpp)

xmake_create_dynamic_library(DYNAMIC_LIB2 dynamic_lib2 1.0.0 1.0.0 ${SRCS})
xmake_add_to_list_property(DYNAMIC_LIB2 COMPILE_DEFINITIONS "DYNAMIC_LIB2_INTERNAL")

xmake_disable_vs_warning(DYNAMIC_LIB2 4305 NO)
xmake_disable_vs_warning(DYNAMIC_LIB2 4661 YES)

# Exportations
xmake_export_include_paths(DYNAMIC_LIB2 "${XMAKE_TEST_DYNAMIC_LIB2_SOURCE_DIR}")
xmake_export_compile_definitions(DYNAMIC_LIB2 "DYNAMIC_LIB2_IMPORT")
