#pragma once

namespace Vulkan {

class RendererImpl;

class VulkanDescriptorSetInstance {
public:

    VulkanDescriptorSetInstance(RendererImpl *renderer);
    VulkanDescriptorSetInstance(VulkanDescriptorSetInstance const &) = delete;
    VulkanDescriptorSetInstance &operator=(VulkanDescriptorSetInstance const &) = delete;
    ~VulkanDescriptorSetInstance();

private:
    RendererImpl *m_renderer;
};

} // namespace Vulkan
