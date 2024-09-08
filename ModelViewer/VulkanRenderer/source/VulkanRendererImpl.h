#pragma once

#include "VulkanPhysicalDevice.h"
#include <vector>

namespace Graphics {
class RendererRequirements;
class RendererScene_Base;
}

namespace Vulkan {

class API;
class APIImpl;
class VulkanPhysicalDevice;
class RendererScene;

class RendererImpl {
public:
    RendererImpl();
    ~RendererImpl();

    Graphics::GraphicsError Initialize(API *api, VulkanPhysicalDevice *physicalDevice, Graphics::RendererRequirements *requirements);
    Graphics::GraphicsError Finalize();
    Graphics::GraphicsError Update(f64 deltaTime);

    void SetSceneActive(Graphics::RendererScene_Base *activeScene);
    void SetSceneInactive(Graphics::RendererScene_Base *inactiveScene);

private:
    Graphics::GraphicsError _createSwapChain(Graphics::RendererRequirements *requirements);
    void _cleanupSwapChain();

private:
    friend class VulkanShaderModule;
    friend class RendererSceneImpl_Basic;

    enum QueueType {
        QUEUE_GRAPHICS = 0,
        QUEUE_PRESENT = 1,
        QUEUE_TRANSFER = 2,

        QUEUE_COUNT
    };

    APIImpl *m_api;
    VulkanPhysicalDevice *m_physicalDevice;
    VkDevice m_device;
    uint32_t m_queueIndices[QueueType::QUEUE_COUNT];
    VkQueue m_queues[QueueType::QUEUE_COUNT];
    VkCommandPool m_commandPools[QueueType::QUEUE_COUNT];

    typedef std::vector<VulkanSwapChain> SwapChainArray;
    SwapChainArray m_swapchains;

    typedef std::unordered_set<Graphics::RendererScene_Base*> SceneSet;
    SceneSet m_activeScenes;
    SceneSet m_inactiveScenes;

    bool m_useValidation;

    typedef std::vector<char const*> StringLiteralArray;
    StringLiteralArray m_vkExtensionsList;
    StringLiteralArray m_vkLayersList;

};

} // namespace Vulkan
