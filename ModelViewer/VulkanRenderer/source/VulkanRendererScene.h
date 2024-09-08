#pragma once

#include "base/RendererScene_Base.h"

namespace Vulkan {

class RendererSceneImpl;

/*
  Renderer Scene class maintains a scene and necessary pipelines/states to render said scene
 */
class RendererScene : public Graphics::RendererScene_Base {
public:
    RendererScene();
    virtual ~RendererScene();

    virtual Graphics::GraphicsError Initialize() override;
    virtual Graphics::GraphicsError Finalize() override;
    virtual Graphics::GraphicsError Update(f64 deltaTime) override;

private:
    RendererSceneImpl *m_impl;

};

} // namespace Vulkan
