#include "pch.h"
#include "VulkanRenderPass.h"
#include "VulkanRendererImpl.h"

namespace Vulkan {

VulkanRenderPass::VulkanRenderPass(RendererImpl *renderer)
    : m_renderer(renderer),
    m_vkRenderPass(VK_NULL_HANDLE) {
    ASSERT(renderer);
}

VulkanRenderPass::~VulkanRenderPass() {
    ResetResources();
}

void VulkanRenderPass::AddAttachment(VkAttachmentDescription *attachment) {
    ASSERT(!m_vkRenderPass);

    m_attachments.emplace_back(*attachment);
}

void VulkanRenderPass::SetSubpassCount(uint32_t subpassCount) {
    ASSERT(!m_vkRenderPass);

    m_subpasses.resize(subpassCount);
    memset(m_subpasses.data(), 0, sizeof(VkSubpassDescription) * subpassCount);

    m_subpassInputAttachments.resize(subpassCount);
    m_subpassColorAttachments.resize(subpassCount);
    m_subpassResolveAttachments.resize(subpassCount);
    m_subpassDepthStencilAttachments.resize(subpassCount);
    memset(m_subpassDepthStencilAttachments.data(), 0, sizeof(VkAttachmentReference) * subpassCount);
    m_subpassPreserveAttachments.resize(subpassCount);
}

void VulkanRenderPass::SetSubpassPipelineBindpoint(uint32_t subpassIndex, VkPipelineBindPoint bindpoint) {
    ASSERT(!m_vkRenderPass);

    m_subpasses[subpassIndex].pipelineBindPoint = bindpoint;
}

void VulkanRenderPass::SetSubpassPipelineFlags(uint32_t subpassIndex, VkSubpassDescriptionFlags flags) {
    ASSERT(!m_vkRenderPass);

    m_subpasses[subpassIndex].flags |= flags;
}

void VulkanRenderPass::AddSubpassInputAttachment(uint32_t subpassIndex, uint32_t attachmentIndex, VkImageLayout attachmentLayout) {
    ASSERT(!m_vkRenderPass);

    auto &newAttachment = m_subpassInputAttachments[subpassIndex].emplace_back(VkAttachmentReference{});
    newAttachment.attachment = attachmentIndex;
    newAttachment.layout = attachmentLayout;
    m_subpasses[subpassIndex].inputAttachmentCount = static_cast<uint32_t>(m_subpassInputAttachments[subpassIndex].size());
    m_subpasses[subpassIndex].pInputAttachments = m_subpassInputAttachments[subpassIndex].data();
}

void VulkanRenderPass::AddSubpassColorAttachment(uint32_t subpassIndex,
    uint32_t colorAttachmentIndex, VkImageLayout colorAttachmentLayout,
    uint32_t resolveAttachmentIndex, VkImageLayout resolveAttachmentLayout) {
    ASSERT(!m_vkRenderPass);

    auto &newColorAttachment = m_subpassColorAttachments[subpassIndex].emplace_back(VkAttachmentReference{});
    newColorAttachment.attachment = colorAttachmentIndex;
    newColorAttachment.layout = colorAttachmentLayout;

    auto &newResolveAttachment = m_subpassResolveAttachments[subpassIndex].emplace_back(VkAttachmentReference{});
    newResolveAttachment.attachment = resolveAttachmentIndex;
    newResolveAttachment.layout = resolveAttachmentLayout;

    m_subpasses[subpassIndex].colorAttachmentCount = static_cast<uint32_t>(m_subpassColorAttachments[subpassIndex].size());
    m_subpasses[subpassIndex].pColorAttachments = m_subpassColorAttachments[subpassIndex].data();
    m_subpasses[subpassIndex].pResolveAttachments = m_subpassResolveAttachments[subpassIndex].data();
}

void VulkanRenderPass::AddSubpassDepthStencilAttachment(uint32_t subpassIndex, uint32_t attachmentIndex, VkImageLayout attachmentLayout) {
    ASSERT(!m_vkRenderPass);

    m_subpassDepthStencilAttachments[subpassIndex].attachment = attachmentIndex;
    m_subpassDepthStencilAttachments[subpassIndex].layout = attachmentLayout;
    m_subpasses[subpassIndex].pDepthStencilAttachment = &m_subpassDepthStencilAttachments[subpassIndex];
}

void VulkanRenderPass::AddSubpassPreserveAttachment(uint32_t subpassIndex, uint32_t attachmentIndex) {
    ASSERT(!m_vkRenderPass);

    m_subpassPreserveAttachments[subpassIndex].emplace_back(attachmentIndex);
    m_subpasses[subpassIndex].preserveAttachmentCount = static_cast<uint32_t>(m_subpassPreserveAttachments[subpassIndex].size());
    m_subpasses[subpassIndex].pPreserveAttachments = m_subpassPreserveAttachments[subpassIndex].data();
}

void VulkanRenderPass::AddSubpassDependency(VkSubpassDependency *dependency) {
    ASSERT(!m_vkRenderPass);

    m_subpassDependencies.emplace_back(*dependency);
}

Graphics::GraphicsError VulkanRenderPass::Initialize() {
    ASSERT(!m_vkRenderPass);

    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = static_cast<uint32_t>(m_attachments.size());
    createInfo.pAttachments = m_attachments.data();
    createInfo.subpassCount = static_cast<uint32_t>(m_subpasses.size());
    createInfo.pSubpasses = m_subpasses.data();
    createInfo.dependencyCount = static_cast<uint32_t>(m_subpassDependencies.size());
    createInfo.pDependencies = m_subpassDependencies.data();

    if (vkCreateRenderPass(m_renderer->GetDevice(), &createInfo, VK_NULL_HANDLE, &m_vkRenderPass) != VK_SUCCESS) {
        return Graphics::GraphicsError::RENDERPASS_CREATE_ERROR;
    }

    return Graphics::GraphicsError::OK;
}

VkRenderPass VulkanRenderPass::GetVkRenderPass() const {
    return m_vkRenderPass;
}

void VulkanRenderPass::ResetResources() {
    if (m_vkRenderPass) {
        vkDestroyRenderPass(m_renderer->GetDevice(), m_vkRenderPass, VK_NULL_HANDLE);
        m_vkRenderPass = VK_NULL_HANDLE;
    }
}

void VulkanRenderPass::ResetAttachments() {
    m_attachments.clear();
}

} // namespace Vulkan
