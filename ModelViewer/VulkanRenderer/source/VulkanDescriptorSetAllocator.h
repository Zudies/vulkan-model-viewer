#pragma once

namespace Vulkan {

class RendererImpl;
class VulkanDescriptorSetLayout;
class VulkanDescriptorSetInstance;

class VulkanDescriptorSetAllocator {
public:
    static const uint32_t DEFAULT_ALLOC_COUNT = 100;
public:

    VulkanDescriptorSetAllocator(RendererImpl *renderer);
    VulkanDescriptorSetAllocator(VulkanDescriptorSetAllocator const &) = delete;
    VulkanDescriptorSetAllocator &operator=(VulkanDescriptorSetAllocator const &) = delete;
    ~VulkanDescriptorSetAllocator();

    void AddDescriptorLayout(VulkanDescriptorSetLayout *layout, uint32_t allocCount = DEFAULT_ALLOC_COUNT);

    Graphics::GraphicsError Initialize();

    // Allocates and initializes all descriptor sets in inOutDescriptorSets
    Graphics::GraphicsError AllocateDescriptorSet(uint32_t count, VulkanDescriptorSetInstance *inOutDescriptorSets);
    Graphics::GraphicsError AllocateDescriptorSet(uint32_t count, VulkanDescriptorSetInstance **inOutDescriptorSets);

    // Resets all resources that have been allocated from this pool
    // All descriptor set instances that were allocated from this pool will be in an unallocated state
    void Reset();

    // Releases and destroys resources used by the pool
    void Clear();

private:
    // Returned pool will be removed from the free pool list and added to the active pool list
    Graphics::GraphicsError _getFreePool();
    Graphics::GraphicsError _allocateVkDescriptorSets(uint32_t count);

private:
    RendererImpl *m_renderer;

    typedef std::vector<VkDescriptorPool> DescriptorPoolArray;
    DescriptorPoolArray m_activePools;
    DescriptorPoolArray m_freePools;

    std::vector<VkDescriptorPoolSize> m_descriptorPoolSizes;
    uint32_t m_expectedAllocs;
    
    // Kept in object to avoid reallocating the vector each allocation
    std::vector<VkDescriptorSetLayout> m_setLayouts;
    std::vector<VkDescriptorSet> m_allocatedSets;
};

} // namespace Vulkan
