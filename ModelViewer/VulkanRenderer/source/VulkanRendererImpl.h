#pragma once

namespace Vulkan {

class API;
class APIImpl;

class RendererImpl {
public:
    RendererImpl();
    ~RendererImpl();

    Graphics::GraphicsError Initialize(API *api);
    Graphics::GraphicsError Finalize();
    Graphics::GraphicsError Update(f32 deltaTime);

private:
    APIImpl *m_api;
    

};

} // namespace Vulkan
