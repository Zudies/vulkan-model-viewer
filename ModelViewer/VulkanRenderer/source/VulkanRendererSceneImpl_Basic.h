#pragma once

#include "VulkanShaderModule.h"
#include "VulkanVertexBuffer.h"
#include "VulkanUniformBufferObject.h"
#include "Vulkan2DTextureBuffer.h"
#include "VulkanSampler.h"

namespace Graphics {
class Renderer_Base;
} // namespace Graphics

namespace Vulkan {

class RendererImpl;
class VulkanSwapChain;

class RendererSceneImpl_Basic {
public:
    RendererSceneImpl_Basic(RendererImpl *parentRenderer);
    ~RendererSceneImpl_Basic();

    Graphics::GraphicsError Initialize();
    Graphics::GraphicsError Finalize();
    Graphics::GraphicsError EarlyUpdate(f64 deltaTime);
    Graphics::GraphicsError Update(f64 deltaTime);
    Graphics::GraphicsError LateUpdate(f64 deltaTime);

private:
    Graphics::GraphicsError _onDestroySwapChain(int idx);
    Graphics::GraphicsError _onCreateSwapChain(int idx);
    Graphics::GraphicsError _createRenderPass(VulkanSwapChain &swapChain);
    Graphics::GraphicsError _createSwapChainFrameBuffers(VulkanSwapChain &swapChain);

private:
    struct Vertex {
        glm::vec2 position;
        glm::vec3 color;
        glm::vec2 texCoord;

        static VkVertexInputBindingDescription getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
    };
    struct UBO {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };
    typedef VulkanVertexBuffer<Vertex> BasicObject;
    BasicObject m_testRenderObject;
    Vulkan2DTextureBuffer m_testTexture;
    VulkanSampler m_testSampler;

private:
    static const size_t FRAMES_IN_FLIGHT = 3;

    RendererImpl *m_renderer;

    typedef std::vector<VkFramebuffer> FrameBufferArray;
    VkRenderPass m_renderPass;
    FrameBufferArray m_swapChainFramebuffers;

    //TODO: Move into shader modules
    typedef std::vector<VkDescriptorSet> DescriptorSetArray;
    VkDescriptorSetLayout m_vertDescriptorSetLayout;
    VkDescriptorPool m_vertDescriptorPool;
    DescriptorSetArray m_vertDescriptorSets;
    VulkanUniformBufferObject m_ubo;
    size_t m_curFrameIndex;
    uint32_t m_curSwapChainImageIndex;

    typedef std::vector<VkCommandBuffer> CommandBufferArray;
    typedef std::vector<VkSemaphore> SemaphoreArray;
    typedef std::vector<VkFence> FenceArray;
    CommandBufferArray m_commandBuffers;
    SemaphoreArray m_swapChainSemaphores;
    SemaphoreArray m_renderFinishedSemaphores;
    FenceArray m_renderFinishedFences;

    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_pipeline;

    f64 m_accumulatedTime;
};

} // namespace Vulkan
