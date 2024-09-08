#pragma once

#include "VulkanShaderModule.h"

namespace Vulkan {

class RendererImpl;

class RendererSceneImpl_Basic {
public:
    RendererSceneImpl_Basic();
    ~RendererSceneImpl_Basic();

    Graphics::GraphicsError Initialize(RendererImpl *parentRenderer);
    Graphics::GraphicsError Finalize();
    Graphics::GraphicsError Update(f64 deltaTime);

private:
    struct Vertex {
        //TODO: Replace with math library
        f32 position[2];
        f32 color[3];

        static VkVertexInputBindingDescription getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
    };

private:
    RendererImpl *m_renderer;

    typedef std::vector<VkFramebuffer> FrameBufferArray;
    VkRenderPass m_renderPass;
    FrameBufferArray m_swapChainFramebuffers;

    //TODO: Move into shader modules
    VkDescriptorSetLayout m_vertDescriptorSetLayout;

    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_pipeline;
};

} // namespace Vulkan
