#include "pch.h"
#include "Win32WindowSurfaceInterop.h"

#include "Win32WindowSurface.h"

Win32WindowSurface::Win32WindowSurface()
  : m_nativeSurface(new Graphics::Win32WindowSurface) {
}

Win32WindowSurface::~Win32WindowSurface() {
    delete m_nativeSurface;
}

void Win32WindowSurface::SetHInstance(System::IntPtr hInstance) {
    m_nativeSurface->SetHinstance(reinterpret_cast<HINSTANCE>(hInstance.ToPointer()));
}

System::IntPtr Win32WindowSurface::GetHInstance() {
    return System::IntPtr(m_nativeSurface->GetHinstance());
}

void Win32WindowSurface::SetHWnd(System::IntPtr hWnd) {
    m_nativeSurface->SetHwnd(reinterpret_cast<HWND>(hWnd.ToPointer()));
}

System::IntPtr Win32WindowSurface::GetHWnd() {
    return System::IntPtr(m_nativeSurface->GetHwnd());
}

Graphics::WindowSurface *Win32WindowSurface::GetNativeSurface() {
    return m_nativeSurface;
}

WindowSurfaceInterface::SurfaceType Win32WindowSurface::GetType() {
    return WindowSurfaceInterface::SurfaceType::SURFACE_TYPE_WIN32;
}

