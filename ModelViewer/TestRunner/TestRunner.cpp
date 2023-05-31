// TestRunner.cpp : This file contains the 'main' function. Program execution begins and ends there.
// A simple harness for testing Renderer dlls without integrating with WPF
//

#include "Common.h"
#include "ErrorCodes.h"
#include "WindowsFrameRateController.h"
#include "VulkanAPI.h"
#include "VulkanRenderer.h"
#include <iostream>
#include <thread>
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

    Performance::FrameRateController_Base *frameController = new Performance::WindowsFrameRateController;
    Performance::FrameRateControllerSettings frcSettings{};

    // Set the target FPS of the test window: 1 / Desired frame rate
    // Set to 0 to unlock frame rate
    frcSettings.DesiredFrameTime = 1.0 / 60.0;

    frameController->Initialize(&frcSettings);

    frameController->Reset();
    f64 timeSinceLastFrame = frcSettings.DesiredFrameTime;
    f64 fps = 0.0;
    f64 fpsUpdateTimer = 0.0;
    while (!g_close) {
        MSG msg;
        if (PeekMessage(&msg, g_hwnd, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        timeSinceLastFrame += frameController->GetElapsedTime();
        if (timeSinceLastFrame >= frcSettings.DesiredFrameTime) {
            // Run a frame
            renderer->Update(timeSinceLastFrame);

            // Update FPS (smooth the fps using 20% of current)
            fps = fps * 0.8 + 0.2 / timeSinceLastFrame;

            fpsUpdateTimer += timeSinceLastFrame;
            timeSinceLastFrame = 0.0;
        }
        else if (!PeekMessage(&msg, g_hwnd, 0, 0, PM_NOREMOVE)) {
            std::this_thread::yield();
        }

        if (fpsUpdateTimer >= 0.5) {
            static wchar_t buffer[64];
            swprintf_s(buffer, countof(buffer), L"%s (FPS: %6.2f)", WINDOW_TITLE, fps);
            SetWindowText(g_hwnd, buffer);
            fpsUpdateTimer = 0.0;
        }
    }

    return 0;
}
