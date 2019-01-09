cmake_minimum_required(VERSION 2.6)
PROJECT(XMAKE_TEST_EXECUTABLE3)

# Search paths
xmake_import_search_paths(DYNAMIC_LIB4)
xmake_import_search_paths(DYNAMIC_LIB3)

# Declaration of the executable
set(SRCS main.cpp)

xmake_create_executable(EXECUTABLE3 executable3 ${SRCS})
xmake_add_to_list_property(EXECUTABLE3 COMPILE_DEFINITIONS "EXECUTABLE3_INTERNAL")

# Link used libraries
xmake_project_link(EXECUTABLE3 DYNAMIC_LIB4 DYNAMIC_LIB3)
