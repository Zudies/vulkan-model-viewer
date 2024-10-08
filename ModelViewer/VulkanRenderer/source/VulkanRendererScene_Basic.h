#pragma once

#include "base/RendererScene_Base.h"

namespace Graphics {

class Camera;

}

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
    virtual Graphics::GraphicsError EarlyUpdate(f64 deltaTime) override;
    virtual Graphics::GraphicsError Update(f64 deltaTime) override;
    virtual Graphics::GraphicsError LateUpdate(f64 deltaTime) override;

    // Functions for setting and fetching UI values
    // Note: These always take as parameters native values and return native values
    std::string GetPipelineStateValue(const std::string &pipelineState);
    void SetPipelineStateValue(const std::string &pipelineState, const std::string &pipelineStateValue);
    Graphics::Camera *GetCamera();

private:
    RendererSceneImpl_Basic *m_impl;

};

} // namespace Vulkan
