#ifndef VK_EXTX_PORTABILITY_SUBSET_H_
#define VK_EXTX_PORTABILITY_SUBSET_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

/*
** Copyright (c) 2018 The Khronos Group Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/*
 Please Note:  This extension is currently defined as "EXTX", meaning "multivendor experimental".
 That means the definition of this extension is in active development, and may break compatibility
 between point releases (defined as any increment of VK_EXTX_PORTABILITY_SUBSET_SPEC_VERSION).
 You are free to explore the extension and provide feedback, but it is not recommended to use this
 extension for shipping applications, particularly applications that require the driver implementing this
 extension to be linked dynamically and potentially "dropped-in" to the application execution environment.
 */

#include "vulkan/vulkan.h"

#define VK_EXTX_PORTABILITY_SUBSET_SPEC_VERSION       1
#define VK_EXTX_PORTABILITY_SUBSET_EXTENSION_NAME     "VK_EXTX_portability_subset"

#define VK_EXTX_PORTABILITY_SUBSET_EXTENSION_ID 164
// See enum_offset() from https://www.khronos.org/registry/vulkan/specs/1.1/styleguide.html#_assigning_extension_token_values
#define VK_EXTX_PORTABILITY_SUBSET_STYPE_ID(x) \
    ((VkStructureType)(1000000000 + 1000 * (VK_EXTX_PORTABILITY_SUBSET_EXTENSION_ID - 1) + x))
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_EXTX      VK_EXTX_PORTABILITY_SUBSET_STYPE_ID(0)
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_PROPERTIES_EXTX    VK_EXTX_PORTABILITY_SUBSET_STYPE_ID(1)
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_SUPPORT_EXTX               VK_EXTX_PORTABILITY_SUBSET_STYPE_ID(2)

typedef struct VkPhysicalDevicePortabilitySubsetFeaturesEXTX {
     VkStructureType    sType;
     void*              pNext;
     VkBool32           triangleFans;
     VkBool32           separateStencilMaskRef;
     VkBool32           events;
     VkBool32           standardImageViews;
     VkBool32           samplerMipLodBias;
} VkPhysicalDevicePortabilitySubsetFeaturesEXTX;

typedef struct VkPhysicalDevicePortabilitySubsetPropertiesEXTX {
     VkStructureType    sType;
     void*              pNext;
     uint32_t           minVertexInputBindingStrideAlignment;
} VkPhysicalDevicePortabilitySubsetPropertiesEXTX;

typedef struct VkPhysicalDeviceImageViewSupportEXTX {
     VkStructureType        sType;
     void*                  pNext;
     VkImageViewCreateFlags flags;
     VkImageViewType        viewType;
     VkFormat               format;
     VkComponentMapping     components;
     VkImageAspectFlags     aspectMask;
} VkPhysicalDeviceImageViewSupportEXTX;

#ifdef __cplusplus
}
#endif

#endif // VK_EXTX_PORTABILITY_SUBSET_H_
