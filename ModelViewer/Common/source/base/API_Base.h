#pragma once

namespace Graphics {

/*
  API class is responsible for initializing a graphics API, making its functions callable
  This can include loading a DLL and loading function pointers, or any other boilerplate
   initialization that needs to occur for that particular API (setting versions/paths/etc.)
  API class is NOT responsible for creating a physical device or any work that would be related
   to initializing a graphics pipeline. Any per-device work should go into the Renderer
 */
class API_Base {
public:
    API_Base();
    virtual ~API_Base() = 0;

    virtual GraphicsError Initialize() = 0;
    virtual GraphicsError Finalize() = 0;

};

} // namespace Graphics
