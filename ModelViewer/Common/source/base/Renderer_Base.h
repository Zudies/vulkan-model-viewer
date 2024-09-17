#pragma once

#include <functional>

namespace Graphics {

class API_Base;
class PhysicalDevice;
class RendererRequirements;
class RendererScene_Base;

/*
  Renderer class creates and maintains a logical device for rendering
  This class is responsible for owning and maintaining general graphics resources (command pools, memory pools, etc.)
 */
class Renderer_Base {
public:
    Renderer_Base();
    Renderer_Base(Renderer_Base const &) = delete;
    Renderer_Base &operator=(Renderer_Base const &) = delete;
    virtual ~Renderer_Base() = 0;

    virtual GraphicsError Initialize(API_Base *api, PhysicalDevice *physicalDevice, RendererRequirements *requirements) = 0;
    virtual GraphicsError Finalize() = 0;
    virtual GraphicsError Update(f64 deltaTime) = 0;
    virtual void SetSceneActive(RendererScene_Base *activeScene) = 0;
    virtual void SetSceneInactive(RendererScene_Base *inactiveScene) = 0;

    //TODO: Should this be here or specific to Vulkan?
    // Called when a swap chain recreation is necessary
    // OnDestroySwapChainFn should be called first before the renderer destroys the swap chain
    // OnCreateSwapChainFn should be called only after the renderer as successfully recreated the swap chain
    typedef std::function<GraphicsError(int)> OnDestroySwapChainFn;
    typedef std::function<GraphicsError(int)> OnCreateSwapChainFn;
    virtual void RegisterOnRecreateSwapChainFunc(OnDestroySwapChainFn destroyFunc, OnCreateSwapChainFn createFunc) = 0;

};

} // namespace Graphics
