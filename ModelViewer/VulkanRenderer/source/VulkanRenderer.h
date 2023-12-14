#pragma once

#include "base/Renderer_Base.h"

namespace Vulkan {

class RendererImpl;

class Renderer : public Graphics::Renderer_Base {
public:
    Renderer();
    virtual ~Renderer();

    virtual Graphics::GraphicsError Initialize(Graphics::API_Base *api, Graphics::PhysicalDevice *physicalDevice);
    virtual Graphics::GraphicsError Finalize();
    virtual Graphics::GraphicsError Update(f64 deltaTime);

private:
    RendererImpl *m_impl;
};

} // namespace Vulkan
