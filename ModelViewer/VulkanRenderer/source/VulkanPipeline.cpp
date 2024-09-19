#include "pch.h"
#include "VulkanPipeline.h"
#include "VulkanRendererImpl.h"

#include "VulkanShaderModule.h"
#include "VulkanRenderPass.h"
#include "VulkanDescriptorSetLayout.h"

namespace Vulkan {

// Local flags
static const uint32_t VULKAN_PIPELINE_FLAG_DIRTY = 0x0001;

VulkanPipeline::VulkanPipeline(RendererImpl *renderer)
  : m_renderer(renderer),
    m_vkPipeline(VK_NULL_HANDLE),
    m_createFlags(0),
    m_shaderStages{},
    m_inputAssemblyTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
    m_inputAssemblyPrimitiveRestart(false),
    m_viewportData{},
    m_scissorData{},
    m_rasterizerState{},
    m_multisampleState{},
    m_depthStencilState{},
    m_colorBlendState{},
    m_renderPass(VK_NULL_HANDLE),
    m_subpassIndex(0),
    m_vkPipelineLayout(VK_NULL_HANDLE) {
    ASSERT(renderer);

    m_rasterizerState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m_rasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
    m_rasterizerState.cullMode = VK_CULL_MODE_NONE;
    m_rasterizerState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    //TODO:
    m_multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    m_multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    m_multisampleState.minSampleShading = 1.0f;

    m_depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    m_depthStencilState.depthCompareOp = VK_COMPARE_OP_NEVER;
    m_depthStencilState.maxDepthBounds = 1.0f;

    m_colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    m_colorBlendState.logicOp = VK_LOGIC_OP_CLEAR;

    m_descriptorSetLayouts.resize(4);
    memset(m_descriptorSetLayouts.data(), 0, sizeof(VkDescriptorSetLayout) * m_descriptorSetLayouts.size());
}

VulkanPipeline::~VulkanPipeline() {
    ClearResources();
    ResetPipelineLayout();
}

void VulkanPipeline::SetPipelineFlags(VkPipelineCreateFlags flags) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    m_createFlags |= flags;
}

void VulkanPipeline::SetVertexInput(uint32_t bindingCount, VkVertexInputBindingDescription *bindings, uint32_t attributeCount, VkVertexInputAttributeDescription *attributes) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    m_vertexBindings.resize(bindingCount);
    memcpy(m_vertexBindings.data(), bindings, sizeof(VkVertexInputBindingDescription) * bindingCount);

    m_vertexAttributes.resize(attributeCount);
    memcpy(m_vertexAttributes.data(), attributes, sizeof(VkVertexInputAttributeDescription) * attributeCount);
}

void VulkanPipeline::SetInputTopology(VkPrimitiveTopology topology) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    m_inputAssemblyTopology = topology;
}

void VulkanPipeline::SetInputPrimitiveRestart(bool primitiveRestartEnable) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    m_inputAssemblyPrimitiveRestart = primitiveRestartEnable;
}

void VulkanPipeline::SetShaderStage(VulkanShaderModule *shader, const char *entryFunc) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    auto foundShader = std::find_if(m_shaderStages.begin(), m_shaderStages.end(),
        [shader](const VkPipelineShaderStageCreateInfo &it) {
            return it.stage == shader->GetShaderStage();
        });
    if (foundShader == m_shaderStages.end()) {
        m_shaderEntryFuncNames.emplace_back(entryFunc);
        auto &shaderStageCreateInfo = m_shaderStages.emplace_back(VkPipelineShaderStageCreateInfo{});
        shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCreateInfo.stage = shader->GetShaderStage();
        shaderStageCreateInfo.module = shader->GetShaderModule();
    }
    else {
        m_shaderEntryFuncNames[std::distance(m_shaderStages.begin(), foundShader)] = entryFunc;
        foundShader->module = shader->GetShaderModule();
    }
}

void VulkanPipeline::SetDescriptorSet(uint32_t descriptorSetIndex, VulkanDescriptorSetLayout *descriptorSet) {
    m_descriptorSetLayouts[descriptorSetIndex] = descriptorSet ? descriptorSet->GetVkLayout() : VK_NULL_HANDLE;
}

void VulkanPipeline::AddPushConstantRange(uint32_t offset, uint32_t size, VkShaderStageFlags shaderStages) {
    m_pushConstantRanges.emplace_back(VkPushConstantRange{ shaderStages, offset, size });
}

Graphics::GraphicsError VulkanPipeline::CreatePipelineLayout() {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    VkPipelineLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size());
    createInfo.pSetLayouts = m_descriptorSetLayouts.data();
    createInfo.pushConstantRangeCount = static_cast<uint32_t>(m_pushConstantRanges.size());
    createInfo.pPushConstantRanges = m_pushConstantRanges.data();

    if (vkCreatePipelineLayout(m_renderer->GetDevice(), &createInfo, nullptr, &m_vkPipelineLayout) != VK_SUCCESS) {
        return Graphics::GraphicsError::DESCRIPTOR_SET_CREATE_ERROR;
    }

    return Graphics::GraphicsError::OK;
}

void VulkanPipeline::ResetPipelineLayout() {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    m_pushConstantRanges.clear();
    if (m_vkPipelineLayout) {
        vkDestroyPipelineLayout(m_renderer->GetDevice(), m_vkPipelineLayout, VK_NULL_HANDLE);
    }
}

void VulkanPipeline::SetDynamicStates(uint32_t dynamicStateCount, VkDynamicState *dynamicStates) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    m_dynamicStatesBuffer.resize(dynamicStateCount);
    memcpy(m_dynamicStatesBuffer.data(), dynamicStates, sizeof(VkDynamicState) * dynamicStateCount);
}

void VulkanPipeline::SetViewport(VkViewport viewport) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    m_viewportData = viewport;

    if (m_scissorData.extent.width == 0 && m_scissorData.extent.height == 0) {
        m_scissorData.extent.width = static_cast<uint32_t>(viewport.width);
        m_scissorData.extent.height = static_cast<uint32_t>(viewport.height);
        m_scissorData.offset.x = static_cast<int32_t>(viewport.x);
        m_scissorData.offset.y = static_cast<int32_t>(viewport.y);
    }
}

void VulkanPipeline::SetScissor(VkRect2D scissor) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    m_scissorData = scissor;
}

void VulkanPipeline::SetDepthClampEnable(bool depthClampEnable) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    m_rasterizerState.depthClampEnable = depthClampEnable;
}

void VulkanPipeline::SetRasterizerDiscardEnable(bool rasterizerDiscardEnable) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    m_rasterizerState.rasterizerDiscardEnable = rasterizerDiscardEnable;
}

void VulkanPipeline::SetPolygonMode(VkPolygonMode polygonMode) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    m_rasterizerState.polygonMode = polygonMode;
}

void VulkanPipeline::SetCullMode(VkCullModeFlagBits cullMode, VkFrontFace frontFace) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    m_rasterizerState.cullMode = cullMode;
    m_rasterizerState.frontFace = frontFace;
}

void VulkanPipeline::SetDepthBias(bool enable, float constantFactor, float clamp, float slopeFactor) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    m_rasterizerState.depthBiasEnable = enable;
    m_rasterizerState.depthBiasConstantFactor = constantFactor;
    m_rasterizerState.depthBiasClamp = clamp;
    m_rasterizerState.depthBiasSlopeFactor = slopeFactor;

}

void VulkanPipeline::SetLineWidth(float lineWidth) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    m_rasterizerState.lineWidth = lineWidth;
}

void VulkanPipeline::SetRenderPass(VulkanRenderPass *renderPass, uint32_t subpassIndex) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    m_renderPass = renderPass->GetVkRenderPass();
    m_subpassIndex = subpassIndex;
}

void VulkanPipeline::SetDepthTest(bool testEnable, bool writeEnable, VkCompareOp compareOp) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    m_depthStencilState.depthTestEnable = testEnable;
    m_depthStencilState.depthWriteEnable = writeEnable;
    m_depthStencilState.depthCompareOp = compareOp;
}

void VulkanPipeline::SetDepthBoundsTest(bool boundsTestEnable, float minBounds, float maxBounds) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    m_depthStencilState.depthBoundsTestEnable = boundsTestEnable;
    m_depthStencilState.minDepthBounds = minBounds;
    m_depthStencilState.maxDepthBounds = maxBounds;
}

void VulkanPipeline::SetStencilTest(bool testEnable, const VkStencilOpState &front, const VkStencilOpState &back) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    m_depthStencilState.stencilTestEnable = testEnable;
    m_depthStencilState.front = front;
    m_depthStencilState.back = back;
}

void VulkanPipeline::SetColorBlendAttachment(uint32_t attachmentIndex, VkPipelineColorBlendAttachmentState *blendState) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    if (m_colorBlendAttachments.size() < (attachmentIndex + 1)) {
        m_colorBlendAttachments.resize(attachmentIndex + 1);
    }
    m_colorBlendAttachments[attachmentIndex] = *blendState;
    m_colorBlendState.attachmentCount = static_cast<uint32_t>(m_colorBlendAttachments.size());
    m_colorBlendState.pAttachments = m_colorBlendAttachments.data();
}

void VulkanPipeline::SetColorBlendLogicOp(bool logicOpEnable, VkLogicOp logicOp) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    m_colorBlendState.logicOpEnable = logicOpEnable;
    m_colorBlendState.logicOp = logicOp;
}

void VulkanPipeline::SetColorBlendConstants(float r, float g, float b, float a) {
    m_flags.SetFlag(VULKAN_PIPELINE_FLAG_DIRTY);

    m_colorBlendState.blendConstants[0] = r;
    m_colorBlendState.blendConstants[1] = g;
    m_colorBlendState.blendConstants[2] = b;
    m_colorBlendState.blendConstants[3] = a;
}

Graphics::GraphicsError VulkanPipeline::CreatePipeline(VkPipeline *out) {
    if (m_vkPipeline && !m_flags.GetFlag(VULKAN_PIPELINE_FLAG_DIRTY)) {
        *out = m_vkPipeline;
        return Graphics::GraphicsError::OK;
    }

    // No pipeline yet so create one
    VkGraphicsPipelineCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    // Set shader entry funcs once we know the entry func names vector won't change anymore
    for (size_t i = 0; i < m_shaderStages.size(); ++i) {
        m_shaderStages[i].pName = m_shaderEntryFuncNames[i].c_str();
    }

    createInfo.flags = m_createFlags;

    createInfo.stageCount = static_cast<uint32_t>(m_shaderStages.size());
    createInfo.pStages = m_shaderStages.data();

    VkPipelineVertexInputStateCreateInfo vertexInputState{};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(m_vertexBindings.size());
    vertexInputState.pVertexBindingDescriptions = m_vertexBindings.data();
    vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertexAttributes.size());
    vertexInputState.pVertexAttributeDescriptions = m_vertexAttributes.data();
    createInfo.pVertexInputState = &vertexInputState;


    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology = m_inputAssemblyTopology;
    inputAssemblyState.primitiveRestartEnable = m_inputAssemblyPrimitiveRestart;
    createInfo.pInputAssemblyState = &inputAssemblyState;

    createInfo.pTessellationState = nullptr;

    VkPipelineViewportStateCreateInfo viewportState;
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &m_viewportData;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &m_scissorData;
    createInfo.pViewportState = &viewportState;

    createInfo.pRasterizationState = &m_rasterizerState;
    createInfo.pMultisampleState = &m_multisampleState;
    createInfo.pDepthStencilState = &m_depthStencilState;
    createInfo.pColorBlendState = &m_colorBlendState;

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(m_dynamicStatesBuffer.size());
    dynamicState.pDynamicStates = m_dynamicStatesBuffer.data();
    createInfo.pDynamicState = &dynamicState;

    createInfo.layout = m_vkPipelineLayout;
    createInfo.renderPass = m_renderPass;
    createInfo.subpass = m_subpassIndex;

    //TODO: Would a pipeline cache help here?
    if (vkCreateGraphicsPipelines(m_renderer->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_vkPipeline) != VK_SUCCESS) {
        return Graphics::GraphicsError::PIPELINE_CREATE_ERROR;
    }

    return Graphics::GraphicsError::OK;
}

void VulkanPipeline::ClearResources() {
    if (m_vkPipeline) {
        vkDestroyPipeline(m_renderer->GetDevice(), m_vkPipeline, VK_NULL_HANDLE);
    }
}

} // namespace Vulkan
