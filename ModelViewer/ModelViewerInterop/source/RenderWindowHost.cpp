#include "pch.h"
#include "RenderWindowHost.h"

namespace {
bool g_close;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CLOSE:
        g_close = true;
        break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

RenderWindowHost::RenderWindowHost()
  : m_hwnd(nullptr) {

}

RenderWindowHost::~RenderWindowHost() {

}

HandleRef RenderWindowHost::GetHwnd() {
    return HandleRef(this, IntPtr(m_hwnd));
}

HandleRef RenderWindowHost::BuildWindowCore(HandleRef hwndParent) {
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = L"ModelViewerInner";
    RegisterClass(&wc);

    m_hwnd = CreateWindowEx(
        0,                   // Optional window styles.
        L"ModelViewerInner", // Window class
        L"ModelViewerInner", // Window text
        WS_CHILD | WS_VISIBLE | WS_MAXIMIZE, // Window style

        // Size and position
        0, 0, CW_USEDEFAULT, CW_USEDEFAULT,

        reinterpret_cast<HWND>(hwndParent.Handle.ToPointer()), // Parent window
        NULL,       // Menu
        NULL,       // Instance handle
        NULL        // Additional application data
    );

    return HandleRef(this, IntPtr(m_hwnd));
}

void RenderWindowHost::DestroyWindowCore(HandleRef) {
    // Do nothing, parent will clean up hwnd
}
