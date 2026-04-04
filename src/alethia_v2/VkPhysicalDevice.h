#pragma once

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <vector>
#include <optional>
#include <cstdint>

// Types
struct QueueFamilyIndices
{
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> compute;
    std::optional<uint32_t> transfer;
    std::optional<uint32_t> present;
    
    // family index
    std::unordered_map<uint32_t, uint32_t> timestampValidBits;
    
    bool isComplete() const
    {
        return graphics.has_value()
        && compute.has_value()
        && transfer.has_value()
        && present.has_value();
    }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

std::vector<VkPhysicalDevice> enumeratePhysicalDevices(VkInstance instance);

VkPhysicalDevice selectPhysicalDevice(VkInstance instance);

void buildEnabledFeatures(
                          VkPhysicalDevice device,
                          VkPhysicalDeviceFeatures outFeatures10,
                          VkPhysicalDeviceVulkan12Features& outFeatures12,
                          VkPhysicalDeviceVulkan13Features& outFeatures13);

// Queue families.
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

// Extension support
bool checkDeviceExtentionSupport(
                                 VkPhysicalDevice device,
                                 const std::vector<const char*>& required
                                 );

// Memory utilities
uint32_t findMemoryType(
                        VkPhysicalDevice device,
                        uint32_t typeFilter,
                        VkMemoryPropertyFlags properties
                        );

// Format utilities
VkFormat findSupportedDepthFormat(VkPhysicalDevice device);
