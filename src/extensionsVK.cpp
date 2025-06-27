#include "vulkan/extensionsVK.h"

PFN_vkCreateAccelerationStructureKHR           vkCreateAccelerationStructure           = nullptr;
PFN_vkDestroyAccelerationStructureKHR          vkDestroyAccelerationStructure          = nullptr;
PFN_vkGetAccelerationStructureBuildSizesKHR    vkGetAccelerationStructureBuildSizes    = nullptr;
PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddress = nullptr;
PFN_vkCmdBuildAccelerationStructuresKHR        vkCmdBuildAccelerationStructures        = nullptr;
PFN_vkBuildAccelerationStructuresKHR           vkBuildAccelerationStructures           = nullptr;
PFN_vkSetDebugUtilsObjectNameEXT               vkSetDebugUtilsObjectName               = nullptr;

void loadExtensions(VkDevice device, VkInstance instance) {

   
    vkCreateAccelerationStructure = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
        vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));

    if (!vkCreateAccelerationStructure)
    {
        MiniEngine::MiniEngineException("Failed to load vkCreateAccelerationStructureKHR!");
    }
    vkDestroyAccelerationStructure = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(
        vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));

    if (!vkDestroyAccelerationStructure)
    {
        MiniEngine::MiniEngineException("Failed to load vkDestroyAccelerationStructureKHR!");
    }
    vkGetAccelerationStructureBuildSizes = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
        vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));

    if (!vkGetAccelerationStructureBuildSizes)
    {
        MiniEngine::MiniEngineException("Failed to load vkGetAccelerationStructureBuildSizesKHR!");
    }
    vkGetAccelerationStructureDeviceAddress = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
        vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));

    if (!vkGetAccelerationStructureDeviceAddress)
    {
        MiniEngine::MiniEngineException("Failed to load vkGetAccelerationStructureDeviceAddressKHR!");
    }
    vkCmdBuildAccelerationStructures = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
        vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));

    if (!vkCmdBuildAccelerationStructures)
    {
        MiniEngine::MiniEngineException("Failed to load vkCmdBuildAccelerationStructuresKHR!");
    }
    vkBuildAccelerationStructures = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(
        vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR"));

    if (!vkBuildAccelerationStructures)
    {
        MiniEngine::MiniEngineException("Failed to load vkBuildAccelerationStructuresKHR!");
    }

    vkSetDebugUtilsObjectName =
        reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT"));
    if (!vkSetDebugUtilsObjectName)
    {
        MiniEngine::MiniEngineException("Failed to load vkSetDebugUtilsObjectNameEXT!");
    }
}