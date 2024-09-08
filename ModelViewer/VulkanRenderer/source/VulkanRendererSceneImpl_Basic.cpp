#include "pch.h"
#include "VulkanRendererSceneImpl_Basic.h"
#include "VulkanRendererImpl.h"

namespace Vulkan {

VkVertexInputBindingDescription RendererSceneImpl_Basic::Vertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};

    bindingDescription.binding = 0;
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    bindingDescription.stride = sizeof(RendererSceneImpl_Basic::Vertex);

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 2> RendererSceneImpl_Basic::Vertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);
    return attributeDescriptions;
}

RendererSceneImpl_Basic::RendererSceneImpl_Basic()
  : m_renderer(VK_NULL_HANDLE),
    m_renderPass(VK_NULL_HANDLE),
    m_vertDescriptorSetLayout(VK_NULL_HANDLE),
    m_pipelineLayout(VK_NULL_HANDLE),
    m_pipeline(VK_NULL_HANDLE) {
}

RendererSceneImpl_Basic::~RendererSceneImpl_Basic() {
}

Graphics::GraphicsError RendererSceneImpl_Basic::Initialize(RendererImpl *parentRenderer) {
    m_renderer = parentRenderer;

    LOG_INFO(L"Creating pipeline for 'Basic'\n");
    //TODO: Read overrides from UI

#pragma region Shader modules
    VulkanShaderModule vertShader(parentRenderer);
    vertShader.CreateFromSpirv("basic-vert.spv");
    if (!vertShader.GetLastError().empty()) {
        LOG_ERROR(L"  Vertex shader creation error: %hs\n", vertShader.GetLastError().c_str());
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }

    VulkanShaderModule fragShader(parentRenderer);
    fragShader.CreateFromSpirv("basic-frag.spv");
    if (!fragShader.GetLastError().empty()) {
        LOG_ERROR(L"  Fragment shader creation error: %hs\n", fragShader.GetLastError().c_str());
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }
#pragma endregion

#pragma region Pipeline shader stage
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShader.GetShaderModule();
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShader.GetShaderModule();
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
#pragma endregion

    // Assume swapchain 0
    if (m_renderer->m_swapchains.size() <= 0) {
        LOG_ERROR(L"  No valid swapchain found in index 0\n");
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }
    auto &swapChain = m_renderer->m_swapchains[0];

#pragma region Render pass
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChain.GetFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_renderer->m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        LOG_ERROR(L"  Failed to create render pass\n");
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }
#pragma endregion

#pragma region Frame buffers (swap chain)
    auto err = _createSwapChainFrameBuffers(swapChain);
    if (err != Graphics::GraphicsError::OK) {
        LOG_ERROR(L"  Failed to create frame buffer for swap chain\n");
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }

    // Register recreate swap chain func
    m_renderer->RegisterOnRecreateSwapChainFunc(
        std::bind(&RendererSceneImpl_Basic::_onDestroySwapChain, this, std::placeholders::_1),
        std::bind(&RendererSceneImpl_Basic::_onCreateSwapChain, this, std::placeholders::_1)
    );
#pragma endregion

#pragma region Dynamic states
    //TODO: Not using dynamic states for now
    std::vector<VkDynamicState> dynamicStates = {
        //VK_DYNAMIC_STATE_VIEWPORT,
        //VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();
#pragma endregion

#pragma region Viewports/Scissors
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChain.GetExtents().width);
    viewport.height = static_cast<float>(swapChain.GetExtents().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissors{};
    scissors.extent.width = swapChain.GetExtents().width;
    scissors.extent.height = swapChain.GetExtents().height;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissors;
#pragma endregion

#pragma region Vertex input
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
#pragma endregion

#pragma region Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
#pragma endregion

#pragma region Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;
#pragma endregion

#pragma region Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;
#pragma endregion

#pragma region Depth/stencil

#pragma endregion

#pragma region Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;
#pragma endregion

#pragma region Pipeline layout
    //TODO: Move this into shader modules
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(m_renderer->m_device, &layoutInfo, nullptr, &m_vertDescriptorSetLayout) != VK_SUCCESS) {
        LOG_ERROR(L"  Failed to create descriptor set layout\n");
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_vertDescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(m_renderer->m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        LOG_ERROR(L"  Failed to create pipeline layout\n");
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }
#pragma endregion

#pragma region Pipeline creation
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = VK_NULL_HANDLE;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = VK_NULL_HANDLE;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(m_renderer->m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS) {
        LOG_ERROR(L"  Failed to create graphics pipeline\n");
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }
#pragma endregion

    LOG_INFO(L"Graphics pipeline successfully created\n");
    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererSceneImpl_Basic::Finalize() {
    vkDestroyPipeline(m_renderer->m_device, m_pipeline, VK_NULL_HANDLE);
    vkDestroyPipelineLayout(m_renderer->m_device, m_pipelineLayout, VK_NULL_HANDLE);
    vkDestroyRenderPass(m_renderer->m_device, m_renderPass, VK_NULL_HANDLE);

    //TODO: Move to shader module
    vkDestroyDescriptorSetLayout(m_renderer->m_device, m_vertDescriptorSetLayout, VK_NULL_HANDLE);

    for (auto framebuffer : m_swapChainFramebuffers) {
        vkDestroyFramebuffer(m_renderer->m_device, framebuffer, VK_NULL_HANDLE);
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererSceneImpl_Basic::Update(f64 deltaTime) {
    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererSceneImpl_Basic::_onDestroySwapChain(int idx) {
    // Only using index 0
    if (idx == 0) {
        // Destroy all framebuffers
        for (auto framebuffer : m_swapChainFramebuffers) {
            vkDestroyFramebuffer(m_renderer->m_device, framebuffer, VK_NULL_HANDLE);
        }
        m_swapChainFramebuffers.clear();
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererSceneImpl_Basic::_onCreateSwapChain(int idx) {
    // Only using index 0
    if (idx == 0) {
        if (m_renderer->m_swapchains.size() <= 0) {
            return Graphics::GraphicsError::SWAPCHAIN_CREATE_ERROR;
        }
        auto &swapChain = m_renderer->m_swapchains[0];
        if (!swapChain.IsValid()) {
            return Graphics::GraphicsError::SWAPCHAIN_INVALID;
        }

        auto err = _createRenderPass(swapChain);
        if (err != Graphics::GraphicsError::OK) {
            return err;
        }
        return _createSwapChainFrameBuffers(swapChain);
    }
    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererSceneImpl_Basic::_createRenderPass(VulkanSwapChain &swapChain) {
    // May need to delete previous render pass first
    //if (m_renderPass) {
    //    vkDestroyRenderPass(m_renderer->m_device, m_renderPass, VK_NULL_HANDLE);
    //}

    //TODO: Rebuilding render pass means rebuilding the entire pipeline

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererSceneImpl_Basic::_createSwapChainFrameBuffers(VulkanSwapChain &swapChain) {
    auto &swapChainImageViews = swapChain.GetImageViews();
    m_swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &swapChainImageViews[i];
        framebufferInfo.width = swapChain.GetExtents().width;
        framebufferInfo.height = swapChain.GetExtents().height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_renderer->m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS) {
            return Graphics::GraphicsError::SWAPCHAIN_CREATE_ERROR;
        }
    }
    return Graphics::GraphicsError::OK;
}

} // namespace Vulkan
