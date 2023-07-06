# Copyright (c) 2017-2023, The Khronos Group Inc.
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

# Set up the OpenXR version variables, used by several targets in this project.
set(MAJOR "0")
set(MINOR "0")
set(PATCH "0")

if(EXISTS "${PROJECT_SOURCE_DIR}/specification/registry/xr.xml")
	file(STRINGS ${PROJECT_SOURCE_DIR}/specification/registry/xr.xml lines REGEX "#define <name>XR_CURRENT_API_VERSION")
else()
	file(STRINGS ${PROJECT_SOURCE_DIR}/include/openxr/openxr.h lines REGEX "#define XR_CURRENT_API_VERSION")
endif()
list(LENGTH lines len)
if(${len} EQUAL 1)
    list(GET lines 0 cur_line)
    # Erase everything up to the open parentheses
    string(REGEX REPLACE "^[^\(]+" "" VERSION_BEFORE_ERASED ${cur_line})
    # Erase everything after the close parentheses
    string(REGEX REPLACE "[^\)]+$" "" VERSION_AFTER_ERASED ${VERSION_BEFORE_ERASED})
    # Erase the parentheses
    string(REPLACE "(" "" VERSION_AFTER_ERASED2 ${VERSION_AFTER_ERASED})
    string(REPLACE ")" "" VERSION_AFTER_ERASED3 ${VERSION_AFTER_ERASED2})
    string(REPLACE " " "" VERSION_AFTER_ERASED4 ${VERSION_AFTER_ERASED3})
    string(REGEX REPLACE "^([0-9]+)\\,[0-9]+\\,[0-9]+" "\\1" MAJOR "${VERSION_AFTER_ERASED4}")
    string(REGEX REPLACE "^[0-9]+\\,([0-9]+)\\,[0-9]+" "\\1" MINOR "${VERSION_AFTER_ERASED4}")
    string(REGEX REPLACE "^[0-9]+\\,[0-9]+\\,([0-9]+)" "\\1" PATCH "${VERSION_AFTER_ERASED4}")
else()
    message(FATAL_ERROR "Unable to fetch major/minor version from registry or header")
endif()
