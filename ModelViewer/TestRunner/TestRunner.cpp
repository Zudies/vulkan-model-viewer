// TestRunner.cpp : This file contains the 'main' function. Program execution begins and ends there.
// A simple harness for testing Renderer dlls without integrating with WPF
//

#include "Common.h"
#include "ErrorCodes.h"
#include "VulkanAPI.h"
#include "VulkanRenderer.h"
#include <iostream>
#include <windows.h>

namespace {
wchar_t const CLASS_NAME[] = L"Test Runner Class";
wchar_t const WINDOW_TITLE[] = L"Test Runner";
HWND g_hwnd = NULL;
HINSTANCE g_hinstance = NULL;
bool g_close = false;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CLOSE:
        g_close = true;
        break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

Graphics::GraphicsError CreateRenderWindow() {
    WNDCLASS wc = { };

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = g_hinstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    g_hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        WINDOW_TITLE,                   // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window    
        NULL,       // Menu
        g_hinstance,  // Instance handle
        NULL        // Additional application data
    );

    if (g_hwnd == NULL)
    {
        return Graphics::GraphicsError::UNKNOWN;
    }

    ShowWindow(g_hwnd, SW_NORMAL);

    return Graphics::GraphicsError::OK;
}

int main()
{
    CreateRenderWindow();

    LOG_INFO("Test Runner: Using Vulkan Renderer\n");
    Graphics::API_Base *api = new Vulkan::API;
    auto result = api->Initialize();
    ASSERT_MSG(result == Graphics::GraphicsError::OK, L"API Initialization Failed");

    Graphics::Renderer_Base *renderer = new Vulkan::Renderer;
    renderer->Initialize(api);

    while (!g_close) {
        MSG msg;
        if (PeekMessage(&msg, g_hwnd, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        //TODO: measure frame time to calculate dt
        renderer->Update(1.0f/60);
    }

    return 0;
}
