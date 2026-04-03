#include <vulkan/vulkan.h>
#include <stdexcept>
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <optional>
#include <limits>

// Types
struct QueueFamilyIndices{
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> compute;
    std::optional<uint32_t> transfer;
    std::optional<uint32_t> present;
    
    bool isComplete() const {
        return graphics.has_value()
        && compute.has_value()
        && transfer.has_value()
        && present.has_value()
    }
};

struct SwapChainSupportDetails{
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// Internal Helpers
static const std::vector<const char*> kRequiredDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME;
};

static bool checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> available(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available.data());
    
    bool allFound = true;
    for (const char* name : kRequiredDeviceExtensions) {
        bool found = false;
        for (const auto& ext : available) {
            if (std::strcmp(name, ext.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            std::cerr << "[Vulkan] Required device extension not available"
            << name << "\n";
            allFound = false;
        }
    }
    return allFound;
}

static int scoreDevice(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(device, &props);
    
    // Reject devices below Vulkan 1.3
    if (props.apiVersion < VK_API_VERSION_1_3)
        return -1;
    
    // Reject devices missing required extensions (e.g. swapchain)
    if (!checkDeviceExtensionSupport(device))
        return -1;
    
    int score = 0;
    
    // Discrete GPU is strongly preferred
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 1000;
    else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        score += 100;
    
    // Core 1.0 features
    VkPhysicalDeviceFeatures features10{};
    vkGetPhysicalDeviceFeatures(device, &features10);
    
    if (features10.samplerAnisotropy)       score += 50;
    if (features10.depthClamp)              score += 50;
    if (features10.multiDrawIndirect)       score += 50;
    if (features10.independentBlend)        score += 30;
    if (features10.fillModeNonSolid)        score += 20;
    if (features10.shaderInt64)             score += 20;
    
    // Not scored — commonly unsupported on Apple/MoltenVK:
    // geometryShader, tessellationShader, wideLines, shaderFloat64
    
    // Vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features12{};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    
    VkPhysicalDeviceFeatures2 query12{};
    query12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    query12.pNext = &features12;
    vkGetPhysicalDeviceFeatures2(device, &query12);
    
    if (features12.bufferDeviceAddress)             score += 100;
    if (features12.descriptorIndexing)              score += 100;
    if (features12.runtimeDescriptionArray)         score += 80;
    if (features12.timelineSemaphore)               score += 80;
    if (features12.scalarBlockLayout)               score += 40;
    
    // Vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_1_3_FEATURES;
    
    VkPhysicalDeviceFeatures2 query13{};
    query13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    query13.pNext = &features13;
    vkGetPhysicalDeviceFeatures2(device, &query13);
    
    if (features13.dynamicRendering)        score += 100;
    if (features13.synchronization2)        score += 100;
    if (features13.maintenance4)            score += 30;
    
    return score;
}

static void printDeviceInfo(VkPhysicalDevice device, int score)
{
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(device, &props);
    
    const char* typeStr = "Unkown";
    switch (props.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            typeStr = "Discrete GPU"; break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:t
            typeStr = "Integrated GPU"; break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_CPU:
            typeStr = "Virtual GPU"; break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            typeStr = "CPU"; break;
        default: break;
    }
    
    uint32_t major = VK_VERSION_MAJOR(props.apiVersion);
    uint32_t minor = VK_VERSION_MINOR(props.apiVersion);
    uint32_t patch = VK_VERSION_PATCH(props.apiVersion);
    
    std::cout   << "[Vulkan] Device: " << props.deviceName
    << " | Type: "         << typeStr
    << " | API: "          << major << "." << minor << "." << patch
    << " | Score: "        << score << "\n";
}

// Public API

std::vector<VkPhysicalDevice> enumeratePhysicalDevices(VkInstance instance)
{
    uint32_t count = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (result != VK_SUCCESS)
        throw std::runtime_error("vkEnumeratePhysicalDevices (count query) failed");
    
    if (count == 0)
        throw std::runtime_error("No Vulkan-capable physical devices found");
    
    std::vector<VkPhysicalDevice> devices(count);
    result = vkEnumeratePhysicalDevices(instance, &count, devices.data());
    if (result != VK_SUCCESS)
        throw std::runtime_error("vkEnumeratePhysicalDevices (handle query) failed");
    return devices;
}

// Selects the highest-scoring device. Logs all candidates.
// Returns VK_NULL_HANDLE if no device meets the minimum bar (Vulkan 1.3).

VkPhysicalDevice selectPhysicalDevice(VkInstance instance)
{
    const std::vector<VkPhysicalDevice> devices = enumeratePhysicalDevices(instance);
​
    VkPhysicalDevice best  = VK_NULL_HANDLE;
    int              bestScore = -1;
​
    for (VkPhysicalDevice device : devices) {
        const int score = scoreDevice(device);
        printDeviceInfo(device, score);
​
        if (score > bestScore) {
            bestScore = score;
            best      = device;
        }
    }
​
    if (best == VK_NULL_HANDLE || bestScore < 0) {
        throw std::runtime_error(
            "No suitable physical device found — Vulkan 1.3 required"
        );
    }
​
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(best, &props);
    std::cout << "[Vulkan] Selected device: " << props.deviceName << "\n";
​
    return best;
}

// Populates a struct with only the features that are both desired and
// supported on the given device. Safe to pass directly to VkDeviceCreateInfo.
// Chains 1.2 and 1.3 feature structs via pNext — caller owns the structs.
void buildEnabledFeatures(VkPhysicalDevice device, VkPhysicalDeviceFeatures& outFeatures10, VkPhysicalDeviceVulkan12Features& outFeatures12, VkPhysicalDeviceVulkan13Features& outFeatures13)
{
    // Query supported
    VkPhysicalDeviceFeatures supported10{};
    vkGetPhysicalDeviceFeatures(device, &supported10);
    
    outFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    outFeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    outFeatures12.pNext = &outFeatures13;
    
    VkPhysicalDeviceFeatures2 query{};
    query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    query.pNext = &outFeatures12;
    vkGetPhysicalDeviceFeatures2(device, &query);
    
    VkPhysicalDeviceVulkan12Features supported12 = outFeatures12;
    VkPhysicalDeviceVulkan13Features supported13 = outFeatures13;
    
    // 1.0: enable only what's supported
    outFeatures10 = {};
    outFeatures10.sampleAnisotropy = supported10.samplerAnisotropy;
    outFeatures10.depthClamp = supported10.depthClamp;
    outFeatures10.multiDrawIndirect = supported10.multiDrawIndirect;
    outFeatures10.independentBlend = supported10.independentBlend;
    outFeatures10.fillModeNonSolid = supported10.fillModeNonSolid;
    outFeatures10.shaderInt64 = supported10.shaderInt64;
    
    // 1.2
    outFeatures12 = {};
    outFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    outFeatures12.bufferDeviceAddress = supported12.bufferDeviceAddress;
    outFeatures12.descriptorIndexing = supported12.descriptorIndexing;
    outFeatures12.runtimeDescriptorArray = supported12.runtimeDescriptorArray;
    outFeatures12.timelineSemaphore = supported12.timelineSemaphore;
    outFeatures12.scalarBlockLayout = supported12.scalarBlockLayout;
    outFeatures12.pNext = &outFeatures13;
    
    // 1.3
    outFeatures = {};
    outFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    outFeatures13.dynamicRendering = supported13.dynamicRendering;
    outFeatures13.synchronization2 = supported13.synchronization2;
    outFeatures13.maintenance4 = supported13.maintenance4;
    outFeatures13.pNext = nullptr;
}

// Queue Family
// Finds graphics, compute, transfer, and present queue families.
// A dedicated transfer family (no graphics bit) is preferred for async uploads;
// falls back to the graphics family if none exists.
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilyIndices indices;
    
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());
    
    for (uint32_t i = 0; i < count; ++i) {
        const VkQueueFlags flags = families[i].queueFlags;
        
        if (!indices.graphics && (flags & VK_QUEUE_GRAPHICS_BIT))
            indices.graphics = i;
        
        if (!indices.compute && (flags & VK_QUEUE_COMPUTE_BIT))
            indices.compute = i;
        
        // Prefer a transfer-only family for async DMA
        if (flags & VK_QUEUE_TRANSFER_BIT) {
            if (!indices.transfer || !(flags & VK_QUEUE_GRAPHICS_BIT))
                indices.transfer = i;
        }
        
        if (!indices.present) {
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport)
                indices.present = i;
        }
        if (indices.isComplete())
            break;
    }
    
    if (!indices.isComplete()) {
        std::cerr << "[Vulkan] Warning: device does not expose a complete queue family set\n";
    }
    return indices;
}

// 2. Swapchain support query.
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }
    
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nulltpr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, details.presentModes.data());
    }
    
    if (details.formats.empty() || details.presentModes.empty()) {
        std::cerr << "[Vulkan] Warning: device has inadequate swap chain support\n";
    }
    return details;
}

// 3. Device extension support (public overload)
// Public variant — caller supplies the required extension list.
// The internal kRequiredDeviceExtensions check runs automatically during
//  device scoring; this overload is for optional feature negotiation later.
bool checkDeviceExtensionSupport(VkPhysicalDevice device,
                                 const std::vector<const char*>& required)
{
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> available(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available.data());
    
    bool allFound = true;
    for (const char* name : required) {
        bool found = false;
        for (const auto& ext : available) {
            if (std::strcmp(name, ext.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            std::cerr << "[Vulkan] Device extension not available: " << name
            << "\n";
            allFound = false;
        }
    }
    return allFound;
}

// 4. Memory type query
// Returns the index of a memory type that satisfies both the type filter
// (from VkMemoryRequirements::memoryTypeBits) and the required property flags.
// Throws if no suitable type exists — caller should check requirements carefully.

uint32_t findMemoryType(
                        VkPhysicalDevice device,
                        uint32_t typeFilter,
                        VkMemoryPropertyFlags properties
                        ){
    VkPhysicalDeviceMemoryProperties memProps{};
    vkGetPhysicalDeviceMemoryProperties(device, &memProps);
    
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        const bool typeMatch = (typeFilter & (1u << i)) != 0;
        const bool propMatch =
                (memProps.memoryTypes[i].propertyFlags & properties) == properties;
        
        if (typeMatch && propMatch)
            return i;
    }
    throw std::runtime_error("Failed to find a suitable memory type");
}

// 5. Depth format selection
// Returns the first format from the candidate list that the device supports
// with optimal tiling and the requested feature flags.
// Tries D32, D32S8, D24S8 in order — D32 preferred for precision.
VkFormat findSupportedDepthFormat(VkPhysicalDevice device)
{
    static const std::vector<VkFormat> candidates = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };
    
    constexpr VkFormatFeatureFlags required =
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    
    for (VkFormat format : candidates) {
        VkFormatProperties props{};
        vkGetPhysicalDeviceFormatProperties(device, format, &props);
        
        if ((props.optimalTilingFeatures & required) == required)
            return format;
    }
    throw std::runtime_error(
                             "Failed to find a supported depth format — "
                             "none of D32, D32S8, D24S8 available with optimal tiling"
                             );
}
