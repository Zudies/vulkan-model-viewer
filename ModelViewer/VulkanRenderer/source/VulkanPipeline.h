#pragma once

#include "BitFlag.h"

namespace Vulkan {

class RendererImpl;
class VulkanShaderModule;
class VulkanRenderPass;
class VulkanDescriptorSetLayout;

class VulkanPipeline {
public:
    // Note: When copying a pipeline, only the state that would not be reset by ClearResources will be copied over
    //       Any resources such as VkPipeline will NOT be copied
    VulkanPipeline(RendererImpl *renderer);
    VulkanPipeline(VulkanPipeline const &);
    VulkanPipeline &operator=(VulkanPipeline const &);
    ~VulkanPipeline();

#pragma region States preserved between calls of ClearResourcces
    // Any flags set will be OR'd with the current flags
    void SetPipelineFlags(VkPipelineCreateFlags flags);

    // If a dynamic state is set, the corresponding pipeline setter is optional and its value will be ignored
    // All desired dynamic states should be set in the same call
    // Additional calls to this will overwrite all previous values
    void SetDynamicStates(uint32_t dynamicStateCount, VkDynamicState *dynamicStates);

    /* Vertex input state */
    // Only one vertex input state can be set
    // Additional sets will overwrite previous values
    void SetVertexInput(uint32_t bindingCount, VkVertexInputBindingDescription *bindings,
                        uint32_t attributeCount, VkVertexInputAttributeDescription *attributes);

    // Default: VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    void SetInputTopology(VkPrimitiveTopology topology);

    // Default: false
    void SetInputPrimitiveRestart(bool primitiveRestartEnable);

    /* Pre-rasterization state*/
    // One call to this for every shader stage that is active
    // If a shader stage is already set when this is called, the previous value is overwritten
    void SetShaderStage(VulkanShaderModule *shader, const char *entryFunc);

    // Each call will set the descriptor set at the specified index of the pipeline layout
    // Max 4 descriptor sets
    void SetDescriptorSet(uint32_t descriptorSetIndex, VulkanDescriptorSetLayout *descriptorSet);

    // Adds a push constant range to the pipeline layout
    // Additional calls will continue to add more ranges until ResetPipelineLayout is called
    void AddPushConstantRange(uint32_t offset, uint32_t size, VkShaderStageFlags shaderStages);

    // Clears push constant ranges, reseting the layout back to having no push constants
    void ResetPushConstantRange();

    // Required if not dynamic
    void SetViewport(VkViewport viewport);

    // Default: Same value as the first call to SetViewport
    void SetScissor(VkRect2D scissor);

    // Default: false
    void SetDepthClampEnable(bool depthClampEnable);

    // Default: false
    void SetRasterizerDiscardEnable(bool rasterizerDiscardEnable);

    // Default: VK_POLYGON_MODE_FILL
    void SetPolygonMode(VkPolygonMode polygonMode);

    // Default: VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE
    void SetCullMode(VkCullModeFlagBits cullMode, VkFrontFace frontFace);

    // Default: false, 0.0f, 0.0f, 0.0f
    void SetDepthBias(bool enable, float constantFactor, float clamp, float slopeFactor);

    // Default: 1.0f
    void SetLineWidth(float lineWidth);

    //TODO: Tesselation state

    // Required
    void SetRenderPass(VulkanRenderPass *renderPass, uint32_t subpassIndex);

    /* Fragment shader state */
    //TODO: Multisampling

    // Default: false, false, VK_COMPARE_OP_NEVER
    void SetDepthTest(bool testEnable, bool writeEnable, VkCompareOp compareOp);

    // Default: false, 0.0f, 1.0f
    void SetDepthBoundsTest(bool boundsTestEnable, float minBounds, float maxBounds);

    // Default: false, 0-initialized VkStencilOpStates for front and back
    void SetStencilTest(bool testEnable, const VkStencilOpState &front, const VkStencilOpState &back);

    /* Fragment output state */
    // Required
    // Must be called for each ColorAttachment within the render pass subpass
    void SetColorBlendAttachment(uint32_t attachmentIndex, VkPipelineColorBlendAttachmentState *blendState);

    // Default: false, VK_LOGIC_OP_CLEAR
    void SetColorBlendLogicOp(bool logicOpEnable, VkLogicOp logicOp);

    // Default: 0.0f, 0.0f, 0.0f, 0.0f
    void SetColorBlendConstants(float r, float g, float b, float a);
#pragma endregion

    // Creates a new pipeline if necessary
    // Returns the previously created pipeline if no state changes have been made
    Graphics::GraphicsError CreatePipeline(VkPipeline *out);

    // Note these functions bypass the dirty flag/recreate pipeline logic
    // Only use if you know the pipeline has not been modified
    VkPipeline GetVkPipeline() const;
    VkPipelineLayout GetVkPipelineLayout() const;

    // Clears device resources used by the pipeline, forcing the next call to CreatePipeline to create a new VkPipeline object
    void ClearResources();

private:
    RendererImpl *m_renderer;
    Graphics::BitFlag<uint32_t> m_flags;
    VkPipeline m_vkPipeline;

    // States preserved between ClearResources
    VkPipelineCreateFlags m_createFlags;
    std::vector<std::string> m_shaderEntryFuncNames;
    std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
    std::vector<VkVertexInputBindingDescription> m_vertexBindings;
    std::vector<VkVertexInputAttributeDescription> m_vertexAttributes;
    VkPrimitiveTopology m_inputAssemblyTopology;
    VkBool32 m_inputAssemblyPrimitiveRestart;
    VkViewport m_viewportData;
    VkRect2D m_scissorData;
    VkPipelineRasterizationStateCreateInfo m_rasterizerState;
    VkPipelineMultisampleStateCreateInfo m_multisampleState;
    VkPipelineDepthStencilStateCreateInfo m_depthStencilState;
    std::vector<VkPipelineColorBlendAttachmentState> m_colorBlendAttachments;
    VkPipelineColorBlendStateCreateInfo m_colorBlendState;
    std::vector<VkDynamicState> m_dynamicStatesBuffer;
    VkRenderPass m_renderPass;
    uint32_t m_subpassIndex;
    std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
    std::vector<VkPushConstantRange> m_pushConstantRanges;
    VkPipelineLayout m_vkPipelineLayout;

};

} // namespace Vulkan
