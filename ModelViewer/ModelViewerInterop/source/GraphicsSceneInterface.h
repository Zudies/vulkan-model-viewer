#pragma once

interface class GraphicsRendererInterface;

namespace Graphics {

class RendererScene_Base;

} // namespace Graphics

public interface class GraphicsSceneInterface  {
public:
    void Initialize(GraphicsRendererInterface ^renderer);
    Graphics::RendererScene_Base *GetNativeScene();
};
