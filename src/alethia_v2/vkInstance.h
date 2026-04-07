#pragma once
#include <vulkan/vulkan.h>

VkInstance createInstance();
VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance);
void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger);
