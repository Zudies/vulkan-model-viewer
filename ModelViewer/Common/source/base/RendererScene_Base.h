#pragma once

namespace Graphics {

/*
  Renderer Scene class maintains a scene and necessary pipelines/states to render said scene
 */
class RendererScene_Base {
public:
    RendererScene_Base();
    RendererScene_Base(RendererScene_Base const &) = delete;
    RendererScene_Base &operator=(RendererScene_Base const &) = delete;
    virtual ~RendererScene_Base() = 0;

    virtual GraphicsError Initialize() = 0;
    virtual GraphicsError Finalize() = 0;
    virtual GraphicsError Update(f64 deltaTime) = 0;

};

} // namespace Graphics
