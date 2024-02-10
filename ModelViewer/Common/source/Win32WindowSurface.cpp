#include "pch.h"
#include "Win32WindowSurface.h"

namespace Graphics {

Win32WindowSurface::Win32WindowSurface()
: m_hwnd(NULL),
  m_hinstance(NULL) {
}

Win32WindowSurface::Win32WindowSurface(HWND hwnd, HINSTANCE hinstance)
: m_hwnd(hwnd),
  m_hinstance(hinstance) {
}

Win32WindowSurface::~Win32WindowSurface() {
}

bool Win32WindowSurface::Compare(WindowSurface const &other) const {
    if (GetType() != other.GetType()) {
        return GetType() < other.GetType();
    }
    Win32WindowSurface const *otherW32 = static_cast<Win32WindowSurface const *>(&other);
    if (m_hwnd != otherW32->m_hwnd) {
        return m_hwnd < otherW32->m_hwnd;
    }
    return m_hinstance < otherW32->m_hinstance;
}

WindowSurfaceType Win32WindowSurface::GetType() const {
    return WindowSurfaceType::SURFACE_WIN32;
}

void Win32WindowSurface::SetHwnd(HWND hwnd) {
    m_hwnd = hwnd;
}

HWND Win32WindowSurface::GetHwnd() const {
    return m_hwnd;
}

void Win32WindowSurface::SetHinstance(HINSTANCE hinstance) {
    m_hinstance = hinstance;
}

HINSTANCE Win32WindowSurface::GetHinstance() const {
    return m_hinstance;
}

} // namespace Graphics
