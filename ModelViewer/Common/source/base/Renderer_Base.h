#pragma once

namespace Graphics {

class API_Base;

class Renderer_Base {
public:
    Renderer_Base();
    virtual ~Renderer_Base() = 0;

    virtual Graphics::GraphicsError Initialize(API_Base *api) = 0;
    virtual Graphics::GraphicsError Finalize() = 0;
    virtual Graphics::GraphicsError Update(f32 deltaTime);

};

} // namespace Graphics
