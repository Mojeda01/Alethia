#include "Window.h"
#include "Instance.h"
#include "Surface.h"
#include "Device.h"
#include "Swapchain.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "CommandPool.h"

#include <iostream>

int main() {
    
    try {
        Window window(800, 600, "VulkanLab");

        Instance instance;
        Surface surface(instance.get(), window.get());
        Device device(instance.get(), surface.get());
        Swapchain swapchain(device.physical(), device.get(), surface.get(), 800, 600);
        RenderPass renderPass(device.get(), swapchain.imageFormat());
        FramebufferSet framebuffers (device.get(), renderPass.get(),
                                        swapchain.extent(), swapchain.imageViews());
        CommandPool commandPool(device.get(), device.graphicsQueueFamily(), 
                                framebuffers.get().size());


        std::cout << "Vulkan fully initialized\n";
        while (!window.shouldClose()){
            window.pollEvents();
        }
    } catch (const std::exception& e){
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}
