#pragma once

#include "VulkanRenderPass.h"
#include "VulkanObjectTypes.h"
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
class VulkanStaticModelTextured;

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

    Graphics::Camera *GetCamera();

    std::string GetPipelineStateValue(const std::string &pipelineState);
    void SetPipelineStateValue(const std::string &pipelineState, const std::string &pipelineStateValue);

public:
    RendererImpl *GetRenderer();
    VulkanPipeline *GetPipeline(RenderableObjectType type);
    VulkanDescriptorSetLayout *GetDescriptorSetLayout(RenderableObjectType type);
    VulkanDescriptorSetAllocator *GetPersistentDescriptorPool();

#pragma region Must be called during an update
    VulkanDescriptorSetAllocator *GetPerFrameDescriptorPool();
    VulkanCommandBuffer *GetMainCommandBuffer();

    // Binds the pipeline and sets common dynamic states and descriptor sets
    void CommandBindPipeline(VulkanCommandBuffer *commandBuffer, VulkanPipeline *pipeline);
#pragma endregion

private:
    Graphics::GraphicsError _onDestroySwapChain(int idx);
    Graphics::GraphicsError _onCreateSwapChain(int idx);
    Graphics::GraphicsError _createRenderPass(VulkanSwapChain &swapChain);
    Graphics::GraphicsError _createSwapChainFrameBuffers(VulkanSwapChain &swapChain);

private:
    struct UBO {
        glm::mat4 viewProj;
    };

    //TODO: Should have a base object class
    std::vector<VulkanStaticModelTextured*> m_objects;

private:

    RendererImpl *m_renderer;

    typedef std::vector<VkFramebuffer> FrameBufferArray;

    VulkanShaderModule m_vertexShader;
    VulkanShaderModule m_fragmentShader;
    VulkanRenderPass m_renderPass;
    VulkanDepthStencilBuffer m_depthBuffer;

    //TODO: Pipeline should be per-material rather than per-object type
    std::vector<VulkanDescriptorSetLayout*> m_descriptorSetLayout;
    std::vector<VulkanPipeline*> m_pipeline;

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
};

} // namespace Vulkan
