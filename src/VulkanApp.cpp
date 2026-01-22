#include "VulkanApp.h"
#include <iostream>
#include <stdexcept>

VulkanApp::VulkanApp(int width, int height, const char* title)
    : window(width, height, title)
    , instance()
    , surface(instance.get(), window.get())
    , device(instance.get(), surface.get())
    , swapchainBundle(  device.physical(),
                        device.get(),
                        surface.get(),
                        (uint32_t)width,
                        (uint32_t)height)
    , triangle(device.get(), swapchainBundle.renderPass())
    , commandPool(  device.get(),
                    device.graphicsQueueFamily(),
                    swapchainBundle.framebuffers().size())
    , sync(device.get(), (uint32_t)swapchainBundle.framebuffers().size(),
            (uint32_t)swapchainBundle.framebuffers().size()) 
{
    std::cout << "Vulkan fully initialized\n";
}

void VulkanApp::run() {
    while (!window.shouldClose()) {
        window.pollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle(device.get());
}

void VulkanApp::cleanup() {
    // nothing yet.
}

void VulkanApp::drawFrame() {
    VkDevice dev = device.get();

    // wait for the CPU/GPU sync fence for this frame
    VkFence frameFence = sync.inFlightFence();
    vkWaitForFences(dev, 1, &frameFence, VK_TRUE, UINT64_MAX);

    // get an image from swapchain.
    uint32_t imageIndex = 0;
    VkResult r = vkAcquireNextImageKHR(
        dev,
        swapchainBundle.swapchain(),
        UINT64_MAX,
        sync.imageAvailable(),
        VK_NULL_HANDLE,
        &imageIndex
    );
    if (r == VK_ERROR_OUT_OF_DATE_KHR) {
        // recreateSwapchain();
        return;
    } 
    if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("vkAcquireNextImageKHR failed");
    }

    // If that swapchain is already in flight, wait for it.
    sync.waitForImage(imageIndex);
    sync.markImageInFlight(imageIndex);

    // reset fence now fresh work for this frame is coming
    vkResetFences(dev, 1, &frameFence);

    // record clear-only render pass into the command buffer.
    VkCommandBuffer cmd = commandPool.buffers()[imageIndex];
    vkResetCommandBuffer(cmd, 0);
    recordClearCommandBuffer(cmd, imageIndex);

    // submit
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSemaphore waitSem = sync.imageAvailable();
    VkSemaphore signalSem = sync.renderFinished();

    VkSubmitInfo submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &waitSem;
    submit.pWaitDstStageMask = &waitStage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &signalSem;

    if (vkQueueSubmit(device.graphicsQueue(), 1, &submit, frameFence) != VK_SUCCESS) {
        throw std::runtime_error("vkQueueSubmit failed");
    }

    // Present
    VkSwapchainKHR sc = swapchainBundle.swapchain();
    VkPresentInfoKHR present{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &signalSem;
    present.swapchainCount = 1;
    present.pSwapchains = &sc;
    present.pImageIndices = &imageIndex;

    r = vkQueuePresentKHR(device.presentQueue(), &present);
    if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR) {
        // recreateSwapchain();
    } else if (r != VK_SUCCESS) {
        throw std::runtime_error("vkQueuePresentKHR failed");
    }

    sync.advanceFrame();
}

void VulkanApp::recordClearCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex) {
    triangle.record(cmd, swapchainBundle.framebuffers()[imageIndex],
                    swapchainBundle.extent());
}
