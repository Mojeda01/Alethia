#pragma once
 
#include <vulkan/vulkan.h>
 
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
 
#include <cstdint>
#include <vector>

class UniformBuffer{
public:
    struct MVPData{
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec4 lightPos;
        glm::vec4 viewPos;
    };
    UniformBuffer(  VkDevice device,
                    VkPhysicalDevice physicalDevice,
                    uint32_t frameCount);
    ~UniformBuffer();

    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;
    UniformBuffer(UniformBuffer&&) noexcept;
    UniformBuffer& operator=(UniformBuffer&&) noexcept;
    void update(uint32_t frameIndex, const MVPData& data);
    void bindTexture(VkImageView textureView, VkSampler textureSampler);
    VkDescriptorSetLayout descriptorSetLayout() const { return layout; }
    VkDescriptorSet descriptorSet(uint32_t frameIndex) const { return sets[frameIndex]; }
private:
    void destroy() noexcept;
    static uint32_t findMemoryType(VkPhysicalDevice physicalDevice,
                                    uint32_t typeFilter,
                                    VkMemoryPropertyFlags properties);
    VkDevice device = VK_NULL_HANDLE;
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VkDescriptorPool pool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> sets;
    std::vector<VkBuffer> buffers;
    std::vector<VkDeviceMemory> memories;
    std::vector<void*> mappedPtrs;
    uint32_t frameCount = 0;
};
