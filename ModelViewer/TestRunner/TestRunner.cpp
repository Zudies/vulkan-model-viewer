// TestRunner.cpp : This file contains the 'main' function. Program execution begins and ends there.
// A simple harness for testing Renderer dlls without integrating with WPF
//

#include "Common.h"
#include "ErrorCodes.h"
#include "WindowsFrameRateController.h"
#include "VulkanAPI.h"
#include "VulkanRenderer.h"
#include "VulkanRendererScene_Basic.h"
#include "JsonRendererRequirements.h"
#include "Win32WindowSurface.h"
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
    g_hinstance = GetModuleHandle(NULL);
    CreateRenderWindow();

    // Load the renderer requirements for Vulkan
    Graphics::Win32WindowSurface windowSurface(g_hwnd, g_hinstance);
    Graphics::JsonRendererRequirements requirements;
    requirements.Initialize("resources/model-viewer-renderer.json");
    requirements.AddWindowSurface(&windowSurface);

    // Initialize the Vulkan API
    LOG_INFO("Test Runner: Using Vulkan Renderer\n");
    Graphics::API_Base *api = new Vulkan::API;
    auto result = api->Initialize(&requirements);
    ASSERT_MSG(result == Graphics::GraphicsError::OK, L"API Initialization Failed");

    // Find a suitable device
    auto *physicalDevice = api->FindSuitableDevice(&requirements);

    // Initialize the renderer now that the API is initialized and we have a physical device
    Graphics::Renderer_Base *renderer = new Vulkan::Renderer;
    renderer->Initialize(api, physicalDevice, &requirements);

    // Create a basic scene to be rendered
    Graphics::RendererScene_Base *scene = new Vulkan::RendererScene_Basic;
    scene->Initialize(renderer);

    renderer->SetSceneActive(scene);

    Performance::FrameRateController_Base *frameController = new Performance::WindowsFrameRateController;
    Performance::FrameRateControllerSettings frcSettings{};

    // Set the target FPS of the test window: 1 / Desired frame rate
    // Set to 0 to unlock frame rate
    frcSettings.DesiredFrameTime = 0.0;// 1.0 / 60.0;

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
            //TODO: Handle errors
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

    scene->Finalize();
    delete scene;
    renderer->Finalize();
    delete renderer;
    api->Finalize();
    delete api;

    return 0;
}
