##########################################################################################
#
# XMake script
#
# See http://www.idiap.ch/~pabbet/articles/combining-git-submodules-and-cmake-for-library-development/
# for details.
#
#
# XMake is made available under the MIT License.
#
# Copyright (c) 2009-2012 Philip Abbet
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
##########################################################################################


#-----------------------------------------------------------------------------------------
# Utility functions
#-----------------------------------------------------------------------------------------

# Set the value of a variable
function(xmake_set VARIABLE VALUE)
    set(${VARIABLE} ${VALUE} CACHE INTERNAL "" FORCE)
endfunction()


# Append a value to the current value of a list variable (values separated by ;)
function(xmake_list_append VARIABLE VALUE)
    if (NOT VALUE)
        return()
    endif()

    set(NEW_VALUE "${${VARIABLE}}")

    if (NEW_VALUE)
        set(NEW_VALUE "${NEW_VALUE};${VALUE}")
    else()
        set(NEW_VALUE "${VALUE}")
    endif()

    list(REMOVE_DUPLICATES NEW_VALUE)

    set(${VARIABLE} "${NEW_VALUE}" CACHE INTERNAL "" FORCE)
endfunction()


# Append a value to the current value of a variable (values separated by a space)
function(xmake_append VARIABLE VALUE)
    if (NOT VALUE)
        return()
    endif()

    if (${VARIABLE})
        set(${VARIABLE} "${${VARIABLE}} ${VALUE}" CACHE INTERNAL "" FORCE)
    else()
        set(${VARIABLE} "${VALUE}" CACHE INTERNAL "" FORCE)
    endif()
endfunction()


# Add a subdirectory to the build, but not if there is no CMakeLists.txt file
# in it (which might happen when using several cascaded submodules with XMake)
function(xmake_add_subdirectory FOLDER_NAME)
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER_NAME}/CMakeLists.txt")
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER_NAME}")
    endif()
endfunction()


# Add a value to a list property of a XMake project (values separated by ;)
function(xmake_add_to_list_property PROJECT PROPERTY_NAME VALUE)
    if (NOT VALUE)
        return()
    endif()

    get_target_property(PROPERTY_VALUE ${XMAKE_${PROJECT}_TARGET} ${PROPERTY_NAME})

    if (NOT PROPERTY_VALUE STREQUAL "PROPERTY_VALUE-NOTFOUND")
        set(PROPERTY_VALUE "${PROPERTY_VALUE};${VALUE}")
    else()
        set(PROPERTY_VALUE "${VALUE}")
    endif()

    list(REMOVE_DUPLICATES PROPERTY_VALUE)

    set_target_properties(${XMAKE_${PROJECT}_TARGET} PROPERTIES ${PROPERTY_NAME} "${PROPERTY_VALUE}")
endfunction()


# Add a value to a property of a XMake project (values separated by a space)
function(xmake_add_to_property PROJECT PROPERTY_NAME VALUE)
    if (NOT VALUE)
        return()
    endif()

    get_target_property(OLD_VALUE ${XMAKE_${PROJECT}_TARGET} ${PROPERTY_NAME})

    if (NOT OLD_VALUE STREQUAL "OLD_VALUE-NOTFOUND")
        set_target_properties(${XMAKE_${PROJECT}_TARGET} PROPERTIES ${PROPERTY_NAME} "${OLD_VALUE} ${VALUE}")
    else()
        set_target_properties(${XMAKE_${PROJECT}_TARGET} PROPERTIES ${PROPERTY_NAME} "${VALUE}")
    endif()
endfunction()


# Append a value to the current value of a variable (values separated by a space)
function(xmake_disable_vs_warning PROJECT WARNING EXPORTED)
    if (XMAKE_${PROJECT}_FRAMEWORK)
        return()
    endif()

    if (WIN32)
        xmake_add_to_property(${PROJECT} COMPILE_FLAGS "/wd${WARNING}")
    endif()

    if (EXPORTED AND NOT XMAKE_${PROJECT}_EXECUTABLE)
        xmake_list_append(XMAKE_${PROJECT}_DISABLED_VS_WARNINGS ${WARNING})
    endif()
endfunction()


# Display all the XMake settings of a project
function(xmake_display PROJECT)

    set(TEXT "\n--------------------------------------------------------------------------------\n")
    set(TEXT "${TEXT}XMake settings for '${PROJECT}':\n")

    if (XMAKE_${PROJECT}_TARGET)
        set(TEXT "${TEXT}  Target:               ${XMAKE_${PROJECT}_TARGET}\n")
    endif()

    if (XMAKE_${PROJECT}_FRAMEWORK)
        get_target_property(OUTPUT_DIRECTORY ${XMAKE_${PROJECT}_TARGET} RUNTIME_OUTPUT_DIRECTORY)
        set(TEXT "${TEXT}  Framework path:       ${OUTPUT_DIRECTORY}\n")
    endif()

    if (XMAKE_${PROJECT}_INCLUDE_PATHS)
        set(TEXT "${TEXT}  Include paths:        ${XMAKE_${PROJECT}_INCLUDE_PATHS}\n")
    endif()

    if (XMAKE_${PROJECT}_LINK_PATHS)
        set(TEXT "${TEXT}  Linking paths:        ${XMAKE_${PROJECT}_LINK_PATHS}\n")
    endif()

    if (XMAKE_${PROJECT}_COMPILE_DEFINITIONS)
        set(TEXT "${TEXT}  Compile definitions:  ${XMAKE_${PROJECT}_COMPILE_DEFINITIONS}\n")
    endif()

    if (XMAKE_${PROJECT}_LINK_FLAGS)
        set(TEXT "${TEXT}  Linking flags:        ${XMAKE_${PROJECT}_LINK_FLAGS}\n")
    endif()

    if (XMAKE_${PROJECT}_LINK_TARGETS)
        set(TEXT "${TEXT}  Link targets:         ${XMAKE_${PROJECT}_LINK_TARGETS}\n")
    endif()

    if (XMAKE_${PROJECT}_DISABLED_VS_WARNINGS)
        set(TEXT "${TEXT}  Disabled VS warnings: ${XMAKE_${PROJECT}_DISABLED_VS_WARNINGS}\n")
    endif()

    if (XMAKE_${PROJECT}_EXECUTABLE)
        set(TEXT "${TEXT}  Type:                 EXECUTABLE\n")
    endif()

    if (XMAKE_${PROJECT}_STATIC_LIBRARY)
        set(TEXT "${TEXT}  Type:                 STATIC LIBRARY\n")
    endif()

    if (XMAKE_${PROJECT}_DYNAMIC_LIBRARY)
        set(TEXT "${TEXT}  Type:                 DYNAMIC LIBRARY\n")
    endif()

    if (XMAKE_${PROJECT}_FRAMEWORK)
        set(TEXT "${TEXT}  Type:                 FRAMEWORK\n")
    endif()

    set(TEXT "${TEXT}--------------------------------------------------------------------------------\n")

    message(STATUS "${TEXT}")

endfunction()


#-----------------------------------------------------------------------------------------
# Functions used to declare a XMake project
#-----------------------------------------------------------------------------------------

# Declare a new executable
function(xmake_create_executable PROJECT EXECUTABLE_NAME SOURCE_FILE1)

    # Project settings
    xmake_set(XMAKE_${PROJECT}_TARGET               "${EXECUTABLE_NAME}")
    xmake_set(XMAKE_${PROJECT}_EXECUTABLE           YES)
    xmake_set(XMAKE_${PROJECT}_STATIC_LIBRARY       NO)
    xmake_set(XMAKE_${PROJECT}_DYNAMIC_LIBRARY      NO)
    xmake_set(XMAKE_${PROJECT}_FRAMEWORK            NO)

    # Create the list of source files
    set(SOURCE_FILES ${SOURCE_FILE1})
    foreach(CURRENT_FILE ${ARGN})
        list(APPEND SOURCE_FILES ${CURRENT_FILE})
    endforeach()

    # Declaration of the library
    add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})

    # Set the RPATH
    if (APPLE)
        set_target_properties(${EXECUTABLE_NAME} PROPERTIES LINK_FLAGS "-Wl,-rpath,@loader_path/.")
    elseif (UNIX)
        set_target_properties(${EXECUTABLE_NAME} PROPERTIES INSTALL_RPATH ".")
    endif()
endfunction()


# Declare a new static library
function(xmake_create_static_library PROJECT LIBRARY_NAME SOURCE_FILE1)

    # Project settings
    xmake_set(XMAKE_${PROJECT}_TARGET                        "${LIBRARY_NAME}")
    xmake_set(XMAKE_${PROJECT}_INCLUDE_PATHS                 "")
    xmake_set(XMAKE_${PROJECT}_LINK_PATHS                    "")
    xmake_set(XMAKE_${PROJECT}_COMPILE_DEFINITIONS           "")
    xmake_set(XMAKE_${PROJECT}_LINK_FLAGS                    "")
    xmake_set(XMAKE_${PROJECT}_LINK_TARGETS                  "${LIBRARY_NAME}")
    xmake_set(XMAKE_${PROJECT}_DISABLED_VS_WARNINGS          "")
    xmake_set(XMAKE_${PROJECT}_EXECUTABLE                    NO)
    xmake_set(XMAKE_${PROJECT}_STATIC_LIBRARY                YES)
    xmake_set(XMAKE_${PROJECT}_DYNAMIC_LIBRARY               NO)
    xmake_set(XMAKE_${PROJECT}_FRAMEWORK                     NO)

    # Create the list of source files
    set(SOURCE_FILES ${SOURCE_FILE1})
    foreach(CURRENT_FILE ${ARGN})
        list(APPEND SOURCE_FILES ${CURRENT_FILE})
    endforeach()

    # Declaration of the library
    add_library(${LIBRARY_NAME} STATIC ${SOURCE_FILES})

    if (NOT WIN32)
        set_target_properties(${LIBRARY_NAME} PROPERTIES COMPILE_FLAGS "-fPIC")
    endif()
endfunction()


# Declare a new dynamic library
function(xmake_create_dynamic_library PROJECT LIBRARY_NAME VERSION API_VERSION SOURCE_FILE1)

    # Project settings
    xmake_set(XMAKE_${PROJECT}_TARGET                        "${LIBRARY_NAME}")
    xmake_set(XMAKE_${PROJECT}_INCLUDE_PATHS                 "")
    xmake_set(XMAKE_${PROJECT}_LINK_PATHS                    "")
    xmake_set(XMAKE_${PROJECT}_COMPILE_DEFINITIONS           "")
    xmake_set(XMAKE_${PROJECT}_LINK_FLAGS                    "")
    xmake_set(XMAKE_${PROJECT}_LINK_TARGETS                  "${LIBRARY_NAME}")
    xmake_set(XMAKE_${PROJECT}_DISABLED_VS_WARNINGS          "")
    xmake_set(XMAKE_${PROJECT}_EXECUTABLE                    NO)
    xmake_set(XMAKE_${PROJECT}_STATIC_LIBRARY                NO)
    xmake_set(XMAKE_${PROJECT}_DYNAMIC_LIBRARY               YES)
    xmake_set(XMAKE_${PROJECT}_FRAMEWORK                     NO)

    # Create the list of source files
    set(SOURCE_FILES ${SOURCE_FILE1})
    foreach(CURRENT_FILE ${ARGN})
        list(APPEND SOURCE_FILES ${CURRENT_FILE})
    endforeach()

    # Declaration of the library
    add_library(${LIBRARY_NAME} SHARED ${SOURCE_FILES})

    set_target_properties(${LIBRARY_NAME} PROPERTIES VERSION "${VERSION}"
                                                     SOVERSION "${API_VERSION}"
    )

    # Set the INSTALL_PATH so that the dynamic library can be local
    if (NOT WIN32)
        set_target_properties(${LIBRARY_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH ON
                                                         INSTALL_NAME_DIR "@rpath"
        )
    endif()
endfunction()


# Declare a new framework (MacOS only)
function(xmake_create_framework PROJECT FRAMEWORK_NAME VERSION API_VERSION SOURCE_FILE1)
    xmake_set(XMAKE_${PROJECT}_TARGET               "${FRAMEWORK_NAME}")
    xmake_set(XMAKE_${PROJECT}_INCLUDE_PATHS        "")
    xmake_set(XMAKE_${PROJECT}_LINK_PATHS           "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
    xmake_set(XMAKE_${PROJECT}_COMPILE_DEFINITIONS  "")
    xmake_set(XMAKE_${PROJECT}_LINK_FLAGS           "-F${CMAKE_LIBRARY_OUTPUT_DIRECTORY};-framework ${FRAMEWORK_NAME}")
    xmake_set(XMAKE_${PROJECT}_LINK_TARGETS         "")
    xmake_set(XMAKE_${PROJECT}_EXECUTABLE           NO)
    xmake_set(XMAKE_${PROJECT}_STATIC_LIBRARY       NO)
    xmake_set(XMAKE_${PROJECT}_DYNAMIC_LIBRARY      NO)
    xmake_set(XMAKE_${PROJECT}_FRAMEWORK            YES)

    # Create the list of source files
    set(SOURCE_FILES ${SOURCE_FILE1})
    foreach(CURRENT_FILE ${ARGN})
        list(APPEND SOURCE_FILES ${CURRENT_FILE})
    endforeach()

    # Declaration of the library
    add_library(${FRAMEWORK_NAME} SHARED ${SOURCE_FILES})

    set_target_properties(${FRAMEWORK_NAME} PROPERTIES VERSION "${VERSION}"
                                                       SOVERSION "${API_VERSION}"
                                                       FRAMEWORK TRUE
    )

    # Set the INSTALL_PATH so that the dynamic library can be local
    set_target_properties(${FRAMEWORK_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH ON
                                                       INSTALL_NAME_DIR "@rpath"
    )
endfunction()


#-----------------------------------------------------------------------------------------
# Functions used to set export information to a XMake project
#-----------------------------------------------------------------------------------------

# Export some include paths for a XMake project
function(xmake_export_include_paths PROJECT PATH1)
    xmake_list_append(XMAKE_${PROJECT}_INCLUDE_PATHS "${PATH1}")

    foreach(CURRENT_PATH ${ARGN})
        xmake_list_append(XMAKE_${PROJECT}_INCLUDE_PATHS "${CURRENT_PATH}")
    endforeach()
endfunction()


# Export some libraries paths for a XMake project
function(xmake_export_link_paths PROJECT PATH1)
    xmake_list_append(XMAKE_${PROJECT}_LINK_PATHS "${PATH1}")

    foreach(CURRENT_PATH ${ARGN})
        xmake_list_append(XMAKE_${PROJECT}_LINK_PATHS "${CURRENT_PATH}")
    endforeach()
endfunction()


# Export some compile definitions for a XMake project
function(xmake_export_compile_definitions PROJECT DEFINITION1)
    xmake_list_append(XMAKE_${PROJECT}_COMPILE_DEFINITIONS "${DEFINITION1}")

    foreach(CURRENT_DEFINITION ${ARGN})
        xmake_list_append(XMAKE_${PROJECT}_COMPILE_DEFINITIONS "${CURRENT_DEFINITION}")
    endforeach()
endfunction()


# Export some link flags for a XMake project
function(xmake_export_link_flags PROJECT FLAGS1)
    xmake_list_append(XMAKE_${PROJECT}_LINK_FLAGS "${FLAGS1}")

    foreach(CURRENT_FLAGS ${ARGN})
        xmake_list_append(XMAKE_${PROJECT}_LINK_FLAGS "${CURRENT_FLAGS}")
    endforeach()
endfunction()


#-----------------------------------------------------------------------------------------
# Functions using information from a XMake project
#-----------------------------------------------------------------------------------------

# Add the include and libraries paths of a XMake project to the scope
function(xmake_import_search_paths PROJECT)
    foreach (CURRENT_PATH ${XMAKE_${PROJECT}_INCLUDE_PATHS})
        include_directories("${CURRENT_PATH}")
    endforeach()

    foreach (CURRENT_PATH ${XMAKE_${PROJECT}_LINK_PATHS})
        link_directories("${CURRENT_PATH}")
    endforeach()
endfunction()


# Link a XMake project with one or more other projects
function(xmake_project_link PROJECT PROJECT_TO_LINK1)

    set(PROJECTS_TO_LINK ${PROJECT_TO_LINK1})

    foreach(PROJECT_TO_LINK ${ARGN})
        list(APPEND PROJECTS_TO_LINK ${PROJECT_TO_LINK})
    endforeach()

    foreach(PROJECT_TO_LINK ${PROJECTS_TO_LINK})
        set(NEW_TARGETS ${XMAKE_${PROJECT}_LINK_TARGETS})

        foreach (CURRENT_LIBRARY ${XMAKE_${PROJECT_TO_LINK}_LINK_TARGETS})
            if (NOT XMAKE_${PROJECT}_STATIC_LIBRARY)
                target_link_libraries(${XMAKE_${PROJECT}_TARGET} "${CURRENT_LIBRARY}")
                add_dependencies(${XMAKE_${PROJECT}_TARGET} "${CURRENT_LIBRARY}")

                if (NOT XMAKE_${PROJECT_TO_LINK}_STATIC_LIBRARY)
                    list(APPEND NEW_TARGETS ${CURRENT_LIBRARY})
                endif()
            else()
                list(APPEND NEW_TARGETS ${CURRENT_LIBRARY})
            endif()
        endforeach()

        foreach (LINK_FLAGS ${XMAKE_${PROJECT_TO_LINK}_LINK_FLAGS})
            xmake_add_to_property(${PROJECT} LINK_FLAGS "${LINK_FLAGS}")
        endforeach()

        if (XMAKE_${PROJECT_TO_LINK}_COMPILE_DEFINITIONS)
            xmake_add_to_list_property(${PROJECT} COMPILE_DEFINITIONS "${XMAKE_${PROJECT_TO_LINK}_COMPILE_DEFINITIONS}")
        endif()

        if (XMAKE_${PROJECT_TO_LINK}_FRAMEWORK)
            get_target_property(OUTPUT_DIRECTORY ${XMAKE_${PROJECT_TO_LINK}_TARGET} RUNTIME_OUTPUT_DIRECTORY)
            set_target_properties(${XMAKE_${PROJECT}_TARGET} PROPERTIES COMPILE_FLAGS "-F${OUTPUT_DIRECTORY}")
        endif()

        if (WIN32)
            foreach (WARNING ${XMAKE_${PROJECT_TO_LINK}_DISABLED_VS_WARNINGS})
                xmake_add_to_property(${PROJECT} COMPILE_FLAGS "/wd${WARNING}")
            endforeach()
        endif()

        if (XMAKE_${PROJECT}_STATIC_LIBRARY)
            xmake_set(XMAKE_${PROJECT}_LINK_TARGETS "${NEW_TARGETS}")
            xmake_list_append(XMAKE_${PROJECT}_INCLUDE_PATHS "${XMAKE_${PROJECT_TO_LINK}_INCLUDE_PATHS}")
            xmake_list_append(XMAKE_${PROJECT}_LINK_FLAGS "${XMAKE_${PROJECT_TO_LINK}_LINK_FLAGS}")
            xmake_list_append(XMAKE_${PROJECT}_COMPILE_DEFINITIONS "${XMAKE_${PROJECT_TO_LINK}_COMPILE_DEFINITIONS}")
            xmake_list_append(XMAKE_${PROJECT}_DISABLED_VS_WARNINGS "${XMAKE_${PROJECT_TO_LINK}_DISABLED_VS_WARNINGS}")
        elseif (XMAKE_${PROJECT}_DYNAMIC_LIBRARY)
            xmake_set(XMAKE_${PROJECT}_LINK_TARGETS "${NEW_TARGETS}")
            xmake_list_append(XMAKE_${PROJECT}_INCLUDE_PATHS "${XMAKE_${PROJECT_TO_LINK}_INCLUDE_PATHS}")
            xmake_list_append(XMAKE_${PROJECT}_COMPILE_DEFINITIONS "${XMAKE_${PROJECT_TO_LINK}_COMPILE_DEFINITIONS}")
            xmake_list_append(XMAKE_${PROJECT}_DISABLED_VS_WARNINGS "${XMAKE_${PROJECT_TO_LINK}_DISABLED_VS_WARNINGS}")
        elseif (XMAKE_${PROJECT}_FRAMEWORK)
            xmake_set(XMAKE_${PROJECT}_LINK_TARGETS "${NEW_TARGETS}")
            xmake_list_append(XMAKE_${PROJECT}_INCLUDE_PATHS "${XMAKE_${PROJECT_TO_LINK}_INCLUDE_PATHS}")
            xmake_list_append(XMAKE_${PROJECT}_COMPILE_DEFINITIONS "${XMAKE_${PROJECT_TO_LINK}_COMPILE_DEFINITIONS}")
        endif()
    endforeach()
endfunction()


#-----------------------------------------------------------------------------------------
# Global XMake settings
#-----------------------------------------------------------------------------------------

xmake_set(XMAKE_VERSION "2.2")

if (NOT DEFINED XMAKE_BINARY_DIR)
    xmake_set(XMAKE_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${XMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${XMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${XMAKE_BINARY_DIR}/bin")

if (NOT DEFINED XMAKE_DEPENDENCIES_DIR)
    xmake_set(XMAKE_DEPENDENCIES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies")
endif()


#-----------------------------------------------------------------------------------------
# Global CMake settings
#-----------------------------------------------------------------------------------------

# By default: compile in Release mode
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

# On MacOS X, by default compile both in 32- and 64-bits
if (APPLE)
    if (NOT XMAKE_OSX_ARCHITECTURES)
        set(XMAKE_OSX_ARCHITECTURES "i386;x86_64" CACHE STRING "" FORCE)
    endif()

    if (NOT "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "${XMAKE_OSX_ARCHITECTURES}")
        set(CMAKE_OSX_ARCHITECTURES "${XMAKE_OSX_ARCHITECTURES}" CACHE INTERNAL "" FORCE)
        set(CMAKE_OSX_ARCHITECTURES_DEFAULT "ppc" CACHE INTERNAL "" FORCE)
    endif()
endif()

# Use relative paths on Windows, to reduce path size for command-line limits
if (WIN32)
    set(CMAKE_USE_RELATIVE_PATHS true)
    set(CMAKE_SUPPRESS_REGENERATION true)
endif()
