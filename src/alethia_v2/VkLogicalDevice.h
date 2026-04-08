#pragma once

#include <vulkan/vulkan.h>
#include "VkPhysicalDevice.h"

// aggregate the VkDevice and the retrieved queue handles.
struct LogicalDevice{
    VkDevice device = VK_NULL_HANDLE;
    
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue computeQueue = VK_NULL_HANDLE;
    VkQueue transferQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
};

// Creates a VkDevice from the selected physical device.
LogicalDevice createLogicalDevice(
    VkPhysicalDevice physical,
    const QueueFamilyIndieces& indices,
    VkPhysicalDeviceFeatures& features10,
    VkPhysicalDeviceVulkan12Features& features12,
    VkPhysicalDeviceVulkan13Features& features13
);

void destroyLogicalDevice(LogicalDevice& ld);
