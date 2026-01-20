#include "VulkanApp.h"
#include <iostream>

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
    , commandPool(  device.get(),
                    device.graphicsQueueFamily(),
                    swapchainBundle.framebuffers().size())
    , sync(device.get(), (uint32_t)swapchainBundle.framebuffers().size(), MAX_FRAMES_IN_FLIGHT)
{
    std::cout << "Vulkan fully initialized\n";
}

void VulkanApp::run() {
    while (!window.shouldClose()) {
        window.pollEvents();
    }
}

void VulkanApp::cleanup() {
    // nothing yet.
}
