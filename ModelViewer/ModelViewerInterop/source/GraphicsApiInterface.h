#pragma once

#include "RendererRequirementsInterface.h"
#include "GraphicsDeviceInterface.h"

namespace Graphics {

class API_Base;

} // namespace Graphics

public interface class GraphicsApiInterface {
public:
    void Initialize(RendererRequirementsInterface ^requirements);
    GraphicsDeviceInterface ^FindSuitableDevice(RendererRequirementsInterface ^requirements);

    Graphics::API_Base * GetNativeApi();
};
