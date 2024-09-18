#pragma once

namespace Vulkan {

class RendererImpl;

class VulkanRenderPass {
public:
    VulkanRenderPass(RendererImpl *renderer);
    VulkanRenderPass(VulkanRenderPass const &) = delete;
    VulkanRenderPass &operator=(VulkanRenderPass const &) = delete;
    ~VulkanRenderPass();

    void AddAttachment(VkAttachmentDescription *attachment);

    void SetSubpassCount(uint32_t subpassCount);
    void SetSubpassPipelineBindpoint(uint32_t subpassIndex, VkPipelineBindPoint bindpoint);
    void SetSubpassPipelineFlags(uint32_t subpassIndex, VkSubpassDescriptionFlags flags);
    void AddSubpassInputAttachment(uint32_t subpassIndex, uint32_t attachmentIndex, VkImageLayout attachmentLayout);
    void AddSubpassColorAttachment(uint32_t subpassIndex,
        uint32_t colorAttachmentIndex, VkImageLayout colorAttachmentLayout,
        uint32_t resolveAttachmentIndex = VK_ATTACHMENT_UNUSED, VkImageLayout resolveAttachmentLayout = VK_IMAGE_LAYOUT_UNDEFINED);
    void AddSubpassDepthStencilAttachment(uint32_t subpassIndex, uint32_t attachmentIndex, VkImageLayout attachmentLayout);
    void AddSubpassPreserveAttachment(uint32_t subpassIndex, uint32_t attachmentIndex);

    void AddSubpassDependency(VkSubpassDependency *dependency);

    Graphics::GraphicsError Initialize();

    VkRenderPass GetVkRenderPass() const;

    // Resets the device resources used while keeping the renderpass settings the same
    // This must be called after a render pass has been created to make changes to the render pass
    void ResetResources();

    // Resets the attachment settings of this render pass, allowing new ones to be added through AddAttachment
    void ResetAttachments();

private:
    RendererImpl *m_renderer;
    VkRenderPass m_vkRenderPass;

    typedef std::vector<VkAttachmentReference> AttachmentArray;
    typedef std::vector<uint32_t> PreserveAttachmentArray;
    std::vector<VkAttachmentDescription> m_attachments;
    std::vector<AttachmentArray> m_subpassInputAttachments;
    std::vector<AttachmentArray> m_subpassColorAttachments;
    std::vector<AttachmentArray> m_subpassResolveAttachments;
    AttachmentArray m_subpassDepthStencilAttachments;
    std::vector<PreserveAttachmentArray> m_subpassPreserveAttachments;
    std::vector<VkSubpassDescription> m_subpasses;
    std::vector<VkSubpassDependency> m_subpassDependencies;
};

} // namespace Vulkan
