#pragma once

#include "base/Renderer_Base.h"

namespace Vulkan {

class RendererImpl;

class Renderer : public Graphics::Renderer_Base {
public:
    Renderer();
    virtual ~Renderer();

    virtual Graphics::GraphicsError Initialize(Graphics::API_Base *api, Graphics::PhysicalDevice *physicalDevice, Graphics::RendererRequirements *requirements) override;
    virtual Graphics::GraphicsError Finalize() override;
    virtual Graphics::GraphicsError Update(f64 deltaTime) override;
    virtual void SetSceneActive(Graphics::RendererScene_Base *activeScene) override;
    virtual void SetSceneInactive(Graphics::RendererScene_Base *inactiveScene) override;

    virtual void RegisterOnRecreateSwapChainFunc(OnDestroySwapChainFn destroyFunc, OnCreateSwapChainFn createFunc) override;

    RendererImpl *GetImpl();

private:
    RendererImpl *m_impl;
};

} // namespace Vulkan
