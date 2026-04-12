#include "VkRenderer.h"

#include <array>
#include <stdexcept>

namespace{

void waitForValidFramebufferSize(GLFWwindow* window)
{
    int width = 0;
    int height = 0;
    
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
}

void destroyDepthResources(VkDevice device, DepthResources& depth)
{
    if (depth.view != VK_NULL_HANDLE)
    {
        vkDestroyImageView(device, depth.view, nullptr);
        depth.view = VK_NULL_HANDLE;
    }
    
    if (depth.image != VK_NULL_HANDLE)
    {
        vkDestroyImage(device, depth.image, nullptr);
        depth.image = VK_NULL_HANDLE;
    }
    
    if (depth.memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, depth.memory, nullptr);
        depth.memory = VK_NULL_HANDLE;
    }
    
    depth.format = VK_FORMAT_UNDEFINED;
}

DepthResources createDepthResources(VkPhysicalDevice physicalDevice, VkDevice device, const VkExtent2D& extent)
{
    DepthResources depth{};
    depth.format = findSupportedDepthFormat(physicalDevice);
    
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = extent.width;
    imageInfo.extent.height = extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depth.format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateImage(device, &imageInfo, nullptr, &depth.image) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create depth image");
    }
    
    return depth;
}

VkCommandPool createGraphicsCommandPool(VkDevice device, uint32_t graphicsFamily)
{
    VkCommandPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = graphicsFamily;
    
    VkCommandPool pool = VK_NULL_HANDLE;
    if (vkCreateCommandPool(device, &info, nullptr, &pool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics command pool");
    }
    return pool;
}

std::array<VkCommandBuffer, Renderer::kFramesInFlight> allocateCommandBuffer(VkDevice device, VkCommandPool pool)
{
    std::array<VkCommandBuffer, Renderer::kFramesInFlight> buffers{};
    
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = Renderer::kFramesInFlight;
    
    if (vkAllocateCommandBuffers(device, &allocInfo, buffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers");
    }
    return buffers;
}

FrameSync createFrameSync (VkDevice device) {
    Framesync  sync{};
    
    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    if (vkCreateSemaphore(device, &semInfo, nullptr, &sync.imageAvailable) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semInfo, nullptr, &sync.renderFinished) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &sync.inFlight) != VK_SUCCESS)
    {
        if (sync.imageAvailable != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, sync.imageAvailable, nullptr);
        }
        if (sync.renderFinished != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, sync.renderFinished, nullptr);
        }
        if (sync.inFlight != VK_NULL_HANDLE) {
            vkDestroyFence(device, sync.inFlight, nullptr);
        }
        throw std::runtime_error("Failed to create frame synchronization objects");
    }
    return sync;
}

void destroyFrameSync (VkDevice device, FrameSync& sync) {
    if (sync.imageAvailable != VK_NULL_HANDLE) {
        vkDestroySemaphore(device, sync.imageAvailable, nullptr);
        sync.imageAvailable = VK_NULL_HANDLE;
    }
    
    if (sync.renderFinished != VK_NULL_HANDLE) {
        vkDestroySemaphore(device, sync.renderFinished, nullptr);
        sync.renderFinished = Vk_NULL_HANDLE;
    }
    
    if (sync.inFlight != VK_NULL_HANDLE) {
        vkDestroyFence(device, sync.inFlight, nullptr);
        sync.inFlight = VK_NULL_HANDLE;
    }
}

void transitionImage(
    VkCommandBuffer cmd,
    VkImage image,
    VkImageAspectFlags aspectMask,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkPipelineStageFlags2 srcStageMask,
    VkAccessFlags2 srcAccessMask,
    VkPipelineStageFlags2 dstStageMask,
    VkAccessFlags2 dstAccessMask
)
{
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStagemask = srcStageMask;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstStageMask = dstStageMask;
    barrier.dstAccessMask = dstAccessMask;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(cmd, &depInfo);
}

void recordClearPass(Renderer& renderer, VkCommandBuffer cmd, uint32_t imageIndex)
{
    const VkImage swapImage = renderer.swapchain.images[imageIndex];
    transitionImage(
        cmd,
        swapImage,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_2_NONE,
        VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
    );

    transitionImage(
        cmd,
        renderer.depth.image,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_2_NONE,
        VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | 
        VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

    VkClearValue colorClear{};
    colorClear.color = {{0.05f, 0.07f, 0.10f, 1.0f}};

    VkClearValue depthClear{};
    depthClear.depthStencil = {1.0f, 0};

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = renderer.swapchain.imageViews[imageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = colorClear;

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = renderer.depth.view;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.clearValue = depthClear;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = renderer.swapchain.extent;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthAttachment;
    renderingInfo.pStencilAttachment = nullptr;
    vkCmdBeginRendering(cmd, &renderingInfo);
    vkCmdEndRendering(cmd);

    transitionImage(
        cmd,
        swapImage,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        VK_ACCESS_2_NONE);
}

void createSwapchainDependentResources(Renderer& renderer, VkSwapchainKHR oldSwapchain)
{
    renderer.swapchain = createSwapchain(
        renderer.physicalDevice,
        renderer.logicalDevice,
        renderer.surface,
        renderer.queueFamilies,
        renderer.window,
        oldSwapchain);

    renderer.depth = createDepthResources(
        renderer.physicalDevice,
        renderer.logicalDevice.device,
        renderer.swapchain.extent);
}

void destroySwapchainDependentResources(Renderer& renderer) {
    destroyDepthResources(renderer.logicalDevice.device, renderer.depth);
    destroySwapchain(renderer.logicalDevice.device, renderer.swapchain);
}

} // namespace

Renderer createRenderer(
    VkPhysicalDevice physicalDevice,
    const LogicalDevice& logicalDevice,
    VkSurfaceKHR surface,
    const QueueFamilyIndices& queueFamilies,
    GLFWwindow* window
)
{
    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("createRenderer: physicalDevice is null");
    }

    if (logicalDevice.device == VK_NULL_HANDLE) {
        throw std::runtime_error("createRenderer: logical device is null");
    }

    if (!queueFamilies.graphics.has_value() || !queueFamilies.present.has_value()) {
        throw std::runtime_error("createRenderer: graphics/present queue families are incomplete");
    }

    if (surface == VK_NULL_HANDLE) {
        throw std::runtime_error("createRenderer: surface is null");
    }

    if (window == nullptr) {
        throw std::runtime_error("createRenderer: window is null");
    }

    Renderer renderer{};
    renderer.physicalDevice = physicalDevice;
    renderer.logicalDevice = logicalDevice;
    renderer.queueFamilies = queueFamilies;
    renderer.surface = surface;
    renderer.window = window;
    renderer.imageInFlight.fill(VK_NULL_HANDLE);

    renderer.graphicsCommandPool = createGraphicsCommandPool(renderer.logicalDevice.device, renderer.queueFamilies.graphics);

    renderer.commandBuffers = allocateCommandBuffers(renderer.logicalDevice.device, renderer.graphicsCommandPool);

    try{
        createSwapchainDependentResources(renderer, VK_NULL_HANDLE);
        for (uint32_t i = 0; i < Renderer::kFramesInFlight; ++i) {
            renderer.frames[i] = createFrameSync(renderer.logicalDevice.device);
        }
    } catch (...) {
        destroyRenderer(renderer);
        throw;
    }
    return renderer;
}

void destroyRenderer(Renderer& renderer) {
    if (renderer.logicalDevice.device == VK_NULL_HANDLE) {
        return;
    }

    vkDeviceWaitIdle(renderer.logicalDevice.device);

    for (auto& sync : renderer.frames) {
        destroyFrameSync(renderer.logicalDevice.device, sync);
    }

    destroySwapchainDependentResources(renderer);

    if (renderer.graphicsCommandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(renderer.logicalDevice.device,
                renderer.graphicsCommandPool, nullptr);
        renderer.graphicsCommandPool = VK_NULL_HANDLE;
    }

    renderer.commandBuffers.fill(VK_NULL_HANDLE);
    renderer.imageInFlight.fill(VK_NULL_HANDLE);
    renderer.currentFrame = 0;
    renderer.framebufferResized = false;
}

void recreateRendererSwapchain(Renderer& renderer) 
{
    waitForValidFramebufferSize(renderer.window);
    vkDeviceWaitIdle(renderer.logicalDevice.device);
    VkSwapchainKHR oldSwapchain = renderer.swapchain.handle;
    destroyDepthResources(renderer.logicalDevice.device, renderer.depth);

    renderer.swapchain = createSwapchain(
        renderer.physicalDevice,
        renderer.logicalDevice,
        renderer.surface,
        renderer.queueFamilies,
        renderer.window,
        oldSwapchain
    );

    renderer.depth = createDepthResources(
        renderer.physicalDevice,
        renderer.logicalDevice.device,
        renderer.swapchain.extent
    );

    if (oldSwapchain != VK_NULL_HANDLE) {
        Swapchain old{};
        old.handle = oldSwapchain;
        destroySwapchain(renderer.logicalDevice.device, old);
    }

    renderer.imageInFlight.fill(VK_NULL_HANDLE);
    renderer.framebufferResized = false;
}

void drawFrame(Renderer& renderer) {
    FrameSync& frame = renderer.frames[renderer.currentFrame];
    vkWaitForFences(renderer.logicalDevice.device, 1, &frame.inFlight, VK_TRUE, UINT64_MAX);
    uint32_t imageIndex = 0;
    VkResult acquireResult = vkAcquireNextImageKHR(
        renderer.logicalDevice.device,
        renderer.swapchain.handle,
        UINT64_MAX,
        frame.imageAvailable,
        VK_NULL_HANDLE,
        &imageIndex
    );
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateRendererSwapchain(renderer);
        return;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("vkAcquireNextImageKHR failed");
    }

    if (renderer.framebufferResized) {
        recreateRendererSwapchain(renderer);
        return;
    }

    if (renderer.imageInFlight[renderer.currentFrame] != VK_NULL_HANDLE) {
        vkWaitForFences(
            renderer.logicalDevice.device,
            1,
            &renderer.imageInFlight[renderer.currentFrame],
            VK_TRUE,
            UINT64_MAX
        );
    }
    renderer.imageInFlight[renderer.currentFrame] = frame.inFlight;
    vkResetFences(renderer.logicalDevice.device, 1, &frame.inFlight);
    vkResetCommandBuffer(renderer.commandBuffers[renderer.currentFrame], 0);
    VkCommandBuffer cmd = renderer.commandBuffers[renderer.currentFrame];

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("vkBeginCommandBuffer failed");
    }

    recordClearPass(renderer, cmd, imageIndex);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("vkEndCommandBuffer failed");
    }
    
    VkSemaphoreSubmitInfo waitSemaphoreInfo{};
    waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitSemaphoreInfo.semaphore = frame.imageAvailable;
    waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    waitSemaphoreInfo.deviceIndex = 0;
    waitSemaphoreInfo.value = 0;

    VkCommandBufferSubmitInfo cmdInfo{};
    cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmdInfo.commandBuffer = cmd;
    cmdInfo.deviceMask = 0;

    VkSemaphoreSubmitInfo signalSemaphoreInfo{};
    signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalSemaphoreInfo.semaphore = frame.renderFinished;
    signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
    signalSemaphoreInfo.deviceIndex = 0;
    signalSemaphoreInfo.value = 0;

    VkSubmitInfo2 submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.waitSemaphoreInfoCount = 1;
    submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &cmdInfo;
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

    if (vkQueueSubmit2(renderer.logicalDevice.graphicsQueue, 1, &submitInfo, frame.inFlight) != VK_SUCCESS) {
        throw std::runtime_error("vkQueueSubmit2 failed");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &frame.renderFinished;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &renderer.swapchain.handle;
    presentInfo.pImageIndices = &imageIndex;

    VkResult presentResult = vkQueuePresentKHR(renderer.logicalDevice.presentQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR ||
            presentResult == VK_SUBOPTIMAL_KHR ||
            renderer.framebufferResized) {
        recreateRendererSwapchain(renderer);
    } else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("vkQueuePresentKHR failed");
    }

    renderer.currentFrame = (renderer.currentFrame + 1) %  Renderer::kFramesInFlight;
}
