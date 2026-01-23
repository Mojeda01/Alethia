#include "FrameSync.h"
#include <stdexcept>
#include <utility>

FrameSync::FrameSync(VkDevice dev, uint32_t swapImageCount, uint32_t frames)
    : device(dev), maxFrames(frames)
{
    imageAvailableSem.resize(maxFrames);
    renderFinishedSem.resize(maxFrames);
    inFlightFrame.resize(maxFrames);
    inFlightImage.assign(swapImageCount, VK_NULL_HANDLE);

    VkSemaphoreCreateInfo sci{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fci{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < maxFrames; ++i) {
        if (    vkCreateSemaphore(device, &sci, nullptr, &imageAvailableSem[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &sci, nullptr, &renderFinishedSem[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fci, nullptr, &inFlightFrame[i]) != VK_SUCCESS) {
                throw std::runtime_error("FrameSync creation failed");
        }
    }
}

void FrameSync::destroy() noexcept {
    if (device != VK_NULL_HANDLE) {
        for (auto f : inFlightFrame) vkDestroyFence(device, f, nullptr);
        for (auto s : renderFinishedSem) vkDestroySemaphore(device, s, nullptr);
        for (auto s : imageAvailableSem) vkDestroySemaphore(device, s, nullptr);
    }
    inFlightFrame.clear();
    renderFinishedSem.clear();
    imageAvailableSem.clear();
    inFlightImage.clear();
    frameIndex = 0;
    maxFrames = 0;
    device = VK_NULL_HANDLE;
}

FrameSync::FrameSync(FrameSync&& o) noexcept  
        :   device(o.device),
            maxFrames(o.maxFrames),
            frameIndex(o.frameIndex),
            imageAvailableSem(std::move(o.imageAvailableSem)),
            renderFinishedSem(std::move(o.renderFinishedSem)),
            inFlightFrame(std::move(o.inFlightFrame)),
            inFlightImage(std::move(o.inFlightImage)) {
    o.device = VK_NULL_HANDLE;
    o.maxFrames = 0;
    o.frameIndex = 0;
}

FrameSync& FrameSync::operator=(FrameSync&& o) noexcept {
    if (this == &o) return *this;
    destroy();
    device = o.device;
    maxFrames = o.maxFrames;
    frameIndex = o.frameIndex;
    imageAvailableSem = std::move(o.imageAvailableSem);
    renderFinishedSem = std::move(o.renderFinishedSem);
    inFlightFrame = std::move(o.inFlightFrame);
    inFlightImage = std::move(o.inFlightImage);
    o.device = VK_NULL_HANDLE;
    o.maxFrames = 0;
    o.frameIndex = 0;
    return *this;
}

FrameSync::~FrameSync() {
    destroy();
}

VkSemaphore FrameSync::imageAvailable() const {
    return imageAvailableSem[frameIndex];
}

VkSemaphore FrameSync::renderFinished() const {
    return renderFinishedSem[frameIndex];
}

VkFence FrameSync::inFlightFence() const {
    return inFlightFrame[frameIndex];
}

void FrameSync::waitForImage(uint32_t imageIndex) {
    if (inFlightImage[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &inFlightImage[imageIndex], VK_TRUE, UINT64_MAX);
    }
}

void FrameSync::markImageInFlight(uint32_t imageIndex) {
    inFlightImage[imageIndex] = inFlightFrame[frameIndex];
}

void FrameSync::advanceFrame() {
    frameIndex = (frameIndex + 1) % maxFrames;
}
