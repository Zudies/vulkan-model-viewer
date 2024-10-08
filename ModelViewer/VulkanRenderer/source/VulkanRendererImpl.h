#pragma once

#include "base/Renderer_Base.h"
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
class VulkanCommandBuffer;

class RendererImpl {
public:
    RendererImpl();
    ~RendererImpl();

    Graphics::GraphicsError Initialize(API *api, VulkanPhysicalDevice *physicalDevice, Graphics::RendererRequirements *requirements);
    Graphics::GraphicsError Finalize();
    Graphics::GraphicsError Update(f64 deltaTime);

    void SetSceneActive(Graphics::RendererScene_Base *activeScene);
    void SetSceneInactive(Graphics::RendererScene_Base *inactiveScene);

    void RegisterOnRecreateSwapChainFunc(Graphics::Renderer_Base::OnDestroySwapChainFn destroyFunc, Graphics::Renderer_Base::OnCreateSwapChainFn createFunc);

public:
    enum QueueType {
        QUEUE_GRAPHICS = 0,
        QUEUE_PRESENT = 1,
        QUEUE_TRANSFER = 2,

        QUEUE_COUNT
    };

    Graphics::GraphicsError AllocateCommandBuffers(QueueType queue, VkCommandBufferLevel level, size_t count, VkCommandBuffer *cmdBufferOut);

    // This function can fail if a swap chain is invalidated
    // In this event, the scene should immediately stop rendering the current frame and OnRecreateSwapChain funcs will be called on the next frame
    Graphics::GraphicsError AcquireNextSwapChainImage(int swapChainIdx, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *imageIdxOut);

    Graphics::GraphicsError GetMemoryTypeIndex(uint32_t typeFilter, uint32_t typeFlags, uint32_t poolFlags, uint32_t *out);

    VkDevice GetDevice() const;
    VulkanPhysicalDevice *GetPhysicalDevice() const;
    Graphics::RendererRequirements *GetRequirements() const;

    uint32_t GetQueueIndex(QueueType type) const;
    VkQueue GetQueue(QueueType type) const;

    // Allows batch submitting one time queue operations before the next Update step
    // When called in the EarlyUpdate step, registered functions will execute in the same frame
    // Otherwise registered functions will execute in the next frame
    // beginFunc is called at the start to record commands
    // endFunc is called after all commands have been submitted, with the same command buffers as beginFunc
    // Note that the commands are not guaranteed to have executed yet during endFunc
    // Each command buffer will have a fence associated with it
    // In the event of an error:
    //   If the error occurred when allocating command buffers, TransferErrorFunc will be called and TransferBeginFunc/TransferEndFunc will no longer be called
    //   If the error occurred during TransferBeginFunc, neither TransferErrorFunc nor TransferEndFunc will be called
    //   If the error occurred during TransferEndFunc, TransferErrorFunc will not be called
    typedef std::function<Graphics::GraphicsError(VulkanCommandBuffer *oneTimeCommandBuffers)> TransferBeginFunc;
    typedef std::function<Graphics::GraphicsError(VulkanCommandBuffer *oneTimeCommandBuffers)> TransferEndFunc;
    typedef std::function<void()> TransferErrorFunc;
    void RegisterTransfer(
        QueueType queue,
        size_t commandBufferCount,
        TransferBeginFunc beginFunc,
        TransferEndFunc endFunc,
        TransferErrorFunc errorFunc);

private:
    typedef std::vector<VulkanCommandBuffer> CommandBufferArray;

    Graphics::GraphicsError _createSwapChain(Graphics::RendererRequirements *requirements, int idx = -1);
    Graphics::GraphicsError _createSingleSwapChain(Graphics::RendererRequirements *requirements, int idx);
    void _cleanupSwapChain(int idx = -1);
    void _cleanupSwapChainSingle(int idx);

    // Determines how to handle each type of error when updating a scene
    // Returns true if the frame should continue rendering, or false if the update should immediately stop
    bool _handleUpdateError(Graphics::GraphicsError error, Graphics::RendererScene_Base *scene);

    Graphics::GraphicsError _createTransferCommandPools();
    Graphics::GraphicsError _allocateTransferCommandBuffers(uint32_t queue, uint32_t count, CommandBufferArray *outArray);
    void _freeTransferCommandBuffers(CommandBufferArray *freeCommandBuffers);
    void _releaseTransferCommandBufferResources();

private:
    friend class VulkanShaderModule;
    friend class RendererSceneImpl_Basic; //TODO: Remove
    friend class VulkanCommandBuffer;

    APIImpl *m_api;
    VulkanPhysicalDevice *m_physicalDevice;
    VkDevice m_device;
    Graphics::RendererRequirements *m_requirements;

    typedef std::vector<std::pair<Graphics::Renderer_Base::OnDestroySwapChainFn, Graphics::Renderer_Base::OnCreateSwapChainFn>> SwapChainFuncArray;
    SwapChainFuncArray m_swapChainFuncs;

    uint32_t m_queueIndices[QueueType::QUEUE_COUNT];
    VkQueue m_queues[QueueType::QUEUE_COUNT];
    VkCommandPool m_commandPools[QueueType::QUEUE_COUNT];
    VkCommandPool m_transferCommandPools[QueueType::QUEUE_COUNT];

    typedef std::vector<VulkanSwapChain> SwapChainArray;
    SwapChainArray m_swapchains;
    uint32_t m_swapChainOutOfDate; // Bitfield of swapchains that need to be recreated

    typedef std::unordered_set<Graphics::RendererScene_Base*> SceneSet;
    SceneSet m_activeScenes;
    SceneSet m_inactiveScenes;
    SceneSet m_curFrameActiveScenes;
    SceneSet m_erroredScenes;

    bool m_useValidation;

    typedef std::vector<char const*> StringLiteralArray;
    StringLiteralArray m_vkExtensionsList;
    StringLiteralArray m_vkLayersList;

    struct TransferFuncDescription {
        QueueType queue;
        size_t commandBufferCount;
        TransferBeginFunc beginFunc;
        TransferEndFunc endFunc;
        TransferErrorFunc errorFunc;
        size_t commandBufferIndex;
    };
    typedef std::vector<TransferFuncDescription> TransferFunctions;
    TransferFunctions m_registeredTransfers;
    CommandBufferArray m_activeTransferCommandBuffers;
    CommandBufferArray m_freeTransferCommandBuffers;
};

} // namespace Vulkan
