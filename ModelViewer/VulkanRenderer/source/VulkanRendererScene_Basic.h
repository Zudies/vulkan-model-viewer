#pragma once

#include "base/RendererScene_Base.h"

namespace Vulkan {

class RendererSceneImpl_Basic;

/*
  Basic forward rendering of a hard-coded primitive
 */
class RendererScene_Basic : public Graphics::RendererScene_Base {
public:
    RendererScene_Basic();
    virtual ~RendererScene_Basic();

    virtual Graphics::GraphicsError Initialize(Graphics::Renderer_Base *parentRenderer) override;
    virtual Graphics::GraphicsError Finalize() override;
    virtual Graphics::GraphicsError Update(f64 deltaTime) override;

private:
    RendererSceneImpl_Basic *m_impl;

};

} // namespace Vulkan
