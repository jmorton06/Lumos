cmake_minimum_required(VERSION 2.8)
project(glad)

####################################
#            Directories           #
####################################

include_directories(${PROJECT_SOURCE_DIR}/include)

IF(UNIX)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -w -g -Wall -fPIC -std=c++1y -Wl,--no-undefined")
ENDIF(UNIX)

set(SOURCE_FILES
        ${PROJECT_SOURCE_DIR}/include/glad/glad.h
        ${PROJECT_SOURCE_DIR}/include/KHR/khrplatform.h
        ${PROJECT_SOURCE_DIR}/src/glad.c)

IF(UNIX)
add_library(glad SHARED ${SOURCE_FILES})
ELSE(UNIX)
add_library(glad STATIC ${SOURCE_FILES})
ENDIF(UNIX)
#SET(LIBRARY_OUTPUT_PATH  Bin)