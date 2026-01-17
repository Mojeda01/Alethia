#include "Instance.h"
#include <GLFW/glfw3.h>
#include <stdexcept>

Instance::Instance() {
    VkApplicationInfo app{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app.pApplicationName = "VulkanLab";
    app.apiVersion = VK_API_VERSION_1_3;

    auto extensions = getRequiredExtensions();

    const char* layers[] = { "VK_LAYER_KHRONOS_validation" };

    VkInstanceCreateInfo ci{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    ci.pApplicationInfo = &app;
    ci.enabledExtensionCount = (uint32_t)extensions.size();
    ci.ppEnabledExtensionNames = extensions.data();
    ci.enabledLayerCount = 1;
    ci.ppEnabledLayerNames = layers;

    if (vkCreateInstance(&ci, nullptr, &instance) != VK_SUCCESS) {
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
