#pragma once

#include "WindowSurfaceInterface.h"

namespace Graphics {

class RendererRequirements;

} // namespace Graphics

public interface class RendererRequirementsInterface {
public:
    void Initialize(System::String ^settingsFile);
    //TODO: Initialize by in-memory file
    void AddWindowSurface(WindowSurfaceInterface ^window);

    Graphics::RendererRequirements *GetNativeRequirements();
};
