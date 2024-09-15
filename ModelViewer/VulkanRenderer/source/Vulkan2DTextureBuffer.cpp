#include "pch.h"
#include "Vulkan2DTextureBuffer.h"
#include "VulkanRendererImpl.h"

#include "ImageLoader.h"

namespace Vulkan {

Vulkan2DTextureBuffer::Vulkan2DTextureBuffer(RendererImpl *renderer)
  : m_renderer(renderer),
    m_imageBuffer(renderer),
    m_stagingBuffer(renderer),
    m_imageView(VK_NULL_HANDLE) {
    ASSERT(renderer);
}

Vulkan2DTextureBuffer::~Vulkan2DTextureBuffer() {
    if (m_imageView) {
        vkDestroyImageView(m_renderer->GetDevice(), m_imageView, VK_NULL_HANDLE);
    }
}

Graphics::GraphicsError Vulkan2DTextureBuffer::LoadImageFromFile(std::string const &filePath) {
    Graphics::ImageLoader imageLoader;
    if (!imageLoader.LoadImageFromFile(filePath, 4)) {
        return Graphics::GraphicsError::FILE_LOAD_ERROR;
    }

    return _createVkImage(&imageLoader);
}

Graphics::GraphicsError Vulkan2DTextureBuffer::LoadImageFromMemory(void *data, size_t dataSize) {
    Graphics::ImageLoader imageLoader;
    if (!imageLoader.LoadImageFromMemory(data, dataSize, 4)) {
        return Graphics::GraphicsError::FILE_LOAD_ERROR;
    }

    return _createVkImage(&imageLoader);
}

VkImage Vulkan2DTextureBuffer::GetDeviceImage() {
    return m_imageBuffer.GetVkImage();
}

VkImageView Vulkan2DTextureBuffer::GetDeviceImageView() {
    return m_imageView;
}

void Vulkan2DTextureBuffer::SetMipLevels(uint32_t mipLevels) {
    m_imageBuffer.SetMipLevels(mipLevels);
}

Graphics::GraphicsError Vulkan2DTextureBuffer::FlushTextureToDevice() {
    // Register the transfer to run on the next frame update
    m_renderer->RegisterTransfer(
#if VK_BUFFERS_USE_TRANSFER_QUEUE
        RendererImpl::QUEUE_TRANSFER,
#else
        RendererImpl::QUEUE_GRAPHICS,
#endif
        std::bind(&Vulkan2DTextureBuffer::_beginTransferCommand, this, m_imageBuffer.GetExtents(), &m_stagingBuffer, &m_imageBuffer, std::placeholders::_1),
        std::bind(&Vulkan2DTextureBuffer::_endTransferCommand, this, &m_stagingBuffer)
    );

    return Graphics::GraphicsError::OK;
}

void Vulkan2DTextureBuffer::ClearHostResources() {
}

Graphics::GraphicsError Vulkan2DTextureBuffer::_createVkImage(Graphics::ImageLoader *loader) {
    m_imageBuffer.SetExtents(loader->GetWidth(), loader->GetHeight(), loader->GetDepth());

    switch (loader->GetChannels()) {
    case 1:
        m_imageBuffer.SetFormat(VK_FORMAT_R8_SRGB);
        break;
    case 2:
        m_imageBuffer.SetFormat(VK_FORMAT_R8G8_SRGB);
        break;
    case 3:
        m_imageBuffer.SetFormat(VK_FORMAT_R8G8B8_SRGB);
        break;
    case 4:
        m_imageBuffer.SetFormat(VK_FORMAT_R8G8B8A8_SRGB);
        break;
    default:
        return Graphics::GraphicsError::UNSUPPORTED_FORMAT;
    }

#if VK_BUFFERS_USE_TRANSFER_QUEUE
    uint32_t queueFamilies[] = { m_renderer->GetQueueIndex(RendererImpl::QUEUE_GRAPHICS), m_renderer->GetQueueIndex(RendererImpl::QUEUE_TRANSFER) };
    uint32_t queueFamilyCount = queueFamilies[0] == queueFamilies[1] ? 1 : 2;
#else
    uint32_t *queueFamilies = nullptr;
    uint32_t queueFamilyCount = 0;
#endif

    // Create the VkImage
    auto err = m_imageBuffer.Initialize(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, queueFamilies, queueFamilyCount);
    if (err != Graphics::GraphicsError::OK) {
        return err;
    }

    err = m_imageBuffer.Allocate(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (err != Graphics::GraphicsError::OK) {
        return err;
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_imageBuffer.GetVkImage();
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_imageBuffer.GetFormat();
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0; //TODO:
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(m_renderer->GetDevice(), &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }

    // Create and copy into staging buffer
    VkDeviceSize memorySize = loader->GetWidth() * loader->GetHeight() * loader->GetDepth() * loader->GetChannels();
    m_stagingBuffer.Clear();
    m_stagingBuffer.Initialize(memorySize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, queueFamilies, queueFamilyCount);
    m_stagingBuffer.Allocate(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *mappedMemory = m_stagingBuffer.GetMappedMemory();
    if (!mappedMemory) {
        m_stagingBuffer.Clear();
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }
    memcpy(mappedMemory, loader->GetData(), memorySize);
    m_stagingBuffer.UnmapMemory();

    return Graphics::GraphicsError::OK;
}

void Vulkan2DTextureBuffer::_beginTransferCommand(VkExtent3D extents, VulkanBuffer *srcBuffer, VulkanImageBuffer *dstBuffer, VkCommandBuffer commandBuffer) {
    // Layout transition VK_IMAGE_LAYOUT_UNDEFINED -> VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = dstBuffer->GetVkImage();
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0; //TODO:
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    // Copy command
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = extents;

    vkCmdCopyBufferToImage(
        commandBuffer,
        srcBuffer->GetVkBuffer(),
        dstBuffer->GetVkImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    // Layout transition VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL -> VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}

void Vulkan2DTextureBuffer::_endTransferCommand(VulkanBuffer *stagingBuffer) {
    stagingBuffer->Clear();
}

} // namespace Vulkan
