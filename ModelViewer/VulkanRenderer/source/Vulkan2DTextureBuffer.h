#pragma once

#include "VulkanBuffer.h"
#include "VulkanImageBuffer.h"

namespace Graphics {

class ImageLoader;

} // namespace Graphics

namespace Vulkan {

class Vulkan2DTextureBuffer {
public:

    Vulkan2DTextureBuffer(RendererImpl *renderer);
    Vulkan2DTextureBuffer(Vulkan2DTextureBuffer const &) = delete;
    Vulkan2DTextureBuffer &operator=(Vulkan2DTextureBuffer const &) = delete;
    ~Vulkan2DTextureBuffer();

    Graphics::GraphicsError LoadImageFromFile(std::string const &filePath);
    Graphics::GraphicsError LoadImageFromMemory(void *data, size_t dataSize);

    VkImage GetDeviceImage();

    void SetMipLevels(uint32_t mipLevels);

    Graphics::GraphicsError FlushTextureToDevice();
    void ClearHostResources();

private:
    Graphics::GraphicsError _createVkImage(Graphics::ImageLoader *loader);
    void _beginTransferCommand(VkExtent3D extents, VulkanBuffer *srcBuffer, VulkanImageBuffer *dstBuffer, VkCommandBuffer commandBuffer);
    void _endTransferCommand(VulkanBuffer *stagingBuffer);

private:

    RendererImpl *m_renderer;
    VulkanImageBuffer m_imageBuffer;
    VulkanBuffer m_stagingBuffer;

};

} // namespace Vulkan
