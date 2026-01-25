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
    glfwSetWindowUserPointer(window.get(), this);
    glfwSetFramebufferSizeCallback(window.get(), VulkanApp::framebufferResizeCallback);

    auto now = std::chrono::steady_clock::now();
    startTime = now;
    lastFrameTime = now;
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

void VulkanApp::framebufferResizeCallback(GLFWwindow* window, int /*width*/, int /*height*/) {
    auto* app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->framebufferResized = true;
    }
}

void VulkanApp::drawFrame() {
    VkDevice dev = device.get();

    if (framebufferResized) {
        framebufferResized = false;
        recreateSwapchain();
        return;
    }

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
    bool suboptimal = (r == VK_SUBOPTIMAL_KHR);
    if (r == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
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
    
    auto now = std::chrono::steady_clock::now();
    float timeSeconds = std::chrono::duration<float>(now - startTime).count();
    float deltaSeconds = std::chrono::duration<float>(now - lastFrameTime).count();
    lastFrameTime = now;

    TriangleRenderer::PushConstants pushConstants{};
    pushConstants.timeSeconds = timeSeconds;
    pushConstants.deltaSeconds = deltaSeconds;
    pushConstants.frameIndex = frameIndex++;

    recordClearCommandBuffer(cmd, imageIndex, pushConstants);

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
    if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR || framebufferResized || suboptimal) {
        framebufferResized = false;
        recreateSwapchain();
        return;
    } else if (r != VK_SUCCESS) {
        throw std::runtime_error("vkQueuePresentKHR failed");
    }

    sync.advanceFrame();
}

void VulkanApp::recordClearCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex, const TriangleRenderer::PushConstants& pushConstants) {
    triangle.record(cmd, swapchainBundle.framebuffers()[imageIndex],
                    swapchainBundle.extent(),
                    pushConstants);
}

void VulkanApp::recreateSwapchain() {
    auto fb = window.framebufferSize();
    while (fb.first == 0 || fb.second == 0) {
        window.pollEvents();
        fb = window.framebufferSize();
    }

    vkDeviceWaitIdle(device.get());

    swapchainBundle.recreate(
        device.physical(),
        device.get(),
        surface.get(),
        static_cast<uint32_t>(fb.first),
        static_cast<uint32_t>(fb.second)
    );
}
