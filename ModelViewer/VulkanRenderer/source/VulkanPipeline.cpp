#include "pch.h"
#include "VulkanPipeline.h"

#include "VulkanShaderModule.h"
#include "VulkanRenderPass.h"

namespace Vulkan {

VulkanPipeline::VulkanPipeline(RendererImpl *renderer)
  : m_renderer(renderer),
    m_createFlags(0),
    m_shaderStages{},
    m_vertexInputState{},
    m_inputAssemblyState{},
    m_viewportData{},
    m_scissorData{},
    m_viewportState{},
    m_rasterizerState{},
    m_multisampleState{},
    m_depthStencilState{},
    m_colorBlendState{},
    m_dynamicState{},
    m_renderPass(VK_NULL_HANDLE),
    m_subpassIndex(0) {
    ASSERT(renderer);

    m_vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    m_inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    m_viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    m_viewportState.viewportCount = 1;
    m_viewportState.scissorCount = 1;

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

    m_dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
}

VulkanPipeline::~VulkanPipeline() {

}

void VulkanPipeline::SetPipelineFlags(VkPipelineCreateFlags flags) {
    m_createFlags |= flags;
}

void VulkanPipeline::SetVertexInput(uint32_t bindingCount, VkVertexInputBindingDescription *bindings, uint32_t attributeCount, VkVertexInputAttributeDescription *attributes) {
    m_vertexBindings.resize(bindingCount);
    memcpy(m_vertexBindings.data(), bindings, sizeof(VkVertexInputBindingDescription) * bindingCount);
    m_vertexInputState.vertexBindingDescriptionCount = bindingCount;
    m_vertexInputState.pVertexBindingDescriptions = m_vertexBindings.data();

    m_vertexAttributes.resize(attributeCount);
    memcpy(m_vertexAttributes.data(), attributes, sizeof(VkVertexInputAttributeDescription) * attributeCount);
    m_vertexInputState.vertexAttributeDescriptionCount = attributeCount;
    m_vertexInputState.pVertexAttributeDescriptions = m_vertexAttributes.data();
}

void VulkanPipeline::SetInputTopology(VkPrimitiveTopology topology) {
    m_inputAssemblyState.topology = topology;
}

void VulkanPipeline::SetInputPrimitiveRestart(bool primitiveRestartEnable) {
    m_inputAssemblyState.primitiveRestartEnable = primitiveRestartEnable;
}

void VulkanPipeline::SetShaderStage(VulkanShaderModule *shader, const char *entryFunc) {
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

void VulkanPipeline::SetDynamicStates(uint32_t dynamicStateCount, VkDynamicState *dynamicStates) {
    m_dynamicStatesBuffer.resize(dynamicStateCount);
    memcpy(m_dynamicStatesBuffer.data(), dynamicStates, sizeof(VkDynamicState) * dynamicStateCount);
    m_dynamicState.dynamicStateCount = dynamicStateCount;
    m_dynamicState.pDynamicStates = m_dynamicStatesBuffer.data();
}

void VulkanPipeline::SetViewport(VkViewport viewport) {
    m_viewportData = viewport;
    m_viewportState.pViewports = &m_viewportData;

    if (!m_viewportState.pScissors) {
        m_scissorData.extent.width = static_cast<uint32_t>(viewport.width);
        m_scissorData.extent.height = static_cast<uint32_t>(viewport.height);
        m_scissorData.offset.x = static_cast<int32_t>(viewport.x);
        m_scissorData.offset.y = static_cast<int32_t>(viewport.y);
        m_viewportState.pScissors = &m_scissorData;
    }
}

void VulkanPipeline::SetScissor(VkRect2D scissor) {
    m_scissorData = scissor;
    m_viewportState.pScissors = &m_scissorData;
}

void VulkanPipeline::SetDepthClampEnable(bool depthClampEnable) {
    m_rasterizerState.depthClampEnable = depthClampEnable;
}

void VulkanPipeline::SetRasterizerDiscardEnable(bool rasterizerDiscardEnable) {
    m_rasterizerState.rasterizerDiscardEnable = rasterizerDiscardEnable;
}

void VulkanPipeline::SetPolygonMode(VkPolygonMode polygonMode) {
    m_rasterizerState.polygonMode = polygonMode;
}

void VulkanPipeline::SetCullMode(VkCullModeFlagBits cullMode, VkFrontFace frontFace) {
    m_rasterizerState.cullMode = cullMode;
    m_rasterizerState.frontFace = frontFace;
}

void VulkanPipeline::SetDepthBias(bool enable, float constantFactor, float clamp, float slopeFactor) {
    m_rasterizerState.depthBiasEnable = enable;
    m_rasterizerState.depthBiasConstantFactor = constantFactor;
    m_rasterizerState.depthBiasClamp = clamp;
    m_rasterizerState.depthBiasSlopeFactor = slopeFactor;

}

void VulkanPipeline::SetLineWidth(float lineWidth) {
    m_rasterizerState.lineWidth = lineWidth;
}

void VulkanPipeline::SetRenderPass(VulkanRenderPass *renderPass, uint32_t subpassIndex) {
    m_renderPass = renderPass->GetVkRenderPass();
    m_subpassIndex = subpassIndex;
}

void VulkanPipeline::SetDepthTest(bool testEnable, bool writeEnable, VkCompareOp compareOp) {
    m_depthStencilState.depthTestEnable = testEnable;
    m_depthStencilState.depthWriteEnable = writeEnable;
    m_depthStencilState.depthCompareOp = compareOp;
}

void VulkanPipeline::SetDepthBoundsTest(bool boundsTestEnable, float minBounds, float maxBounds) {
    m_depthStencilState.depthBoundsTestEnable = boundsTestEnable;
    m_depthStencilState.minDepthBounds = minBounds;
    m_depthStencilState.maxDepthBounds = maxBounds;
}

void VulkanPipeline::SetStencilTest(bool testEnable, const VkStencilOpState &front, const VkStencilOpState &back) {
    m_depthStencilState.stencilTestEnable = testEnable;
    m_depthStencilState.front = front;
    m_depthStencilState.back = back;
}

void VulkanPipeline::SetColorBlendAttachment(uint32_t attachmentIndex, VkPipelineColorBlendAttachmentState *blendState) {
    if (m_colorBlendAttachments.size() < (attachmentIndex + 1)) {
        m_colorBlendAttachments.resize(attachmentIndex + 1);
    }
    m_colorBlendAttachments[attachmentIndex] = *blendState;
    m_colorBlendState.attachmentCount = m_colorBlendAttachments.size();
    m_colorBlendState.pAttachments = m_colorBlendAttachments.data();
}

void VulkanPipeline::SetColorBlendLogicOp(bool logicOpEnable, VkLogicOp logicOp) {
    m_colorBlendState.logicOpEnable = logicOpEnable;
    m_colorBlendState.logicOp = logicOp;
}

void VulkanPipeline::SetColorBlendConstants(float r, float g, float b, float a) {
    m_colorBlendState.blendConstants[0] = r;
    m_colorBlendState.blendConstants[1] = g;
    m_colorBlendState.blendConstants[2] = b;
    m_colorBlendState.blendConstants[3] = a;
}

Graphics::GraphicsError VulkanPipeline::CreatePipeline(VkPipeline *out) {
    //TODO: set entry func names

    return Graphics::GraphicsError::OK;
}

void VulkanPipeline::ClearResources() {

}

} // namespace Vulkan
