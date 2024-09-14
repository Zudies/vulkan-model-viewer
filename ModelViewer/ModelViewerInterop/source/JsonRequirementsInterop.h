#pragma once

#include "WindowSurfaceInterface.h"

namespace Graphics {

class JsonRendererRequirements;

} // namespace Graphics

public ref class JsonRequirements {
public:
    JsonRequirements();
    virtual ~JsonRequirements();

    void Initialize(System::String ^settingsFile);
    void AddWindowSurface(WindowSurfaceInterface ^window);

private:
    Graphics::JsonRendererRequirements *m_nativeRequirements;
    WindowSurfaceInterface ^m_windowSurface;
};
