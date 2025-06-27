#ifndef VULKAN_EXT_H
#define VULKAN_EXT_H

// /////////////////////////////////////////////////////////////

// DECLARATION OF FUNCTION POINTERS RELATED TO VULKAN EXTENSIONS

// /////////////////////////////////////////////////////////////

#include "common.h"

// Global function pointers
extern PFN_vkCreateAccelerationStructureKHR           vkCreateAccelerationStructure;
extern PFN_vkDestroyAccelerationStructureKHR          vkDestroyAccelerationStructure;
extern PFN_vkGetAccelerationStructureBuildSizesKHR    vkGetAccelerationStructureBuildSizes;
extern PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddress;
extern PFN_vkCmdBuildAccelerationStructuresKHR        vkCmdBuildAccelerationStructures;
extern PFN_vkBuildAccelerationStructuresKHR           vkBuildAccelerationStructures;
extern PFN_vkSetDebugUtilsObjectNameEXT               vkSetDebugUtilsObjectName;


void loadExtensions(VkDevice device, VkInstance instance);

#endif