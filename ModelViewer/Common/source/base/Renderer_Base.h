#pragma once

namespace Graphics {

class API_Base;
class PhysicalDevice;

/*
  Renderer class creates and maintains a logical device for rendering
  Rendering requests should generally be made to this class
 */
class Renderer_Base {
public:
    Renderer_Base();
    virtual ~Renderer_Base() = 0;

    virtual Graphics::GraphicsError Initialize(API_Base *api, PhysicalDevice *physicalDevice) = 0;
    virtual Graphics::GraphicsError Finalize() = 0;
    virtual Graphics::GraphicsError Update(f64 deltaTime) = 0;

};

} // namespace Graphics
