cmake_minimum_required(VERSION 2.6)
PROJECT(XMAKE_TEST_STATIC_LIB1)


# Declaration of the library
set(SRCS static_lib1.cpp)

xmake_create_static_library(STATIC_LIB1 static_lib1 ${SRCS})
xmake_add_to_list_property(STATIC_LIB1 COMPILE_DEFINITIONS "STATIC_LIB1_INTERNAL")

xmake_add_to_list_property(STATIC_LIB1 COMPILE_DEFINITIONS "STATIC_LIB1_UNUSED")
xmake_add_to_list_property(STATIC_LIB1 COMPILE_DEFINITIONS "STATIC_LIB1_UNUSED")

# Exportations
xmake_export_include_paths(STATIC_LIB1 "${XMAKE_TEST_STATIC_LIB1_SOURCE_DIR}")
xmake_export_compile_definitions(STATIC_LIB1 "STATIC_LIB1_IMPORT")

xmake_export_compile_definitions(STATIC_LIB1 "STATIC_LIB1_UNUSED")
xmake_export_compile_definitions(STATIC_LIB1 "STATIC_LIB1_UNUSED")
