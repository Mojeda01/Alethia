#include "VkLogicalDevice.h"
#include "VkPhysicalDevice.h"

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <vector>
#include <unordered_set>
#include <iostream>

static const std::vector<const char*> kValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

static std::vector<VkDeviceQueueCreateInfo> buildQueueCreateInfos(const QueueFamilyIndices& indices, const float& priority)
{
    std::unordered_set<uint32_t> unique;
    
    // Only add indices that are actually populated.
    if (indices.graphics) unique.insert(*indices.graphics);
    if (indices.compute)  unique.insert(*indices.compute);
    if (indices.transfer) unique.insert(*indices.transfer);
    if (indices.present)  unique.insert(*indices.present);
    
    std::vector<VkDeviceQueueCreateInfo> infos;
    infos.reserve(unique.size());
    
    for (uint32_t family : unique) {
        VkDeviceQueueCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        info.queueFamilyIndex = family;
        info.queueCount = 1;
        info.pQueuePriorities = &priority;
        infos.push_back(info);
    }
    return infos;
}

// createLogicalDevice
LogicalDevice createLogicalDevice(
        VkPhysicalDevice physical,
        const QueueFamilyIndices& indices,
        VkPhysicalDeviceFeatures& features10,
        VkPhysicalDeviceVulkan12Features& features12,
        VkPhysicalDeviceVulkan13Features& features13)
{
    if (!indices.isComplete())
    {
        throw std::runtime_error(
        "Cannot create logical device — QueueFamilyIndices is incomplete");
    }
    
    // Queue create infos
    constexpr float kQueuePriority = 1.0f;
    const auto queueInfos = buildQueueCreateInfos(indices, kQueuePriority);
    
    features13.pNext = nullptr;
    features12.pNext = &features13;
    
    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.features = features10;
    features2.pNext = &features12;
    
    // Extensions
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    
    // VkDeviceCreateInfo
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &features2; // feature chain
    createInfo.pEnabledFeatures = nullptr; // must be null when
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
    createInfo.pQueueCreateInfos = queueInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t> (deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data(); 
    
#ifndef NDEBUG
    // Deprecciated in Vulkan 1.1+ but some older drivers still read it
    createInfo.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
    createInfo.ppEnabledLayerNames = kValidationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif
    
    // Create
    LogicalDevice ld;
    if (vkCreateDevice(physical, &createInfo, nullptr, &ld.device) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreateDevice failed");
    }
    
    // Retrieve queue handles
    vkGetDeviceQueue(ld.device, *indices.graphics, 0, &ld.graphicsQueue);
    vkGetDeviceQueue(ld.device, *indices.compute,  0, &ld.computeQueue);
    vkGetDeviceQueue(ld.device, *indices.transfer, 0, &ld.transferQueue);
    vkGetDeviceQueue(ld.device, *indices.present,  0, &ld.presentQueue);
    
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physical, &props);
    std::cout << "[Vulkan] Logical device created on: " << props.deviceName << "\n";
    std::cout << "[Vulkan]   Graphics queue family : " << *indices.graphics << "\n";
    std::cout << "[Vulkan]   Compute  queue family : " << *indices.compute  << "\n";
    std::cout << "[Vulkan]   Transfer queue family : " << *indices.transfer << "\n";
    std::cout << "[Vulkan]   Present  queue family : " << *indices.present  << "\n";
    return ld;
}

// destroyLogicalDevice
void destroyLogicalDevice(LogicalDevice& ld)
{
    if (ld.device != VK_NULL_HANDLE) {
        vkDestroyDevice(ld.device, nullptr);
        ld.device = VK_NULL_HANDLE;
    }

    // Queue handles are owned by the device — zero them out for safety.
    ld.graphicsQueue = VK_NULL_HANDLE;
    ld.computeQueue  = VK_NULL_HANDLE;
    ld.transferQueue = VK_NULL_HANDLE;
    ld.presentQueue  = VK_NULL_HANDLE;
}
