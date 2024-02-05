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
