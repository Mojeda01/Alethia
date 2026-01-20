#include "Device.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>

Device::Device(VkInstance instance, VkSurfaceKHR surface) {
    uint32_t devCount = 0;
    vkEnumeratePhysicalDevices(instance, &devCount, nullptr);
    if (devCount == 0) throw std::runtime_error("No GPUs found");

    std::vector<VkPhysicalDevice> devs(devCount);
    vkEnumeratePhysicalDevices(instance, &devCount, devs.data());

    phys = devs[0]; // first device is fine on macOS

    graphicsFamily = findGraphicsQueue(phys, surface);

    if (graphicsFamily == UINT32_MAX || graphicsFamily == VK_QUEUE_FAMILY_IGNORED) {
        throw std::runtime_error("graphicsFamily not set");
    }

    float prio = 1.0f;
    VkDeviceQueueCreateInfo qci{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    qci.queueFamilyIndex = graphicsFamily;
    qci.queueCount = 1;
    qci.pQueuePriorities = &prio;

    const char* exts[] = { 
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset"
    };

    VkDeviceCreateInfo dci{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = 2;
    dci.ppEnabledExtensionNames = exts;

    if (vkCreateDevice(phys, &dci, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateDevice failed");
    }

    vkGetDeviceQueue(device, graphicsFamily, 0, &graphics);
    present = graphics;
}

Device::~Device() {
    if (device) vkDestroyDevice(device, nullptr);
}

VkDevice Device::get() const { return device; }
VkPhysicalDevice Device::physical() const { return phys; }
VkQueue Device::graphicsQueue() const { return graphics; }
VkQueue Device::presentQueue() const { return present; }

uint32_t Device::findGraphicsQueue(VkPhysicalDevice dev, VkSurfaceKHR surface) {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr);
    std::vector<VkQueueFamilyProperties> props(count);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, props.data());

    for (uint32_t i = 0; i < count; ++i) {
        VkBool32 present = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &present);
        if ((props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) {
            return i;
        }
    }
    throw std::runtime_error("No suitable queue family found");
}
