#pragma once

namespace Graphics {

class API_Base;
class PhysicalDevice;
class RendererRequirements;
class RendererScene_Base;

/*
  Renderer class creates and maintains a logical device for rendering
  Rendering requests should generally be made to this class
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

};

} // namespace Graphics
