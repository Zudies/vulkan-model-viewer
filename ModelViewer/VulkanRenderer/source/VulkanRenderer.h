#pragma once

namespace Vulkan {

class Renderer {
public:
    Renderer();
    ~Renderer();

    virtual Graphics::GraphicsError Initialize();
    virtual Graphics::GraphicsError Finalize();
    virtual Graphics::GraphicsError Update(f32 deltaTime);

private:

};

} // namespace Vulkan
