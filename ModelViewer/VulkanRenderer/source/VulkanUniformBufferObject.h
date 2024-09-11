#pragma once

#include "VulkanMultiBuffer.h"

namespace Vulkan {

class RendererImpl;

class VulkanUniformBufferObject {
public:
    VulkanUniformBufferObject(RendererImpl *renderer);
    VulkanUniformBufferObject(VulkanUniformBufferObject const &) = delete;
    VulkanUniformBufferObject &operator=(VulkanUniformBufferObject const &) = delete;
    ~VulkanUniformBufferObject();

    Graphics::GraphicsError Initialize(size_t sizePerUbo, size_t numUbos);

    void *GetMappedMemory(size_t index);
    VkBuffer GetDeviceBuffer(size_t index);

private:
    RendererImpl *m_renderer;
    VulkanMultiBuffer m_buffer;
};

} // namespace Vulkan