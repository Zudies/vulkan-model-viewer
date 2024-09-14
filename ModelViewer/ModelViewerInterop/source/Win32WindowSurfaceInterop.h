#pragma once

#include "WindowSurfaceInterface.h"

namespace Graphics {

class Win32WindowSurface;

} // namespace Graphics

public ref class Win32WindowSurface : public WindowSurfaceInterface {
public:
    Win32WindowSurface();
    virtual ~Win32WindowSurface();

    void SetHInstance(System::IntPtr hInstance);
    System::IntPtr GetHInstance();
    void SetHWnd(System::IntPtr hWnd);
    System::IntPtr GetHWnd();

    virtual Graphics::WindowSurface *GetNativeSurface();

    virtual WindowSurfaceInterface::SurfaceType GetType();

private:
    Graphics::Win32WindowSurface *m_nativeSurface;
};
