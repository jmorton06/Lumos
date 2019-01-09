cmake_minimum_required(VERSION 2.6)
PROJECT(XMAKE_TEST_EXECUTABLE4)

# Search paths
xmake_import_search_paths(FRAMEWORK1)

# Declaration of the executable
set(SRCS main.cpp)

xmake_create_executable(EXECUTABLE4 executable4 ${SRCS})
xmake_add_to_list_property(EXECUTABLE4 COMPILE_DEFINITIONS "EXECUTABLE4_INTERNAL")

# Link used libraries
xmake_project_link(EXECUTABLE4 FRAMEWORK1)
