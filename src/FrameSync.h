#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class FrameSync{
public:
    FrameSync(VkDevice device, uint32_t swapImageCount, uint32_t maxFrames);
    ~FrameSync();
    
    FrameSync(const FrameSync&) = delete;
    FrameSync& operator=(const FrameSync&) = delete;
    FrameSync(FrameSync&&) noexcept;
    FrameSync& operator=(FrameSync&&) noexcept;

    uint32_t currentFrame() const { return frameIndex; }

    VkSemaphore imageAvailable() const;
    VkSemaphore renderFinished() const;
    VkFence inFlightFence() const;

    void advanceFrame();

    void waitForImage(uint32_t imageIndex);
    void markImageInFlight(uint32_t imageIndex);
private:
    void destroy() noexcept;
    VkDevice device;
    uint32_t maxFrames;
    uint32_t frameIndex = 0;

    std::vector<VkSemaphore> imageAvailableSem;
    std::vector<VkSemaphore> renderFinishedSem;
    std::vector<VkFence> inFlightFrame;
    std::vector<VkFence> inFlightImage;
};
