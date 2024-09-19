#pragma once

namespace Vulkan {

class RendererImpl;

class VulkanDescriptorSetLayout {
public:

    VulkanDescriptorSetLayout(RendererImpl *renderer);
    VulkanDescriptorSetLayout(VulkanDescriptorSetLayout const &) = delete;
    VulkanDescriptorSetLayout &operator=(VulkanDescriptorSetLayout const &) = delete;
    ~VulkanDescriptorSetLayout();

    // Adds a descriptor to the set
    // Once a descriptor is added, it cannot be removed
    //TODO: Add types as necessary
    void AddUniformBuffer(uint32_t binding, uint32_t count, VkShaderStageFlags shaderStages);
    void AddCombinedImageSampler(uint32_t binding, uint32_t count, VkShaderStageFlags shaderStages);

    // Once a layout has been successfully initialized, Initialize can no longer be called
    Graphics::GraphicsError Initialize();

    VkDescriptorSetLayout GetVkLayout() const;

private:
    RendererImpl *m_renderer;
    VkDescriptorSetLayout m_vkLayout;

    std::vector<VkDescriptorSetLayoutBinding> m_bindings;
};

} // namespace Vulkan
