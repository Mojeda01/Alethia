#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"
#include "TextureImage.h"

#include <stdexcept>
#include <utility>
#include <cstring>

uint32_t TextureImage::findMemoryType(
    VkPhysicalDevice physicalDevice,
    uint32_t typeFilter,
    VkMemoryPropertyFlags properties
) {
    VkPhysicalDeviceMemoryProperties memProps{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) && 
                (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("TextureImage: failed to find suitable memory type");
}

void TextureImage::createSampler() {
    VkSamplerCreateInfo sci{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    sci.magFilter = VK_FILTER_LINEAR;
    sci.minFilter = VK_FILTER_LINEAR;
    sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sci.anisotropyEnable = VK_FALSE;
    sci.maxAnisotropy = 1.0f;
    sci.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sci.unnormalizedCoordinates = VK_FALSE;
    sci.compareEnable = VK_FALSE;
    sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sci.mipLodBias = 0.0f;
    sci.minLod = 0.0f;
    sci.maxLod = 0.0f;

    if (vkCreateSampler(device, &sci, nullptr, &texSampler) != VK_SUCCESS) {
        throw std::runtime_error("TextureImage: vkCreateSampler failed");
    }
}

void TextureImage::createFromPixels(
    VkDevice dev,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue transferQueue,
    const unsigned char* pixels,
    int width, int height
) 
{
    device = dev;
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(width * height * 4);
    VkBufferCreateInfo bci{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bci.size = imageSize;
    bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    if (vkCreateBuffer(device, &bci, nullptr, &stagingBuffer) != VK_SUCCESS) {
        throw std::runtime_error("TextureImage: staging vkCreateBuffer failed");
    }

    VkMemoryRequirements stagingReqs{};
    vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingReqs);

    VkMemoryAllocateInfo stagingAi{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    stagingAi.allocationSize = stagingReqs.size;
    stagingAi.memoryTypeIndex = findMemoryType(physicalDevice, stagingReqs.memoryTypeBits,
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VkDeviceMemory stagingMem = VK_NULL_HANDLE;
    if (vkAllocateMemory(device, &stagingAi, nullptr, &stagingMem) != VK_SUCCESS) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        throw std::runtime_error("TextureImage: staging vkAllocateMemory failed");
    }

    if (vkBindBufferMemory(device, stagingBuffer, stagingMem, 0) != VK_SUCCESS) {
        vkFreeMemory(device, stagingMem, nullptr);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        throw std::runtime_error("TextureImage: staging vkBindBufferMemory failed");
    }

    void* mapped = nullptr;
    if (vkMapMemory(device, stagingMem, 0, imageSize, 0, &mapped) != VK_SUCCESS) {
        vkFreeMemory(device, stagingMem, nullptr);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        throw std::runtime_error("TextureImage: staging vkMapMemory failed");
    }
    std::memcpy(mapped, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingMem);

    VkImageCreateInfo ici{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.format = VK_FORMAT_R8G8B8A8_SRGB;
    ici.extent.width = static_cast<uint32_t>(width);
    ici.extent.height = static_cast<uint32_t>(height);
    ici.extent.depth = 1;
    ici.mipLevels = 1;
    ici.arrayLayers = 1;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(device, &ici, nullptr, &image) != VK_SUCCESS) {
        vkFreeMemory(device, stagingMem, nullptr);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        throw std::runtime_error("TextureImage: vkCreateImage failed");
    }

    VkMemoryRequirements imgReqs{};
    vkGetImageMemoryRequirements(device, image, &imgReqs);
    VkMemoryAllocateInfo imgAi{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    imgAi.allocationSize = imgReqs.size;
    imgAi.memoryTypeIndex = findMemoryType(physicalDevice, imgReqs.memoryTypeBits,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &imgAi, nullptr, &mem) != VK_SUCCESS) {
        vkDestroyImage(device, image, nullptr);
        image = VK_NULL_HANDLE;
        vkFreeMemory(device, stagingMem, nullptr);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        throw std::runtime_error("TextureImage: vkAllocateMemory failed");
    }

    if (vkBindImageMemory(device, image, mem, 0) != VK_SUCCESS) {
        vkFreeMemory(device, mem, nullptr);
        vkDestroyImage(device, image, nullptr);
        mem = VK_NULL_HANDLE;
        image = VK_NULL_HANDLE;
        vkFreeMemory(device, stagingMem, nullptr);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        throw std::runtime_error("TextureImage: vkBindImageMemory failed");
    }

    VkCommandBufferAllocateInfo cbai{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cbai.commandPool = commandPool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(device, &cbai, &cmd) != VK_SUCCESS) {
        vkFreeMemory(device, stagingMem, nullptr);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        throw std::runtime_error("TextureImage: vkAllocateCommandBuffers failed");
    }

    VkCommandBufferBeginInfo cbbi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &cbbi);

    VkImageMemoryBarrier toTransfer{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.image = image;
    toTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toTransfer.subresourceRange.baseMipLevel = 0;
    toTransfer.subresourceRange.levelCount = 1;
    toTransfer.subresourceRange.baseArrayLayer = 0;
    toTransfer.subresourceRange.layerCount = 1;
    toTransfer.srcAccessMask = 0;
    toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &toTransfer);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };

    vkCmdCopyBufferToImage(cmd, stagingBuffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VkImageMemoryBarrier toShader{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    toShader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toShader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toShader.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toShader.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toShader.image = image;
    toShader.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toShader.subresourceRange.baseMipLevel = 0;
    toShader.subresourceRange.levelCount = 1;
    toShader.subresourceRange.baseArrayLayer = 0;
    toShader.subresourceRange.layerCount = 1;
    toShader.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toShader.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &toShader);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo si{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd;

    vkQueueSubmit(transferQueue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(transferQueue);
    vkFreeCommandBuffers(device, commandPool, 1, &cmd);

    vkFreeMemory(device, stagingMem, nullptr);
    vkDestroyBuffer(device, stagingBuffer, nullptr);

    VkImageViewCreateInfo vci{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    vci.image = image;
    vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vci.format = VK_FORMAT_R8G8B8A8_SRGB;
    vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vci.subresourceRange.baseMipLevel = 0;
    vci.subresourceRange.levelCount = 1;
    vci.subresourceRange.baseArrayLayer = 0;
    vci.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &vci, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("TextureImage: vkCreateImageView failed");
    }

    createSampler();
}

TextureImage::TextureImage(
    VkDevice dev,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue transferQueue,
    const std::string& filePath
)
{
    int w = 0, h = 0, channels = 0;
    unsigned char* pixels = stbi_load(filePath.c_str(), &w, &h, &channels, STBI_rgb_alpha);

    if (!pixels) {
        unsigned char white[4] = { 255, 255, 255, 255 };
        createFromPixels(dev, physicalDevice, commandPool, transferQueue, white, 1, 1);
        return;
    }

    createFromPixels(dev, physicalDevice, commandPool, transferQueue, pixels, w, h);
    stbi_image_free(pixels);
}

TextureImage::TextureImage(VkDevice dev,
                            VkPhysicalDevice physicalDevice,
                            VkCommandPool commandPool,
                            VkQueue transferQueue,
                            const unsigned char* pixels,
                            int width, int height){
    if (!pixels || width <= 0 || height <= 0) {
        throw std::invalid_argument("TextureImage: invalid pixel data");
    }
    createFromPixels(dev, physicalDevice, commandPool, transferQueue, pixels, width, height);
}

TextureImage::TextureImage(VkDevice dev,
                           VkPhysicalDevice physicalDevice) {
    device = dev;

    VkImageCreateInfo ici{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.format = VK_FORMAT_R8G8B8A8_SRGB;
    ici.extent = { 1, 1, 1 };
    ici.mipLevels = 1;
    ici.arrayLayers = 1;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(device, &ici, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("TextureImage: vkCreateImage failed (fallback)");
    }

    VkMemoryRequirements imgReqs{};
    vkGetImageMemoryRequirements(device, image, &imgReqs);

    VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ai.allocationSize = imgReqs.size;
    ai.memoryTypeIndex = findMemoryType(physicalDevice, imgReqs.memoryTypeBits,
                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &ai, nullptr, &mem) != VK_SUCCESS) {
        vkDestroyImage(device, image, nullptr);
        image = VK_NULL_HANDLE;
        throw std::runtime_error("TextureImage: vkAllocateMemory failed (fallback)");
    }

    vkBindImageMemory(device, image, mem, 0);

    VkImageViewCreateInfo vci{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    vci.image = image;
    vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vci.format = VK_FORMAT_R8G8B8A8_SRGB;
    vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vci.subresourceRange.baseMipLevel = 0;
    vci.subresourceRange.levelCount = 1;
    vci.subresourceRange.baseArrayLayer = 0;
    vci.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &vci, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("TextureImage: vkCreateImageView failed (fallback)");
    }

    createSampler();
}

void TextureImage::destroy() noexcept {
    if (device != VK_NULL_HANDLE) {
        if (texSampler != VK_NULL_HANDLE) vkDestroySampler(device, texSampler, nullptr);
        if (imageView != VK_NULL_HANDLE) vkDestroyImageView(device, imageView, nullptr);
        if (image != VK_NULL_HANDLE) vkDestroyImage(device, image, nullptr);
        if (mem != VK_NULL_HANDLE) vkFreeMemory(device, mem, nullptr);
    }
    texSampler = VK_NULL_HANDLE;
    imageView = VK_NULL_HANDLE;
    image = VK_NULL_HANDLE;
    mem = VK_NULL_HANDLE;
    device = VK_NULL_HANDLE;
}

TextureImage::TextureImage(TextureImage&& o) noexcept
    : device(o.device),
      image(o.image),
      mem(o.mem),
      imageView(o.imageView),
      texSampler(o.texSampler)
{
    o.device = VK_NULL_HANDLE;
    o.image = VK_NULL_HANDLE;
    o.mem = VK_NULL_HANDLE;
    o.imageView = VK_NULL_HANDLE;
    o.texSampler = VK_NULL_HANDLE;
}

TextureImage& TextureImage::operator=(TextureImage&& o) noexcept {
    if (this == &o) return *this;
    destroy();
    device = o.device;
    image = o.image;
    mem = o.mem;
    imageView = o.imageView;
    texSampler = o.texSampler;
    o.device = VK_NULL_HANDLE;
    o.image = VK_NULL_HANDLE;
    o.mem = VK_NULL_HANDLE;
    o.imageView = VK_NULL_HANDLE;
    o.texSampler = VK_NULL_HANDLE;
    return *this;
}

TextureImage::~TextureImage() {
    destroy();
}
