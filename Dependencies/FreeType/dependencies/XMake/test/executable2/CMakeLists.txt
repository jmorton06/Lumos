cmake_minimum_required(VERSION 2.6)
PROJECT(XMAKE_TEST_EXECUTABLE2)

# Search paths
xmake_import_search_paths(DYNAMIC_LIB1)

# Declaration of the executable
set(SRCS main.cpp)

xmake_create_executable(EXECUTABLE2 executable2 ${SRCS})
xmake_add_to_list_property(EXECUTABLE2 COMPILE_DEFINITIONS "EXECUTABLE2_INTERNAL")

# Link used libraries
xmake_project_link(EXECUTABLE2 DYNAMIC_LIB1)
