#include "pch.h"
#include "Vulkan2DTextureBuffer.h"
#include "VulkanRendererImpl.h"
#include "VulkanCommandBuffer.h"

#include "ImageLoader.h"

namespace Vulkan {

Vulkan2DTextureBuffer::Vulkan2DTextureBuffer(RendererImpl *renderer)
  : m_renderer(renderer),
    m_imageBuffer(renderer),
    m_stagingBuffer(renderer),
    m_imageView(VK_NULL_HANDLE),
    m_transferSemaphore(VK_NULL_HANDLE) {
    ASSERT(renderer);
}

Vulkan2DTextureBuffer::~Vulkan2DTextureBuffer() {
    if (m_imageView) {
        vkDestroyImageView(m_renderer->GetDevice(), m_imageView, VK_NULL_HANDLE);
    }
    if (m_transferSemaphore) {
        vkDestroySemaphore(m_renderer->GetDevice(), m_transferSemaphore, VK_NULL_HANDLE);
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

VkImage Vulkan2DTextureBuffer::GetDeviceImage() const {
    return m_imageBuffer.GetVkImage();
}

VkImageView Vulkan2DTextureBuffer::GetDeviceImageView() const {
    return m_imageView;
}

void Vulkan2DTextureBuffer::SetMipLevels(uint32_t mipLevels) {
    m_imageBuffer.SetMipLevels(mipLevels);
}

Graphics::GraphicsError Vulkan2DTextureBuffer::FlushTextureToDevice() {
    // Determine if there is a transfer queue to use
    if (!VK_BUFFERS_FORCE_NO_TRANSFER_QUEUE && (m_renderer->GetQueueIndex(RendererImpl::QUEUE_GRAPHICS) != m_renderer->GetQueueIndex(RendererImpl::QUEUE_TRANSFER))) {
        // Unique transfer queue so need to do:
        //   copy -> queue ownership transfer -> layout transitions on separate queues

        // Need a semaphore for syncing transfer and layout transition
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (vkCreateSemaphore(m_renderer->GetDevice(), &semaphoreInfo, VK_NULL_HANDLE, &m_transferSemaphore) != VK_SUCCESS) {
            return Graphics::GraphicsError::INITIALIZATION_FAILED;
        }

        // Register transfer functions for both transfer and graphics queues
        m_renderer->RegisterTransfer(
            RendererImpl::QUEUE_TRANSFER,
            1,
            std::bind(&Vulkan2DTextureBuffer::_beginTransferQueueCommand, this, m_imageBuffer.GetExtents(), &m_stagingBuffer, &m_imageBuffer, std::placeholders::_1),
            std::bind(&Vulkan2DTextureBuffer::_endTransferQueueCommand, this, &m_stagingBuffer, std::placeholders::_1),
            std::bind(&Vulkan2DTextureBuffer::_errorTransferQueueCommand, this, &m_stagingBuffer)

        );
        m_renderer->RegisterTransfer(
            RendererImpl::QUEUE_GRAPHICS,
            1,
            std::bind(&Vulkan2DTextureBuffer::_beginGraphicsQueueCommand, this, &m_imageBuffer, std::placeholders::_1),
            std::bind(&Vulkan2DTextureBuffer::_endGraphicsQueueCommand, this, std::placeholders::_1),
            std::bind(&Vulkan2DTextureBuffer::_errorGraphicsQueueCommand, this)
        );
    }
    else {
        // No transfer queue so just use graphics queue without additional syncing needed
        m_renderer->RegisterTransfer(
            RendererImpl::QUEUE_GRAPHICS,
            1,
            std::bind(&Vulkan2DTextureBuffer::_beginTransferQueueCommand, this, m_imageBuffer.GetExtents(), &m_stagingBuffer, &m_imageBuffer, std::placeholders::_1),
            std::bind(&Vulkan2DTextureBuffer::_endTransferQueueCommand, this, &m_stagingBuffer, std::placeholders::_1),
            std::bind(&Vulkan2DTextureBuffer::_errorTransferQueueCommand, this, &m_stagingBuffer)

        );
    }

    return Graphics::GraphicsError::OK;
}

void Vulkan2DTextureBuffer::ClearHostResources() {
    // Nothing to do, staging buffer is automatically cleaned up after transfer
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

    // Create the VkImage
    auto err = m_imageBuffer.Initialize(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, nullptr, 0);
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
    m_stagingBuffer.Initialize(memorySize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, nullptr, 0);
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

Graphics::GraphicsError Vulkan2DTextureBuffer::_beginTransferQueueCommand(VkExtent3D extents, VulkanBuffer *srcBuffer, VulkanImageBuffer *dstBuffer, VulkanCommandBuffer *commandBuffer) {
    auto err = commandBuffer->BeginCommandBuffer();
    if (err != Graphics::GraphicsError::OK) {
        srcBuffer->Clear();
        return err;
    }

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
        commandBuffer->GetVkCommandBuffer(),
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
        commandBuffer->GetVkCommandBuffer(),
        srcBuffer->GetVkBuffer(),
        dstBuffer->GetVkImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    // If this is on graphics queue, just transition to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    // If this is on transfer queue, need to start ownership transfer to graphics queue
    // Note: The barrier is the same regardless if just transitioning layout or if also releasing ownership
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
#if VK_BUFFERS_FORCE_NO_TRANSFER_QUEUE
    barrier.srcQueueFamilyIndex = m_renderer->GetQueueIndex(RendererImpl::QUEUE_GRAPHICS);
#else
    barrier.srcQueueFamilyIndex = m_renderer->GetQueueIndex(RendererImpl::QUEUE_TRANSFER);
#endif
    barrier.dstQueueFamilyIndex = m_renderer->GetQueueIndex(RendererImpl::QUEUE_GRAPHICS);

    // Technically dstStageMask is ignored when releasing ownership but validation will still complain about
    //   the transfer queue not supporting the fragment shader stage so this should be set to 0 for a release
    VkPipelineStageFlags dstStage = 0;
    if (VK_BUFFERS_FORCE_NO_TRANSFER_QUEUE || commandBuffer->GetQueue() == RendererImpl::QUEUE_GRAPHICS) {
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    vkCmdPipelineBarrier(
        commandBuffer->GetVkCommandBuffer(),
        VK_PIPELINE_STAGE_TRANSFER_BIT, dstStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    if (!VK_BUFFERS_FORCE_NO_TRANSFER_QUEUE && commandBuffer->GetQueue() == RendererImpl::QUEUE_TRANSFER) {
        // Need to sync transfer and graphics queues
        commandBuffer->AddSignalSemaphore(m_transferSemaphore);
    }

    err = commandBuffer->EndCommandBuffer();
    if (err != Graphics::GraphicsError::OK) {
        srcBuffer->Clear();
        return err;
    }

    err = commandBuffer->Submit();
    if (err != Graphics::GraphicsError::OK) {
        srcBuffer->Clear();
        return err;
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError Vulkan2DTextureBuffer::_endTransferQueueCommand(VulkanBuffer *stagingBuffer, VulkanCommandBuffer *commandBuffer) {
    VkFence waitFence = commandBuffer->GetWaitFence();
    vkWaitForFences(m_renderer->GetDevice(), 1, &waitFence, true, std::numeric_limits<uint64_t>::max());

    stagingBuffer->Clear();
    return Graphics::GraphicsError::OK;
}

void Vulkan2DTextureBuffer::_errorTransferQueueCommand(VulkanBuffer *stagingBuffer) {
    stagingBuffer->Clear();
}

Graphics::GraphicsError Vulkan2DTextureBuffer::_beginGraphicsQueueCommand(VulkanImageBuffer *dstBuffer, VulkanCommandBuffer *commandBuffer) {
    auto err = commandBuffer->BeginCommandBuffer();
    if (err != Graphics::GraphicsError::OK) {
        return err;
    }

    // Sync with the transfer
    commandBuffer->AddWaitSemaphore(m_transferSemaphore, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    // Acquire ownership
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = m_renderer->GetQueueIndex(RendererImpl::QUEUE_TRANSFER);
    barrier.dstQueueFamilyIndex = m_renderer->GetQueueIndex(RendererImpl::QUEUE_GRAPHICS);
    barrier.image = dstBuffer->GetVkImage();
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0; //TODO:
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(
        commandBuffer->GetVkCommandBuffer(),
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    err = commandBuffer->EndCommandBuffer();
    if (err != Graphics::GraphicsError::OK) {
        return err;
    }

    err = commandBuffer->Submit();
    if (err != Graphics::GraphicsError::OK) {
        return err;
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError Vulkan2DTextureBuffer::_endGraphicsQueueCommand(VulkanCommandBuffer *commandBuffer) {
    (void)commandBuffer;
    vkDestroySemaphore(m_renderer->GetDevice(), m_transferSemaphore, VK_NULL_HANDLE);
    return Graphics::GraphicsError::OK;
}

void Vulkan2DTextureBuffer::_errorGraphicsQueueCommand() {
    vkDestroySemaphore(m_renderer->GetDevice(), m_transferSemaphore, VK_NULL_HANDLE);
}

} // namespace Vulkan
