#include "pch.h"
#include "Win32WindowSurfaceInterop.h"

Win32WindowSurface::Win32WindowSurface() {

}

Win32WindowSurface::~Win32WindowSurface() {

}

void Win32WindowSurface::SetHInstance(System::IntPtr hInstance) {
    m_hInstance = hInstance;
}

System::IntPtr Win32WindowSurface::GetHInstance() {
    return m_hInstance;
}

void Win32WindowSurface::SetHWnd(System::IntPtr hWnd) {
    m_hWnd = hWnd;
}

System::IntPtr Win32WindowSurface::GetHWnd() {
    return m_hWnd;
}

WindowSurfaceInterface::SurfaceType Win32WindowSurface::GetType() {
    return WindowSurfaceInterface::SurfaceType::SURFACE_TYPE_WIN32;
}

