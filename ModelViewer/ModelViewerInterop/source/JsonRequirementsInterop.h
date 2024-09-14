#pragma once

#include "RendererRequirementsInterface.h"

using namespace System::Collections::Generic;

namespace Graphics {

class JsonRendererRequirements;
class WindowSurface;

} // namespace Graphics

public ref class JsonRequirements : public RendererRequirementsInterface {
public:
    JsonRequirements();
    virtual ~JsonRequirements();

    virtual void Initialize(System::String ^settingsFile);
    virtual void AddWindowSurface(WindowSurfaceInterface ^window);

    virtual Graphics::RendererRequirements *GetNativeRequirements();

private:
    Graphics::JsonRendererRequirements *m_nativeRequirements;
    List<WindowSurfaceInterface^> m_surfaces;
};
