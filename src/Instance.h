#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class Instance{
public:
    Instance();
    ~Instance();

    VkInstance get() const;
private:
    VkInstance instance = VK_NULL_HANDLE;
    std::vector<const char*> getRequiredExtensions();
};
