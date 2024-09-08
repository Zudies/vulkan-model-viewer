#pragma once

namespace Graphics {

class Renderer_Base;

/*
  Renderer Scene class maintains a scene and necessary pipelines/states to render said scene
  This class is responsible for owning and managing the pipeline and resources (textures, models, etc.) necessary to render itself
 */
class RendererScene_Base {
public:
    RendererScene_Base();
    RendererScene_Base(RendererScene_Base const &) = delete;
    RendererScene_Base &operator=(RendererScene_Base const &) = delete;
    virtual ~RendererScene_Base() = 0;

    virtual GraphicsError Initialize(Renderer_Base *parentRenderer) = 0;
    virtual GraphicsError Finalize() = 0;
    virtual GraphicsError Update(f64 deltaTime) = 0;

};

} // namespace Graphics
