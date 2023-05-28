#pragma once

namespace Graphics {

class Renderer_Base {
public:
    Renderer_Base();
    virtual ~Renderer_Base() = 0;

    virtual void Initialize() = 0;
    virtual void Finalize() = 0;

};

} // namespace Graphics
