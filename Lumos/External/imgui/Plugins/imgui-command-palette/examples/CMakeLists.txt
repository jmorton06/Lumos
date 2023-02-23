cmake_minimum_required(VERSION 3.12)
project(imgui-command-palette)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(MAIN_SRC_DIR ${CMAKE_SOURCE_DIR}/..)
add_library(imcmd
    ${MAIN_SRC_DIR}/imcmd_command_palette.h
    ${MAIN_SRC_DIR}/imcmd_command_palette.cpp
    ${MAIN_SRC_DIR}/imcmd_fuzzy_search.h
    ${MAIN_SRC_DIR}/imcmd_fuzzy_search.cpp
)

add_subdirectory(application)
