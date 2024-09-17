#pragma once

#include "VulkanBuffer.h"
#include "VulkanImageBuffer.h"

namespace Graphics {

class ImageLoader;

} // namespace Graphics

namespace Vulkan {

class RendererImpl;
class VulkanCommandBuffer;

// Once a texture is uploaded to the device it is immutable and the same instance of this class cannot be used to re-upload
class Vulkan2DTextureBuffer {
public:

    Vulkan2DTextureBuffer(RendererImpl *renderer);
    Vulkan2DTextureBuffer(Vulkan2DTextureBuffer const &) = delete;
    Vulkan2DTextureBuffer &operator=(Vulkan2DTextureBuffer const &) = delete;
    ~Vulkan2DTextureBuffer();

    Graphics::GraphicsError LoadImageFromFile(std::string const &filePath);
    Graphics::GraphicsError LoadImageFromMemory(void *data, size_t dataSize);

    VkImage GetDeviceImage() const;
    VkImageView GetDeviceImageView() const;

    void SetMipLevels(uint32_t mipLevels);

    Graphics::GraphicsError FlushTextureToDevice();
    void ClearHostResources();

private:
    Graphics::GraphicsError _createVkImage(Graphics::ImageLoader *loader);

    Graphics::GraphicsError _beginTransferQueueCommand(VkExtent3D extents, VulkanBuffer *srcBuffer, VulkanImageBuffer *dstBuffer, VulkanCommandBuffer *commandBuffer);
    Graphics::GraphicsError _endTransferQueueCommand(VulkanBuffer *stagingBuffer, VulkanCommandBuffer *commandBuffer);
    void _errorTransferQueueCommand(VulkanBuffer *stagingBuffer);
    Graphics::GraphicsError _beginGraphicsQueueCommand(VulkanImageBuffer *dstBuffer, VulkanCommandBuffer *commandBuffer);
    Graphics::GraphicsError _endGraphicsQueueCommand(VulkanCommandBuffer *commandBuffer);
    void _errorGraphicsQueueCommand();

private:

    RendererImpl *m_renderer;
    VulkanImageBuffer m_imageBuffer;
    VulkanBuffer m_stagingBuffer;
    VkImageView m_imageView;
    VkSemaphore m_transferSemaphore;

};

} // namespace Vulkan
