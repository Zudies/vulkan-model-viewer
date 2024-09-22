#include "pch.h"
#include "VulkanRendererSceneImpl_Basic.h"
#include "VulkanRendererImpl.h"
#include "VulkanDescriptorSetInstance.h"

#include "VulkanPipeline.h"
#include "glm/gtc/matrix_transform.hpp"

namespace Vulkan {

VkVertexInputBindingDescription RendererSceneImpl_Basic::Vertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};

    bindingDescription.binding = 0;
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    bindingDescription.stride = sizeof(RendererSceneImpl_Basic::Vertex);

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> RendererSceneImpl_Basic::Vertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
    return attributeDescriptions;
}

RendererSceneImpl_Basic::RendererSceneImpl_Basic(RendererImpl *parentRenderer)
  : m_testRenderObject(parentRenderer),
    m_testTexture(parentRenderer),
    m_testSampler(parentRenderer),
    m_testObjectDescriptorLayout(parentRenderer),
    m_testObjectDescriptorSet{},
    m_renderer(parentRenderer),
    m_vertexShader(parentRenderer),
    m_fragmentShader(parentRenderer),
    m_renderPass(parentRenderer),
    m_pipeline(parentRenderer),
    m_depthBuffer(parentRenderer),
    m_perFrameDescriptorSetLayout(parentRenderer),
    m_perFrameUbo(parentRenderer),
    m_perFrameDescriptorSet{},
    m_persistentDescriptorPool(parentRenderer),
    m_perFrameDescriptorPool{},
    m_curFrameIndex(0),
    m_curSwapChainImageIndex(0),
    m_commandBuffers{},
    m_accumulatedTime(0.0) {
    ASSERT(parentRenderer);
}

RendererSceneImpl_Basic::~RendererSceneImpl_Basic() {
}

Graphics::GraphicsError RendererSceneImpl_Basic::Initialize() {
    //TODO: Read overrides from UI

    // Assume swapchain 0
    if (m_renderer->m_swapchains.size() <= 0) {
        LOG_ERROR(L"  No valid swapchain found in index 0\n");
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }
    auto &swapChain = m_renderer->m_swapchains[0];

#pragma region Scene object creation
    //TODO: Move object initialization elsewhere
    m_camera.SetPosition(0.0f, 2.0f, -2.0f);
    m_camera.LookAt(0.0f, 0.0f, 0.0f);
    m_camera.SetVerticalFOVDeg(90.0f);
    m_camera.SetAspectRatio(static_cast<f32>(swapChain.GetExtents().width), static_cast<f32>(swapChain.GetExtents().height));
    m_camera.SetNearFarPlanes(0.1f, 10.0f);

    m_testTexture.LoadImageFromFile("resources/texture.jpg");
    m_testTexture.FlushTextureToDevice();
    m_testTexture.ClearHostResources();
    m_testSampler.Initialize();

    m_perFrameUbo.Initialize(sizeof(UBO), FRAMES_IN_FLIGHT);

    m_testRenderObject.SetVertexCount(8);
    Vertex *vertexData = reinterpret_cast<Vertex *>(m_testRenderObject.GetVertexData());
    m_testRenderObject.SetIndexCount(12);
    uint16_t *indexData = reinterpret_cast<uint16_t *>(m_testRenderObject.GetIndexData());

    vertexData[0] = { {-0.5f, 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} };
    vertexData[1] = { {0.5f, 0.0f, -0.5f}, { 0.0f, 1.0f, 0.0f }, {1.0f, 1.0f} };
    vertexData[2] = { {0.5f, 0.0f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} };
    vertexData[3] = { {-0.5f, 0.0f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f} };

    vertexData[4] = { {-0.5f, -0.5f, -0.5f}, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } };
    vertexData[5] = { {0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} };
    vertexData[6] = { {0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} };
    vertexData[7] = { {-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f} };

    indexData[0] = 0; indexData[1] = 1; indexData[2] = 2;
    indexData[3] = 2; indexData[4] = 3; indexData[5] = 0;

    indexData[6] = 4; indexData[7] = 5; indexData[8] = 6;
    indexData[9] = 6; indexData[10] = 7; indexData[11] = 4;

    m_testRenderObject.FlushVertexToDevice();
    m_testRenderObject.FlushIndexToDevice();
#pragma endregion

#pragma region Shader modules
    LOG_INFO(L"Loading shaders\n");
    m_vertexShader.SetShaderStage(VK_SHADER_STAGE_VERTEX_BIT);
    m_vertexShader.CreateFromSpirv("resources/basic-vert.spv");
    if (!m_vertexShader.GetLastError().empty()) {
        LOG_ERROR(L"  Vertex shader creation error: %hs\n", m_vertexShader.GetLastError().c_str());
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }

    m_fragmentShader.SetShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT);
    m_fragmentShader.CreateFromSpirv("resources/basic-frag.spv");
    if (!m_fragmentShader.GetLastError().empty()) {
        LOG_ERROR(L"  Fragment shader creation error: %hs\n", m_fragmentShader.GetLastError().c_str());
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }
    LOG_INFO("Shaders loaded successfully\n");
#pragma endregion

#pragma region Descriptor sets
    LOG_INFO(L"Creating descriptor sets\n");
    m_perFrameDescriptorSetLayout.AddUniformBuffer(0, 1, VK_SHADER_STAGE_VERTEX_BIT);
    if (m_perFrameDescriptorSetLayout.Initialize() != Graphics::GraphicsError::OK) {
        LOG_ERROR(L"  Failed to create per frame descriptor set layout\n");
        return Graphics::GraphicsError::DESCRIPTOR_SET_CREATE_ERROR;
    }

    m_testObjectDescriptorLayout.AddCombinedImageSampler(0, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    if (m_testObjectDescriptorLayout.Initialize() != Graphics::GraphicsError::OK) {
        LOG_ERROR(L"  Failed to create test object descriptor set layout\n");
        return Graphics::GraphicsError::DESCRIPTOR_SET_CREATE_ERROR;
    }

    m_persistentDescriptorPool.Initialize();

    // Create descriptor pools and descriptor sets for each frame in flight
    for (size_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        m_perFrameDescriptorSet[i] = new VulkanDescriptorSetInstance(m_renderer);
        m_perFrameDescriptorSet[i]->SetDescriptorSetLayout(&m_perFrameDescriptorSetLayout);

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_perFrameUbo.GetDeviceBuffer(i);
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;
        m_perFrameDescriptorSet[i]->UpdateDescriptorWrite(0, &bufferInfo);

        m_testObjectDescriptorSet[i] = new VulkanDescriptorSetInstance(m_renderer);
        m_testObjectDescriptorSet[i]->SetDescriptorSetLayout(&m_testObjectDescriptorLayout);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_testTexture.GetDeviceImageView();
        imageInfo.sampler = m_testSampler.GetVkSampler();
        m_testObjectDescriptorSet[i]->UpdateDescriptorWrite(0, &imageInfo);

        m_perFrameDescriptorPool[i] = new VulkanDescriptorSetAllocator(m_renderer);
        m_perFrameDescriptorPool[i]->AddDescriptorLayout(&m_perFrameDescriptorSetLayout, 1);
        m_perFrameDescriptorPool[i]->AddDescriptorLayout(&m_testObjectDescriptorLayout, 1);
        m_perFrameDescriptorPool[i]->Initialize();
    }

    LOG_INFO(L"Descriptor sets created successfully\n");
#pragma endregion

#pragma region Depth stencil buffer
    LOG_INFO(L"Creating depth buffers\n");

    if (m_depthBuffer.Initialize(swapChain.GetExtents().width, swapChain.GetExtents().height, VK_FORMAT_D32_SFLOAT) != Graphics::GraphicsError::OK) {
        LOG_ERROR(L"  Failed to initialize depth stencil buffer\n");
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }

    LOG_INFO(L"Depth buffers created successfully\n");
#pragma endregion

#pragma region Render pass
    LOG_INFO(L"Creating render passes\n");

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChain.GetFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    m_renderPass.AddAttachment(&colorAttachment);

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = m_depthBuffer.GetFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    m_renderPass.AddAttachment(&depthAttachment);

    m_renderPass.SetSubpassCount(1);

    m_renderPass.SetSubpassPipelineBindpoint(0, VK_PIPELINE_BIND_POINT_GRAPHICS);
    m_renderPass.AddSubpassColorAttachment(0, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    m_renderPass.AddSubpassDepthStencilAttachment(0, 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    m_renderPass.AddSubpassDependency(&dependency);

    if (m_renderPass.Initialize() != Graphics::GraphicsError::OK) {
        LOG_ERROR(L"  Failed to create render pass\n");
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }

    LOG_INFO(L"Render passes created successfully\n");
#pragma endregion

#pragma region Pipeline settings
    LOG_INFO(L"Creating 'Basic' pipeline\n");

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    m_pipeline.SetDynamicStates(countof(dynamicStates), dynamicStates);

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    m_pipeline.SetVertexInput(1, &bindingDescription,
        static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());

    m_pipeline.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    m_pipeline.SetInputPrimitiveRestart(false);

    m_pipeline.SetShaderStage(&m_vertexShader, "main");
    m_pipeline.SetShaderStage(&m_fragmentShader, "main");

    m_pipeline.SetDescriptorSet(0, &m_perFrameDescriptorSetLayout);
    m_pipeline.SetDescriptorSet(1, &m_testObjectDescriptorLayout);
    m_pipeline.AddPushConstantRange(0, sizeof(glm::mat4x4), VK_SHADER_STAGE_VERTEX_BIT);

    m_pipeline.SetDepthClampEnable(false);
    m_pipeline.SetRasterizerDiscardEnable(false);
    m_pipeline.SetPolygonMode(VK_POLYGON_MODE_FILL);
    m_pipeline.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    m_pipeline.SetDepthBias(false, 0.0f, 0.0f, 0.0f);
    m_pipeline.SetLineWidth(1.0f);

    m_pipeline.SetRenderPass(&m_renderPass, 0);

    //TODO: multisampling
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

    m_pipeline.SetDepthTest(true, true, VK_COMPARE_OP_LESS);
    m_pipeline.SetDepthBoundsTest(false, 0.0f, 1.0f);
    m_pipeline.SetStencilTest(false, VkStencilOpState{}, VkStencilOpState{});

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    m_pipeline.SetColorBlendAttachment(0, &colorBlendAttachment);
    m_pipeline.SetColorBlendLogicOp(false, VK_LOGIC_OP_COPY);
    m_pipeline.SetColorBlendConstants(0.0f, 0.0f, 0.0f, 0.0f);

    if (m_pipeline.CreatePipeline(nullptr) != Graphics::GraphicsError::OK) {
        LOG_ERROR(L"  Failed to create graphics pipeline\n");
        return Graphics::GraphicsError::INITIALIZATION_FAILED;
    }

    LOG_INFO(L"Pipeline created successfully\n");
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

    LOG_INFO(L"Allocating main command buffers\n");
#pragma region Command buffers
    for (size_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        m_commandBuffers[i] = new VulkanCommandBuffer(m_renderer);
        m_commandBuffers[i]->SetSingleUse(false);
        m_commandBuffers[i]->SetLevel(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        m_commandBuffers[i]->SetWaitFence(true, VK_NULL_HANDLE, VK_FENCE_CREATE_SIGNALED_BIT);
        if (m_commandBuffers[i]->Initialize(RendererImpl::QUEUE_GRAPHICS, VK_NULL_HANDLE)  != Graphics::GraphicsError::OK) {
            LOG_ERROR(L"  Failed to allocate command buffers\n");
            return Graphics::GraphicsError::INITIALIZATION_FAILED;
        }
    }
#pragma endregion
    LOG_INFO(L"Command buffers successfully allocated\n");

    LOG_INFO(L"Creating sync objects\n");
#pragma region Sync objects
    m_swapChainSemaphores.resize(FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        if (vkCreateSemaphore(m_renderer->GetDevice(), &semaphoreInfo, VK_NULL_HANDLE, &m_swapChainSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_renderer->GetDevice(), &semaphoreInfo, VK_NULL_HANDLE, &m_renderFinishedSemaphores[i]) != VK_SUCCESS) {
            LOG_ERROR(L"  Failed to create semaphores\n");
            return Graphics::GraphicsError::INITIALIZATION_FAILED;
        }
    }
#pragma endregion
    LOG_INFO(L"Sync objects successfully created\n");

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererSceneImpl_Basic::Finalize() {
    vkDeviceWaitIdle(m_renderer->GetDevice());

    for (size_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        delete m_commandBuffers[i];
    }
    for (auto &semaphore : m_swapChainSemaphores) {
        vkDestroySemaphore(m_renderer->GetDevice(), semaphore, VK_NULL_HANDLE);
    }
    for (auto &semaphore : m_renderFinishedSemaphores) {
        vkDestroySemaphore(m_renderer->GetDevice(), semaphore, VK_NULL_HANDLE);
    }

    m_pipeline.ClearResources();
    m_renderPass.ResetResources();

    for (size_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        delete m_perFrameDescriptorSet[i];
        m_perFrameDescriptorSet[i] = nullptr;
        delete m_testObjectDescriptorSet[i];
        m_testObjectDescriptorSet[i] = nullptr;
        delete m_perFrameDescriptorPool[i];
        m_perFrameDescriptorPool[i] = nullptr;
    }

    for (auto framebuffer : m_swapChainFramebuffers) {
        vkDestroyFramebuffer(m_renderer->GetDevice(), framebuffer, VK_NULL_HANDLE);
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererSceneImpl_Basic::EarlyUpdate(f64 deltaTime) {
    auto &swapChain = m_renderer->m_swapchains[0];
    if (!swapChain.IsValid()) {
        return Graphics::GraphicsError::SWAPCHAIN_OUT_OF_DATE;
    }

    m_curFrameIndex = (m_curFrameIndex + 1) % FRAMES_IN_FLIGHT;
    m_accumulatedTime += deltaTime;

    // If pipeline is dirty, need to wait for all frames to finish so that it can be recreated
    //TODO: Not ideal to do this wait instead of just having the VkPipeline be cached until all frames are done
    if (m_pipeline.IsDirty()) {
        for (size_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
            vkWaitForFences(m_renderer->GetDevice(), 1, &m_commandBuffers[i]->GetWaitFence(), VK_TRUE, std::numeric_limits<uint64_t>::max());
        }
        m_pipeline.CreatePipeline(nullptr);
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererSceneImpl_Basic::Update(f64 deltaTime) {
    // Acquire swap chain image
    // Make sure that the frame we're about to use is not still busy
    vkWaitForFences(m_renderer->GetDevice(), 1, &m_commandBuffers[m_curFrameIndex]->GetWaitFence(), VK_TRUE, std::numeric_limits<uint64_t>::max());

    // Update per frame UBO
    UBO ubo;
    ubo.viewProj = m_camera.ProjectionMatrix() * m_camera.ViewMatrix();
    uint8_t *data = reinterpret_cast<uint8_t *>(m_perFrameUbo.GetMappedMemory(m_curFrameIndex));
    memcpy(data, &ubo, sizeof(UBO));

    // Reset and allocate descriptor sets
    m_perFrameDescriptorPool[m_curFrameIndex]->Reset();
    VulkanDescriptorSetInstance *perFrameDescriptorSets[] = { m_perFrameDescriptorSet[m_curFrameIndex], m_testObjectDescriptorSet[m_curFrameIndex] };
    m_perFrameDescriptorPool[m_curFrameIndex]->AllocateDescriptorSet(countof(perFrameDescriptorSets), perFrameDescriptorSets);

    auto &swapChain = m_renderer->m_swapchains[0];
    auto err = m_renderer->AcquireNextSwapChainImage(0, std::numeric_limits<uint64_t>::max(), m_swapChainSemaphores[m_curFrameIndex], VK_NULL_HANDLE, &m_curSwapChainImageIndex);
    if (err == Graphics::GraphicsError::SWAPCHAIN_OUT_OF_DATE) {
        return err;
    }
    else if (err != Graphics::GraphicsError::OK) {
        return Graphics::GraphicsError::SWAPCHAIN_INVALID;
    }

    // Reset command buffer and prepare it for rendering
    m_commandBuffers[m_curFrameIndex]->ResetCommandBuffer(0);

    // Draw object
    if (m_commandBuffers[m_curFrameIndex]->BeginCommandBuffer() != Graphics::GraphicsError::OK) {
        return Graphics::GraphicsError::QUEUE_ERROR;
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass.GetVkRenderPass();
    renderPassInfo.framebuffer = m_swapChainFramebuffers[m_curSwapChainImageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChain.GetExtents();

    VkClearValue clearColors[2] = {};
    clearColors[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    clearColors[1].depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = countof(clearColors);
    renderPassInfo.pClearValues = clearColors;

    vkCmdBeginRenderPass(m_commandBuffers[m_curFrameIndex]->GetVkCommandBuffer(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(m_commandBuffers[m_curFrameIndex]->GetVkCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.GetVkPipeline());
    vkCmdBindDescriptorSets(m_commandBuffers[m_curFrameIndex]->GetVkCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.GetVkPipelineLayout(), 0, 1, &m_perFrameDescriptorSet[m_curFrameIndex]->GetVkDescriptorSet(), 0, nullptr);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = static_cast<float>(swapChain.GetExtents().height);
    viewport.width = static_cast<float>(swapChain.GetExtents().width);
    viewport.height = -static_cast<float>(swapChain.GetExtents().height); // Negative height to flip y-axis
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_commandBuffers[m_curFrameIndex]->GetVkCommandBuffer(), 0, 1, &viewport);

    VkRect2D scissors{};
    scissors.extent.width = swapChain.GetExtents().width;
    scissors.extent.height = swapChain.GetExtents().height;
    vkCmdSetScissor(m_commandBuffers[m_curFrameIndex]->GetVkCommandBuffer(), 0, 1, &scissors);

    VkBuffer vertexBuffers[] = { m_testRenderObject.GetVertexDeviceBuffer()};
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(m_commandBuffers[m_curFrameIndex]->GetVkCommandBuffer(), 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(m_commandBuffers[m_curFrameIndex]->GetVkCommandBuffer(), m_testRenderObject.GetIndexDeviceBuffer(), 0, VK_INDEX_TYPE_UINT16);

    //TODO: This comes from the object
    glm::mat4x4 modelMatrix =
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f))
        * glm::rotate(glm::mat4(1.0f), static_cast<float>(m_accumulatedTime) * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f))
        * glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f));

    VkDescriptorSet bindDescriptorSets[] = { m_testObjectDescriptorSet[m_curFrameIndex]->GetVkDescriptorSet() };
    vkCmdBindDescriptorSets(m_commandBuffers[m_curFrameIndex]->GetVkCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.GetVkPipelineLayout(), 1, countof(bindDescriptorSets), bindDescriptorSets, 0, nullptr);
    vkCmdPushConstants(m_commandBuffers[m_curFrameIndex]->GetVkCommandBuffer(), m_pipeline.GetVkPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4x4), &modelMatrix);

    vkCmdDrawIndexed(m_commandBuffers[m_curFrameIndex]->GetVkCommandBuffer(), static_cast<uint32_t>(m_testRenderObject.GetIndexCount()), 1, 0, 0, 0);

    vkCmdEndRenderPass(m_commandBuffers[m_curFrameIndex]->GetVkCommandBuffer());

    if (m_commandBuffers[m_curFrameIndex]->EndCommandBuffer() != Graphics::GraphicsError::OK) {
        return Graphics::GraphicsError::QUEUE_ERROR;
    }

    // Submit the command buffer
    m_commandBuffers[m_curFrameIndex]->AddWaitSemaphore(m_swapChainSemaphores[m_curFrameIndex], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    m_commandBuffers[m_curFrameIndex]->AddSignalSemaphore(m_renderFinishedSemaphores[m_curFrameIndex]);
    if (m_commandBuffers[m_curFrameIndex]->Submit() != Graphics::GraphicsError::OK) {
        return Graphics::GraphicsError::QUEUE_ERROR;
    }

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererSceneImpl_Basic::LateUpdate(f64 deltaTime) {
    auto &swapChain = m_renderer->m_swapchains[0];
    VkSwapchainKHR vkSwapChain = swapChain.GetSwapchain();

    // Present the frame
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_renderFinishedSemaphores[m_curFrameIndex];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vkSwapChain;
    presentInfo.pImageIndices = &m_curSwapChainImageIndex;
    presentInfo.pResults = nullptr;
    auto err = vkQueuePresentKHR(m_renderer->m_queues[RendererImpl::QUEUE_PRESENT], &presentInfo);
    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        return Graphics::GraphicsError::SWAPCHAIN_OUT_OF_DATE;
    }
    else if (err != VK_SUCCESS) {
        return Graphics::GraphicsError::SWAPCHAIN_INVALID;
    }

    return Graphics::GraphicsError::OK;
}

std::string RendererSceneImpl_Basic::GetPipelineStateValue(const std::string &pipelineState) {
    if (pipelineState.empty()) {
        return "";
    }

    if (pipelineState == "rasterizer.polygonMode") {
        switch (m_pipeline.GetPolygonMode()) {
        case VK_POLYGON_MODE_FILL:
            return "VK_POLYGON_MODE_FILL";
        case VK_POLYGON_MODE_LINE:
            return "VK_POLYGON_MODE_LINE";
        case VK_POLYGON_MODE_POINT:
            return "VK_POLYGON_MODE_POINT";
        }
    }
    else if (pipelineState == "rasterizer.cullMode") {
        switch (m_pipeline.GetCullMode()) {
        case VK_CULL_MODE_NONE:
            return "VK_CULL_MODE_NONE";
        case VK_CULL_MODE_FRONT_BIT:
            return "VK_CULL_MODE_FRONT_BIT";
        case VK_CULL_MODE_BACK_BIT:
            return "VK_CULL_MODE_BACK_BIT";
        case VK_CULL_MODE_FRONT_AND_BACK:
            return "VK_CULL_MODE_FRONT_AND_BACK";
        }
    }

    return "";
}

void RendererSceneImpl_Basic::SetPipelineStateValue(const std::string &pipelineState, const std::string &pipelineStateValue) {
    if (pipelineState.empty()) {
        return;
    }

    if (pipelineState == "rasterizer.polygonMode") {
        if (pipelineStateValue == "VK_POLYGON_MODE_FILL") {
            m_pipeline.SetPolygonMode(VK_POLYGON_MODE_FILL);
        }
        else if (pipelineStateValue == "VK_POLYGON_MODE_LINE") {
            m_pipeline.SetPolygonMode(VK_POLYGON_MODE_LINE);
        }
        else if (pipelineStateValue == "VK_POLYGON_MODE_POINT") {
            m_pipeline.SetPolygonMode(VK_POLYGON_MODE_POINT);
        }
    }
    else if (pipelineState == "rasterizer.cullMode") {
        if (pipelineStateValue == "VK_CULL_MODE_NONE") {
            m_pipeline.SetCullMode(VK_CULL_MODE_NONE, m_pipeline.GetFrontFace());
        }
        else if (pipelineStateValue == "VK_CULL_MODE_FRONT_BIT") {
            m_pipeline.SetCullMode(VK_CULL_MODE_FRONT_BIT, m_pipeline.GetFrontFace());
        }
        else if (pipelineStateValue == "VK_CULL_MODE_BACK_BIT") {
            m_pipeline.SetCullMode(VK_CULL_MODE_BACK_BIT, m_pipeline.GetFrontFace());
        }
        else if (pipelineStateValue == "VK_CULL_MODE_FRONT_AND_BACK") {
            m_pipeline.SetCullMode(VK_CULL_MODE_FRONT_AND_BACK, m_pipeline.GetFrontFace());
        }
    }
}

Graphics::GraphicsError RendererSceneImpl_Basic::_onDestroySwapChain(int idx) {
    // Only using index 0
    if (idx == 0) {
        // Destroy all framebuffers
        for (auto framebuffer : m_swapChainFramebuffers) {
            vkDestroyFramebuffer(m_renderer->GetDevice(), framebuffer, VK_NULL_HANDLE);
        }
        m_swapChainFramebuffers.clear();

        // Destroy depth buffer
        m_depthBuffer.Clear();
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

        // Recreate depth buffer
        auto err = m_depthBuffer.Initialize(swapChain.GetExtents().width, swapChain.GetExtents().height, VK_FORMAT_D32_SFLOAT);
        if (err != Graphics::GraphicsError::OK) {
            return err;
        }

        // Recreate render pass
        err = _createRenderPass(swapChain);
        if (err != Graphics::GraphicsError::OK) {
            return err;
        }

        // Recreate framebuffers
        return _createSwapChainFrameBuffers(swapChain);
    }
    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererSceneImpl_Basic::_createRenderPass(VulkanSwapChain &swapChain) {
    // May need to delete previous render pass first
    //if (m_renderPass) {
    //    vkDestroyRenderPass(m_renderer->GetDevice(), m_renderPass, VK_NULL_HANDLE);
    //}

    //TODO: Rebuilding render pass means rebuilding the entire pipeline

    return Graphics::GraphicsError::OK;
}

Graphics::GraphicsError RendererSceneImpl_Basic::_createSwapChainFrameBuffers(VulkanSwapChain &swapChain) {
    auto &swapChainImageViews = swapChain.GetImageViews();
    m_swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = { swapChainImageViews[i], m_depthBuffer.GetDeviceImageView() };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass.GetVkRenderPass();
        framebufferInfo.attachmentCount = countof(attachments);
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChain.GetExtents().width;
        framebufferInfo.height = swapChain.GetExtents().height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_renderer->GetDevice(), &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS) {
            return Graphics::GraphicsError::SWAPCHAIN_CREATE_ERROR;
        }
    }
    return Graphics::GraphicsError::OK;
}

} // namespace Vulkan
