#pragma once

namespace Vulkan {

class RendererImpl;
class VulkanDescriptorSetLayout;

class VulkanDescriptorSetInstance {
public:
    union WriteDescriptorSetInfo {
        VkDescriptorBufferInfo bufferInfo;
        VkDescriptorImageInfo imageInfo;
        VkBufferView bufferViewInfo;
    };

public:

    VulkanDescriptorSetInstance(RendererImpl *renderer);
    VulkanDescriptorSetInstance(VulkanDescriptorSetInstance const &) = delete;
    VulkanDescriptorSetInstance &operator=(VulkanDescriptorSetInstance const &) = delete;
    ~VulkanDescriptorSetInstance();

    // This will populate one VkWriteDescriptorSet for each binding in the layout
    // Defaults:
    //   dstArrayElement = 0
    //   descriptorCount = 1
    void SetDescriptorSetLayout(VulkanDescriptorSetLayout *layout);

    // Adds a VkWriteDescriptorSet that will be appended after the default layout writes
    void AddDescriptorWrite(uint32_t binding, const VkDescriptorBufferInfo *bufferInfo, uint32_t arrayElement = 0, uint32_t descriptorCount = 1);
    void AddDescriptorWrite(uint32_t binding, const VkDescriptorImageInfo *imageInfo, uint32_t arrayElement = 0, uint32_t descriptorCount = 1);
    void AddDescriptorWrite(uint32_t binding, const VkBufferView *bufferViewInfo, uint32_t arrayElement = 0, uint32_t descriptorCount = 1);

    // Update the VkWriteDescriptorSet at the specified index
    void UpdateDescriptorWrite(uint32_t writeIndex, const VkDescriptorBufferInfo *bufferInfo, uint32_t arrayElement = 0, uint32_t descriptorCount = 1);
    void UpdateDescriptorWrite(uint32_t writeIndex, const VkDescriptorImageInfo *imageInfo, uint32_t arrayElement = 0, uint32_t descriptorCount = 1);
    void UpdateDescriptorWrite(uint32_t writeIndex, const VkBufferView *bufferViewInfo, uint32_t arrayElement = 0, uint32_t descriptorCount = 1);

    // Will set and update the descriptor set according to the VkWriteDescriptorSet that have been set
    // Descriptor sets cannot be freed so this must be called each time the associated descriptor set pool is reset to re-initialize the descriptor set
    Graphics::GraphicsError SetInternalDescriptorSet(VkDescriptorSet vkDescriptorSet);

    VulkanDescriptorSetLayout *GetDescriptorSetLayout() const;
    const VkDescriptorSet &GetVkDescriptorSet() const;

private:
    const VkDescriptorSetLayoutBinding *_findBinding(uint32_t binding);

private:
    RendererImpl *m_renderer;
    VkDescriptorSet m_vkDescriptorSet;

    VulkanDescriptorSetLayout *m_descriptorSetLayout;
    std::vector<uint8_t*> m_writeInfos;
    std::vector<VkWriteDescriptorSet> m_writeDescriptors;

};

} // namespace Vulkan
