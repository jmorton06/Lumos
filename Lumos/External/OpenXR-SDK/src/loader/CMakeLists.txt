# Copyright (c) 2017 The Khronos Group Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Author:
#

if(WIN32)
    # Pass version fields as preprocessor for .rc resource version on Windows.
    add_definitions(-DXR_CURRENT_API_MAJOR_VERSION=${MAJOR} -DXR_CURRENT_API_MINOR_VERSION=${MINOR} -DXR_CURRENT_API_PATCH_VERSION=${PATCH})
    set(openxr_loader_RESOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/loader.rc)
endif()

if(NOT MSVC)
    set(CMAKE_C_VISIBILITY_PRESET hidden)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
endif()

# make cache variables for install destinations
include(GNUInstallDirs)

# List of all files externally generated outside of the loader that the loader
# needs to build with.
set(LOADER_EXTERNAL_GEN_FILES ${COMMON_GENERATED_OUTPUT})
set(LOADER_EXTERNAL_GEN_DEPENDS ${COMMON_GENERATED_DEPENDS})
run_xr_xml_generate(loader_source_generator.py xr_generated_loader.hpp)
run_xr_xml_generate(loader_source_generator.py xr_generated_loader.cpp)

if(DYNAMIC_LOADER)
    add_definitions(-DXRAPI_DLL_EXPORT)
    set(LIBRARY_TYPE SHARED)
    if(MSVC)
        list(APPEND openxr_loader_RESOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/openxr-loader.def)
    endif()
else() # build static lib
    set(LIBRARY_TYPE STATIC)
endif()

add_library(openxr_loader ${LIBRARY_TYPE}
    android_utilities.cpp
    android_utilities.h
    api_layer_interface.cpp
    api_layer_interface.hpp
    loader_core.cpp
    loader_instance.cpp
    loader_instance.hpp
    loader_logger.cpp
    loader_logger.hpp
    loader_logger_recorders.cpp
    loader_logger_recorders.hpp
    manifest_file.cpp
    manifest_file.hpp
    runtime_interface.cpp
    runtime_interface.hpp
    ${GENERATED_OUTPUT}
    ${PROJECT_SOURCE_DIR}/src/common/hex_and_handles.h
    ${PROJECT_SOURCE_DIR}/src/common/object_info.cpp
    ${PROJECT_SOURCE_DIR}/src/common/object_info.h
    ${PROJECT_SOURCE_DIR}/src/common/platform_utils.hpp
    ${LOADER_EXTERNAL_GEN_FILES}
    ${openxr_loader_RESOURCE_FILE}
)
if(BUILD_WITH_SYSTEM_JSONCPP)
    target_link_libraries(openxr_loader PRIVATE JsonCpp::JsonCpp)
else()
    target_sources(openxr_loader
        PRIVATE
        ${PROJECT_SOURCE_DIR}/src/external/jsoncpp/src/lib_json/json_reader.cpp
        ${PROJECT_SOURCE_DIR}/src/external/jsoncpp/src/lib_json/json_value.cpp
        ${PROJECT_SOURCE_DIR}/src/external/jsoncpp/src/lib_json/json_writer.cpp
    )
    target_include_directories(openxr_loader
        PRIVATE
        ${PROJECT_SOURCE_DIR}/src/external/jsoncpp/include
    )
    if(SUPPORTS_Werrorunusedparameter)
        # Don't error on this - triggered by jsoncpp
        target_compile_options(openxr_loader PRIVATE -Wno-unused-parameter)
    endif()
endif()
if(BUILD_WITH_WAYLAND_HEADERS)
    target_include_directories(openxr_loader PRIVATE ${WAYLAND_CLIENT_INCLUDE_DIRS})
endif()

set_target_properties(openxr_loader PROPERTIES FOLDER ${LOADER_FOLDER})

if(LOADER_EXTERNAL_GEN_DEPENDS)
    set_source_files_properties(${LOADER_EXTERNAL_GEN_DEPENDS} PROPERTIES GENERATED TRUE)
endif()
add_dependencies(openxr_loader generate_openxr_header xr_global_generated_files)
target_include_directories(
    openxr_loader
    # for OpenXR headers
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
           $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>

    PRIVATE ${PROJECT_SOURCE_DIR}/src/common

            # for generated dispatch table, common_config.h
            ${CMAKE_CURRENT_SOURCE_DIR}/..
            ${CMAKE_CURRENT_BINARY_DIR}/..

            # for target-specific generated files
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}
    INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(Vulkan_FOUND)
    target_include_directories(openxr_loader PRIVATE ${Vulkan_INCLUDE_DIRS})
endif()
if(NOT BUILD_LOADER_WITH_EXCEPTION_HANDLING)
    target_compile_definitions(openxr_loader PRIVATE XRLOADER_DISABLE_EXCEPTION_HANDLING)
endif()

target_link_libraries(
    openxr_loader
    PRIVATE ${CMAKE_DL_LIBS}
    PUBLIC Threads::Threads
)
target_compile_definitions(openxr_loader PRIVATE ${OPENXR_ALL_SUPPORTED_DEFINES})
if(ANDROID)
    target_link_libraries(
        openxr_loader
        PRIVATE
        ${ANDROID_LOG_LIBRARY}
        ${ANDROID_LIBRARY})
endif()

target_compile_definitions(openxr_loader PRIVATE API_NAME="OpenXR")
openxr_add_filesystem_utils(openxr_loader)

set_target_properties(openxr_loader PROPERTIES DEBUG_POSTFIX "${OPENXR_DEBUG_POSTFIX}")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(FALLBACK_CONFIG_DIRS
        "/etc/xdg"
        CACHE
            STRING
            "Search path to use when XDG_CONFIG_DIRS is unset or empty or the current process is SUID/SGID. Default is freedesktop compliant."
    )
    set(FALLBACK_DATA_DIRS
        "/usr/local/share:/usr/share"
        CACHE
            STRING
            "Search path to use when XDG_DATA_DIRS is unset or empty or the current process is SUID/SGID. Default is freedesktop compliant."
    )
    target_compile_definitions(
        openxr_loader
        PRIVATE
        FALLBACK_CONFIG_DIRS="${FALLBACK_CONFIG_DIRS}"
        FALLBACK_DATA_DIRS="${FALLBACK_DATA_DIRS}"
        SYSCONFDIR="${CMAKE_INSTALL_FULL_SYSCONFDIR}"
    )
    if(NOT (CMAKE_INSTALL_FULL_SYSCONFDIR STREQUAL "/etc"))
        target_compile_definitions(openxr_loader PRIVATE EXTRASYSCONFDIR="/etc")
    endif()

    set_target_properties(openxr_loader PROPERTIES SOVERSION "${MAJOR}" VERSION "${MAJOR}.${MINOR}.${PATCH}")

    add_custom_target(
        libopenxr_loader.so.${MAJOR}.${MINOR} ALL
        COMMAND ${CMAKE_COMMAND} -E create_symlink libopenxr_loader.so.${MAJOR}.${MINOR}.${PATCH}
                libopenxr_loader.so.${MAJOR}.${MINOR}
    )
elseif(WIN32)
    if(MSVC)
        # WindowsStore (UWP) apps must be compiled with dynamic CRT linkage (default)
        if(NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
            foreach(configuration in CMAKE_C_FLAGS_DEBUG
                                    CMAKE_C_FLAGS_RELEASE
                                    CMAKE_C_FLAGS_RELWITHDEBINFO
                                    CMAKE_CXX_FLAGS_DEBUG
                                    CMAKE_CXX_FLAGS_RELEASE
                                    CMAKE_CXX_FLAGS_RELWITHDEBINFO)
                # If building DLLs, force static CRT linkage
                if(DYNAMIC_LOADER)
                    if(${configuration} MATCHES "/MD")
                        string(REGEX REPLACE "/MD" "/MT" ${configuration} "${${configuration}}")
                    endif()
                else() # Otherwise for static libs, link the CRT dynamically
                    if(${configuration} MATCHES "/MT")
                        string(REGEX REPLACE "/MT" "/MD" ${configuration} "${${configuration}}")
                    endif()
                endif()
            endforeach()
        endif()

        target_compile_options(openxr_loader PRIVATE /wd6386)
    endif()

    target_link_libraries(openxr_loader PUBLIC advapi32)

    # Need to copy DLL to client directories so clients can easily load it.
    if(DYNAMIC_LOADER AND (CMAKE_GENERATOR MATCHES "^Visual Studio.*"))
        file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>/openxr_loader$<$<CONFIG:Debug>:${OPENXR_DEBUG_POSTFIX}>.dll COPY_DLL_SRC_PATH)
        file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>/openxr_loader$<$<CONFIG:Debug>:${OPENXR_DEBUG_POSTFIX}>.pdb COPY_PDB_SRC_PATH)
        file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/../tests/hello_xr/$<CONFIGURATION>/
             COPY_DST_HELLO_XR_PATH
        )
        file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/../tests/loader_test/$<CONFIGURATION>/
             COPY_DST_LOADER_TEST_PATH
        )
        file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/../conformance/conformance_test/$<CONFIGURATION>/
             COPY_DST_CONFORMANCE_TEST_PATH
        )
        add_custom_command(
            TARGET openxr_loader POST_BUILD
            COMMAND xcopy /Y /I ${COPY_DLL_SRC_PATH} ${COPY_DST_HELLO_XR_PATH}
            COMMAND if exist ${COPY_PDB_SRC_PATH} xcopy /Y /I ${COPY_PDB_SRC_PATH} ${COPY_DST_HELLO_XR_PATH}
            COMMAND xcopy /Y /I ${COPY_DLL_SRC_PATH} ${COPY_DST_LOADER_TEST_PATH}
            COMMAND if exist ${COPY_PDB_SRC_PATH} xcopy /Y /I ${COPY_PDB_SRC_PATH} ${COPY_DST_LOADER_TEST_PATH}
            COMMAND xcopy /Y /I ${COPY_DLL_SRC_PATH} ${COPY_DST_CONFORMANCE_TEST_PATH}
            COMMAND if exist ${COPY_PDB_SRC_PATH} xcopy /Y /I ${COPY_PDB_SRC_PATH} ${COPY_DST_CONFORMANCE_TEST_PATH}
        )
    endif()
elseif(ANDROID)
    set(JNIPP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../external/jnipp)
    set(JNIWRAPPER_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../external/android-jni-wrappers)
    file(GLOB ANDROID_WRAP_SOURCES ${JNIWRAPPER_ROOT}/wrap/*.cpp)
    set_target_properties(openxr_loader
        PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED TRUE)
    target_sources(openxr_loader
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/android_utilities.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/android_utilities.h
        ${ANDROID_WRAP_SOURCES}
        ${JNIPP_ROOT}/jnipp.cpp
    )
    target_include_directories(openxr_loader
        PRIVATE
        ${JNIPP_ROOT}
        ${JNIWRAPPER_ROOT}
    )
endif()

if(NOT MSVC)
    set(XR_API_VERSION "${MAJOR}.${MINOR}")
    set(EXTRA_LIBS ${CMAKE_THREAD_LIBS_INIT})
    configure_file(openxr.pc.in openxr.pc @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/openxr.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif()

# Copy loader to conformance_cli binary folder if built as dynamic
if(DYNAMIC_LOADER AND BUILD_CONFORMANCE_CLI)
    add_custom_command(TARGET openxr_loader POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:openxr_loader> $<TARGET_PROPERTY:conformance_cli,BINARY_DIR>
    )
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        openxr_loader
        PRIVATE -Wextra
                -fno-strict-aliasing
                -fno-builtin-memcmp
                "$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>"
                -ffunction-sections
                -fdata-sections
    )
    # Make build depend on the version script/export map
    target_sources(openxr_loader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/openxr-loader.map)
    # Add the linker flag.
    set_target_properties(openxr_loader PROPERTIES LINK_FLAGS "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/openxr-loader.map")
    # For GCC version 7.1 or greater, we need to disable the implicit fallthrough warning since
    # there's no consistent way to satisfy all compilers until they all accept the C++17 standard
    if(CMAKE_COMPILER_IS_GNUCC AND NOT (CMAKE_CXX_COMPILER_VERSION LESS 7.1))
        target_compile_options(openxr_loader PRIVATE -Wimplicit-fallthrough=0)
    endif()
endif()

add_library(headers INTERFACE)
target_include_directories(headers INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# We will never actually install here, but it's easier to set something than block all install code.
if(ANDROID AND NOT INSTALL_TO_ARCHITECTURE_PREFIXES)
    set(CMAKE_INSTALL_BINDIR bin)
    set(CMAKE_INSTALL_LIBDIR lib)
endif()

install(
    TARGETS openxr_loader headers EXPORT openxr_loader_export
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT Loader
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT Loader
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT Loader
)

export(
    EXPORT openxr_loader_export
    FILE ${CMAKE_CURRENT_BINARY_DIR}/OpenXRTargets.cmake
    NAMESPACE OpenXR::
)

if(WIN32 AND NOT INSTALL_TO_ARCHITECTURE_PREFIXES)
    set(TARGET_DESTINATION cmake)
else()
    set(TARGET_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/openxr/)
endif()

install(
    EXPORT openxr_loader_export
    FILE OpenXRTargets.cmake
    NAMESPACE OpenXR::
    DESTINATION ${TARGET_DESTINATION}
    COMPONENT CMakeConfigs
)

include(CMakePackageConfigHelpers)
set(INCLUDE_INSTALL_DIR include/)
configure_package_config_file(
    OpenXRConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/OpenXRConfig.cmake
    INSTALL_DESTINATION ${TARGET_DESTINATION}
    PATH_VARS INCLUDE_INSTALL_DIR
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/OpenXRConfigVersion.cmake
    VERSION "${MAJOR}.${MINOR}.${PATCH}"
    COMPATIBILITY SameMajorVersion
)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/OpenXRConfig.cmake
          ${CMAKE_CURRENT_BINARY_DIR}/OpenXRConfigVersion.cmake
    DESTINATION ${TARGET_DESTINATION}
    COMPONENT CMakeConfigs
)

if(WIN32 AND NOT MINGW AND DYNAMIC_LOADER)
    install(FILES $<TARGET_PDB_FILE:openxr_loader> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
endif()

# Make the "meta" cmake config/version file to redirect to the right arch/build
if(WIN32 AND INSTALL_TO_ARCHITECTURE_PREFIXES)
    set(TARGET_SUBDIR cmake/openxr)
    set(WARNING "This is a generated file - do not edit!")
    set(FN OpenXRConfig)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/metaconfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/metaconfig.cmake
        @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/metaconfig.cmake
        DESTINATION .
        RENAME OpenXRConfig.cmake
        COMPONENT CMakeConfigs
    )

    set(FN OpenXRConfigVersion)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/metaconfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/metaconfigversion.cmake
        @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/metaconfigversion.cmake
        DESTINATION .
        RENAME OpenXRConfigVersion.cmake
        COMPONENT CMakeConfigs
    )
elseif(ANDROID AND INSTALL_TO_ARCHITECTURE_PREFIXES)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/abi.json
        ${CMAKE_CURRENT_BINARY_DIR}/abi.json
        @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/abi.json
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT Prefab)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/prefab.json
        ${CMAKE_CURRENT_BINARY_DIR}/prefab.json
        @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/prefab.json
        DESTINATION ${PREFAB_INSTALL_DIR}
        COMPONENT Prefab)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/AndroidManifest.xml.in
        ${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml
        DESTINATION .
        COMPONENT Prefab)
    install(FILES module.json
        DESTINATION ${PREFAB_MODULE_INSTALL_DIR}
        COMPONENT Prefab)

    # This gets used directly by build-aar.sh
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/openxr_loader_for_android.pom
        ${CMAKE_CURRENT_BINARY_DIR}/openxr_loader_for_android-${MAJOR}.${MINOR}.${PATCH}${OPENXR_ANDROID_VERSION_SUFFIX}.pom)
endif()
