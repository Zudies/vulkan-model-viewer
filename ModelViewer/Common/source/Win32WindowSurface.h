#pragma once

#include "base/WindowSurface.h"
#include <windef.h>

namespace Graphics {

class Win32WindowSurface : public WindowSurface {
public:
    Win32WindowSurface();
    Win32WindowSurface(HWND hwnd, HINSTANCE hinstance);
    virtual ~Win32WindowSurface();

    virtual bool Compare(WindowSurface const &other) const override;

    virtual WindowSurfaceType GetType() const override;

    void SetHwnd(HWND hwnd);
    HWND GetHwnd() const;

    void SetHinstance(HINSTANCE hinstance);
    HINSTANCE GetHinstance() const;

private:
    HWND m_hwnd;
    HINSTANCE m_hinstance;
};

}