cmake_minimum_required(VERSION 2.6)
PROJECT(XMAKE_TEST_EXECUTABLE1)

# Search paths
xmake_import_search_paths(STATIC_LIB2)

# Declaration of the executable
set(SRCS main.cpp)

xmake_create_executable(EXECUTABLE1 executable1 ${SRCS})
xmake_add_to_list_property(EXECUTABLE1 COMPILE_DEFINITIONS "EXECUTABLE1_INTERNAL")

# Link used libraries
xmake_project_link(EXECUTABLE1 STATIC_LIB2)
