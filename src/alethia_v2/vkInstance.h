#pragma once
#include <vulkan/vulkan.h>

VkInstance createInstance();
VkDebugUtilsMessengerEXT createDebuggerMessenger(VkInstance instance);
void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger);
