#pragma once

namespace Graphics {

class API_Base {
public:
    API_Base();
    virtual ~API_Base() = 0;

    virtual GraphicsError Initialize() = 0;
    virtual GraphicsError Finalize() = 0;

};

} // namespace Graphics
