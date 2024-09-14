#pragma once

#include "GraphicsApiInterface.h"
#include "RendererRequirementsInterface.h"
#include "GraphicsDeviceInterface.h"
#include "GraphicsSceneInterface.h"

namespace Graphics {

class Renderer_Base;

} // namespace Graphics

public interface class GraphicsRendererInterface {
public:
    void Initialize(GraphicsApiInterface ^api, GraphicsDeviceInterface ^device, RendererRequirementsInterface ^requirements);
    void SetSceneActive(GraphicsSceneInterface ^scene);

    Graphics::Renderer_Base *GetNativeRenderer();

    void Update(float dt);
};
