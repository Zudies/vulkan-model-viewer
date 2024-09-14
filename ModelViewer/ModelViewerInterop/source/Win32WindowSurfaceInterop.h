#pragma once

#include "WindowSurfaceInterface.h"

public ref class Win32WindowSurface : public WindowSurfaceInterface {
public:
    Win32WindowSurface();
    virtual ~Win32WindowSurface();

    void SetHInstance(System::IntPtr hInstance);
    System::IntPtr GetHInstance();
    void SetHWnd(System::IntPtr hWnd);
    System::IntPtr GetHWnd();

    virtual WindowSurfaceInterface::SurfaceType GetType();

private:
    System::IntPtr m_hInstance;
    System::IntPtr m_hWnd;
};
