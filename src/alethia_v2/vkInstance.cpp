#include <vulkan/vulkan.h>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <iostream>

#ifdef _WIN32
    #include <debugapi.h>
    #define DEBUG_BREAK() DebugBreak()
#else
    #include <csignal>
    #define DEBUG_BREAK() raise(SIGTRAP)
#endif

static const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbak(
    VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
    VkDebugUtilsMessageTypeFlagsEXT             type,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    [[maybe_unused]] void*                      pUserData
    ){
    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT unusedType = type;
    
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "[Vulkan] " << pCallbackData->pMessage << "\n";
    } else {
        std::cout << "[Vulkan] " << pCallbackData->pMessage << "\n";
    }
#ifndef NDEBUG
    if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT){
        DEBUG_BREAK();
    }
#endif
    
    return VK_FALSE;
}

static bool checkValidationLayerSupport() {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> available(layerCounter);
    vkEnumerateInstanceLayerProperties(&layerCount, available.data());
    
    bool allFound = true;
    for (const char* name : validationLayers) {
        bool found = false;
        for (const auto& layer : available) {
            if (std::strcmp(name, layer.layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            std::cerr << "[Vulkan] Validation layer not available: " << name << "\n";
            allFound = false;
        }
    }
    return allFound;
}

static bool checkExtensionSupport(const std::vector<const char*>& required)
{
    uint32_t count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> available(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, available.data());
    
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
            std::cerr << "[vulkan] Required extension not available: " << name << "\n";
            allFound = false;
        }
    }
    return allFound;
}

VkInstance createInstance() {
    // Confirm Vulkan 1.3 is supported before requesting it.
    uint32_t apiVersion = 0;
    vkEnumerateInstanceVersion(&apiVersion);
    if (apiVersion < VK_API_VERSION_1_3) {
        throw std::runtime_error(
                                 "Vulkan 1.3 not supported on this system — found version " +
                                 std::to_string(VK_VERSION_MAJOR(apiVersion)) + "." +
                                 std::to_string(VK_VERSION_MINOR(apiVersion)) + "." +
                                 std::to_string(VK_VERSION_PATCH(apiVersion))
                                 );
    }
    
    // Guard against GLFW not being initialized or Vulkan not being supported
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (glfwExtensions == nullptr) {
        throw std::runtime_error(
                                 "glfwGetRequiredInstanceExtensions returned nullptr — "
                                 "GLFW not initialized or Vulkan not supported"
                                 );
    }
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
#ifndef NDEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    
#ifdef __APPLE__
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
    
    
    if (!checkExtensionSupport(extensions)) {
        throw std::runtime_error(
                                 "One or more required Vulkan instance extensions are not supported — "
                                 "see above for details"
                                 );
    }
#ifndef NDEBUG
    if (!checkValidationLayerSupport()) {
        throw std::runtime_error(
                                 "One or more validation layers are not available — "
                                 "see above for details"
                                 );
    }
#endif
    
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "ALETHIA";
    appInfo.pApplicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "ALETHIA GAME ENGINE";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;
    
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    
#ifdef __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
#ifndef NDEBUG
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity =   VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType =   VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;
    
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
    createInfo.pNext = &debugCreateInfo;
#else
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
#endif
    
    VkInstance instance;
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateInstance failed");
    }
    return instance;
}

// Call this after createInstance() to get a persistent messenger for the
// lifetime of the application. Destroy it before vkDestroyInstance.
VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance) {
#ifdef NDEBUG
    return VK_NULL_HANDLE;
#else
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")
    );
    if (func == nullptr) {
        throw std::runtime_error("vkCreateDebugUtilsMessengerEXT not available");
    }
    
    VkDebugUtilsMessengerEXT messenger;
    if (func(instance, &createInfo, nullptr, &messenger) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create debug messenger");
    }
    return messenger;
#endif
}

// Call before vkDestroyInstance
void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger) {
#ifndef NDEBUG
    if (messenger == VK_NULL_HANDLE) return;
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")
    );
    if (func != nullptr) {
        func(instance, messenger, nullptr);
    }
#endif
}
