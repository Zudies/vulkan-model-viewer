#include "pch.h"
#include "VulkanUniformBufferObject.h"

namespace Vulkan {

VulkanUniformBufferObject::VulkanUniformBufferObject(RendererImpl *renderer)
  : m_renderer(renderer),
    m_buffer(renderer) {
    ASSERT(renderer);
}

VulkanUniformBufferObject::~VulkanUniformBufferObject() {
}

Graphics::GraphicsError VulkanUniformBufferObject::Initialize(size_t sizePerUbo, size_t numUbos) {
    auto err = m_buffer.Initialize(sizePerUbo, numUbos, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, nullptr, 0);
    if (err != Graphics::GraphicsError::OK) {
        return err;
    }

    err = m_buffer.Allocate(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (err != Graphics::GraphicsError::OK) {
        return err;
    }

    return Graphics::GraphicsError::OK;
}

void *VulkanUniformBufferObject::GetMappedMemory(size_t index) {
    return m_buffer.GetMappedMemory(index);
}

VkBuffer VulkanUniformBufferObject::GetDeviceBuffer(size_t index) {
    return m_buffer.GetVkBuffer(index);
}


} // namespace Vulkan
