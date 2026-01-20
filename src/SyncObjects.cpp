#include "SyncObjects.h"
#include <stdexcept>

SyncObjects::SyncObjects(VkDevice dev, size_t maxFramesInFlight) : device(dev)
{
    imageAvailable.resize(maxFramesInFlight, VK_NULL_HANDLE);
    renderFinished.resize(maxFramesInFlight, VK_NULL_HANDLE);
    inFlight.resize(maxFramesInFlight, VK_NULL_HANDLE);

    VkSemaphoreCreateInfo sci{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    VkFenceCreateInfo fci { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < maxFramesInFlight; ++i) {
        if (vkCreateSemaphore(device, &sci, nullptr, &imageAvailable[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &sci, nullptr, &renderFinished[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fci, nullptr, &inFlight[i]) != VK_SUCCESS) {
            throw std::runtime_error("Sync Objects Creation Failed");
        }
    }
}

SyncObjects::~SyncObjects() 
{
    for (auto f : inFlight) {
        if (f) vkDestroyFence(device, f, nullptr);
    }
    for (auto s : renderFinished) {
        if (s) vkDestroySemaphore(device, s, nullptr);
    }
    for (auto s : imageAvailable) {
        if (s) vkDestroySemaphore(device, s, nullptr);
    }
}
