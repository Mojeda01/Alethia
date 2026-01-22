#pragma once 

#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace swapchain_select{
inline VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
    if (formats.empty()) {
        throw std::invalid_argument("No surface formats available");
    }
    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return f;
        }
    }
    return formats.front();
}

inline VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes) {
    if (modes.empty()) {
        throw std::invalid_argument("No present modes available");
    }
    for (auto m : modes) {
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
            return m;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

inline VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& caps, uint32_t width, uint32_t height) {
    if (caps.currentExtent.width != UINT32_MAX) {
        return caps.currentExtent;
    }
    if (width == 0 || height == 0) {
        throw std::invalid_argument("Requested extent must be non-zero when currentExtent is undefined");
    }
    VkExtent2D;
    e.width = std::clamp(width, caps.minImageExtent.width, caps.maxImageExtent.width);
    e.height = std::clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height);
    return e;
}
} // namespace swapchain_select
