#pragma once

namespace Vulkan {

class RendererImpl;

class VulkanDescriptorSetLayout {
public:
    typedef std::array<uint8_t, 16> KeyType;
    typedef std::vector<VkDescriptorPoolSize> MemoryRequirements;
    typedef std::vector<VkDescriptorSetLayoutBinding> BindingsArray;

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
    const KeyType &GetLayoutKey() const;
    const MemoryRequirements &GetMemoryRequirements() const;
    const BindingsArray &GetBindings() const;

private:
    int _descriptorTypeToKeyIndex(VkDescriptorType descriptorType);

private:
    RendererImpl *m_renderer;
    VkDescriptorSetLayout m_vkLayout;

    BindingsArray m_bindings;
    KeyType m_key;
    MemoryRequirements m_memoryRequirements;

};

} // namespace Vulkan
