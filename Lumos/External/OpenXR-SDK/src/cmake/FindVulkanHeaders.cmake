# ~~~
# Copyright (c) 2018-2019 Valve Corporation
# Copyright (c) 2018-2019 LunarG, Inc.
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
# ~~~

#.rst:
# FindVulkanHeaders
# -----------------
#
# Try to find Vulkan Headers and Registry.
#
# This module is intended to be used by projects that build Vulkan
# "system" components such as the loader and layers.
# Vulkan applications should instead use the FindVulkan (or similar)
# find module that locates the headers and the loader library.
#
# When using this find module to locate the headers and registry
# in a Vulkan-Headers repository, the Vulkan-Headers repository
# should be built with 'install' target and the following environment
# or CMake variable set to the location of the install directory.
#
#    VULKAN_HEADERS_INSTALL_DIR
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines no IMPORTED targets
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables::
#
#   VulkanHeaders_FOUND          - True if VulkanHeaders was found
#   VulkanHeaders_INCLUDE_DIRS   - include directories for VulkanHeaders
#
#   VulkanRegistry_FOUND         - True if VulkanRegistry was found
#   VulkanRegistry_DIRS          - directories for VulkanRegistry
#
# The module will also define two cache variables::
#
#   VulkanHeaders_INCLUDE_DIR    - the VulkanHeaders include directory
#   VulkanRegistry_DIR           - the VulkanRegistry directory
#

# Use HINTS instead of PATH to search these locations before
# searching system environment variables like $PATH that may
# contain SDK directories.
find_path(VulkanHeaders_INCLUDE_DIR
    NAMES vulkan/vulkan.h
    HINTS
        ${VULKAN_HEADERS_INSTALL_DIR}/include
        "$ENV{VULKAN_HEADERS_INSTALL_DIR}/include"
        "$ENV{VULKAN_SDK}/include")

if(VulkanHeaders_INCLUDE_DIR)
   get_filename_component(VULKAN_REGISTRY_PATH_HINT ${VulkanHeaders_INCLUDE_DIR} DIRECTORY)
   find_path(VulkanRegistry_DIR
       NAMES vk.xml
       HINTS "${VULKAN_REGISTRY_PATH_HINT}/share/vulkan/registry")
endif()

set(VulkanHeaders_INCLUDE_DIRS ${VulkanHeaders_INCLUDE_DIR})
set(VulkanRegistry_DIRS ${VulkanRegistry_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VulkanHeaders
    DEFAULT_MSG
    VulkanHeaders_INCLUDE_DIR)
find_package_handle_standard_args(VulkanRegistry
    DEFAULT_MSG
    VulkanRegistry_DIR)

mark_as_advanced(VulkanHeaders_INCLUDE_DIR VulkanRegistry_DIR)
