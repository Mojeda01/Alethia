#include "Instance.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>
#include <iostream>

Instance::Instance() {
    VkApplicationInfo app{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app.pApplicationName = "VulkanLab";
    app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app.pEngineName = "NoEngine";
    app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app.apiVersion = VK_API_VERSION_1_3;
    
    std::vector<const char*> extensions = getRequiredExtensions();

    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    VkInstanceCreateInfo ci{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    ci.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    ci.pApplicationInfo = &app;
    ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    ci.ppEnabledExtensionNames = extensions.data();

    ci.enabledLayerCount = 0;
    ci.ppEnabledLayerNames = nullptr;

    VkResult r = vkCreateInstance(&ci, nullptr, &instance);
    if (r != VK_SUCCESS) {
        std::cerr << "vkCreateInstance failed: " << r << "\n";
        throw std::runtime_error("vkCreateInstance failed");
    }
}

Instance::~Instance() {
    if (instance) vkDestroyInstance(instance, nullptr);
}

VkInstance Instance::get() const {
    return instance;
}

std::vector<const char*> Instance::getRequiredExtensions() {
    uint32_t count = 0;
    const char** exts = glfwGetRequiredInstanceExtensions(&count); 
    return std::vector<const char*>(exts, exts + count);
}
