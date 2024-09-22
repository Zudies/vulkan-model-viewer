#pragma once

#include "VulkanRenderPass.h"
#include "VulkanShaderModule.h"
#include "VulkanVertexBuffer.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanDescriptorSetInstance.h"
#include "VulkanDescriptorSetAllocator.h"
#include "VulkanPipeline.h"
#include "VulkanUniformBufferObject.h"
#include "Vulkan2DTextureBuffer.h"
#include "VulkanSampler.h"
#include "VulkanDepthStencilBuffer.h"
#include "Camera.h"

namespace Graphics {
class Renderer_Base;
} // namespace Graphics

namespace Vulkan {

class RendererImpl;
class VulkanSwapChain;

class RendererSceneImpl_Basic {
    static const size_t FRAMES_IN_FLIGHT = 3;

public:
    RendererSceneImpl_Basic(RendererImpl *parentRenderer);
    ~RendererSceneImpl_Basic();

    Graphics::GraphicsError Initialize();
    Graphics::GraphicsError Finalize();
    Graphics::GraphicsError EarlyUpdate(f64 deltaTime);
    Graphics::GraphicsError Update(f64 deltaTime);
    Graphics::GraphicsError LateUpdate(f64 deltaTime);

    std::string GetPipelineStateValue(const std::string &pipelineState);
    void SetPipelineStateValue(const std::string &pipelineState, const std::string &pipelineStateValue);

private:
    Graphics::GraphicsError _onDestroySwapChain(int idx);
    Graphics::GraphicsError _onCreateSwapChain(int idx);
    Graphics::GraphicsError _createRenderPass(VulkanSwapChain &swapChain);
    Graphics::GraphicsError _createSwapChainFrameBuffers(VulkanSwapChain &swapChain);

private:
    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;
        glm::vec2 texCoord;

        static VkVertexInputBindingDescription getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
    };
    struct UBO {
        glm::mat4 viewProj;
    };
    typedef VulkanVertexBuffer<Vertex> BasicObject;
    BasicObject m_testRenderObject;
    Vulkan2DTextureBuffer m_testTexture;
    VulkanSampler m_testSampler;

    VulkanDescriptorSetLayout m_testObjectDescriptorLayout;
    VulkanDescriptorSetInstance *m_testObjectDescriptorSet[FRAMES_IN_FLIGHT];

private:

    RendererImpl *m_renderer;

    typedef std::vector<VkFramebuffer> FrameBufferArray;

    VulkanShaderModule m_vertexShader;
    VulkanShaderModule m_fragmentShader;
    VulkanRenderPass m_renderPass;
    VulkanPipeline m_pipeline;
    VulkanDepthStencilBuffer m_depthBuffer;

    Graphics::Camera m_camera;
    VulkanDescriptorSetLayout m_perFrameDescriptorSetLayout;
    VulkanUniformBufferObject m_perFrameUbo;
    VulkanDescriptorSetInstance *m_perFrameDescriptorSet[FRAMES_IN_FLIGHT];

    VulkanDescriptorSetAllocator m_persistentDescriptorPool;
    VulkanDescriptorSetAllocator *m_perFrameDescriptorPool[FRAMES_IN_FLIGHT];

    FrameBufferArray m_swapChainFramebuffers;

    size_t m_curFrameIndex;
    uint32_t m_curSwapChainImageIndex;

    typedef std::vector<VkSemaphore> SemaphoreArray;
    VulkanCommandBuffer *m_commandBuffers[FRAMES_IN_FLIGHT];
    SemaphoreArray m_swapChainSemaphores;
    SemaphoreArray m_renderFinishedSemaphores;

    f64 m_accumulatedTime;
};

} // namespace Vulkan
