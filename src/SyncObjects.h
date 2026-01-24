#pragma once 
#include <vulkan/vulkan.h>
#include <vector>
#include <cstddef>

class SyncObjects{
public:
    SyncObjects(VkDevice device, size_t maxFramesInFlight);
    ~SyncObjects();

    size_t maxFrames() const { return imageAvailable.size(); }
    VkSemaphore imageAvailableSemaphore(size_t frame) const { return imageAvailable[frame]; }
    VkSemaphore renderFinishedSemaphore(size_t frame) const { return renderFinished[frame]; }
    VkFence inFlightFence(size_t frame) const { return inFlight[frame]; }
private:
    VkDevice device = VK_NULL_HANDLE;
    std::vector<VkSemaphore> imageAvailable;
    std::vector<VkSemaphore> renderFinished;
    std::vector<VkFence> inFlight;
};
