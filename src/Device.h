#pragma once
#include <vulkan/vulkan.h>

class Device{
public:
    Device(VkInstance instance, VkSurfaceKHR surface);
    ~Device();

    VkDevice get() const;
    VkPhysicalDevice physical() const;
    VkQueue graphicsQueue() const;
    VkQueue presentQueue() const;

    uint32_t graphicsQueueFamily() const { return graphicsFamily; }

private:
    VkPhysicalDevice phys = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphics = VK_NULL_HANDLE;
    VkQueue present = VK_NULL_HANDLE;

    uint32_t findGraphicsQueue(VkPhysicalDevice dev, VkSurfaceKHR surface);
    uint32_t graphicsFamily = UINT32_MAX;
};
