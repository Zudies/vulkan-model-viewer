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

private:
    RendererImpl *m_impl;
};

} // namespace Vulkan
