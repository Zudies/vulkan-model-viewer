#pragma once

namespace Vulkan {

/*
  Renderer Scene class maintains a scene and necessary pipelines/states to render said scene
 */
class RendererSceneImpl {
public:
    RendererSceneImpl();
    ~RendererSceneImpl();

    Graphics::GraphicsError Initialize();
    Graphics::GraphicsError Finalize();
    Graphics::GraphicsError Update(f64 deltaTime);

private:
    friend class Renderer;
};

} // namespace Vulkan
