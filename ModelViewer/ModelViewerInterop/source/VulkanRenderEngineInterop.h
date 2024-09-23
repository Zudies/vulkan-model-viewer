#pragma once

#include "GraphicsEngineToUIInterface.h"

ref class JsonRequirements;
ref class VulkanApi;
ref class VulkanRenderer;
interface class GraphicsSceneInterface;
ref class CameraInputController;

using namespace System::Collections::Generic;
using namespace System::Threading;

public ref class VulkanRenderEngine : public GraphicsEngineToUIInterface {

    //TODO: Read from UI
    // Set the max FPS of the rendering engine
    // Set to 0 to unlock frame rate
    static f64 MAX_FRAME_RATE = 0.0f;// 60.0f;

public:
    VulkanRenderEngine();
    ~VulkanRenderEngine();

    virtual System::String ^GetEngineValue(System::String ^contentId);
    virtual void SetEngineValue(System::String ^contentId, System::String ^contentValue);

    void Initialize(System::IntPtr hInstance, System::IntPtr hWnd);
    void Exit();

    f64 GetFps();

    CameraInputController ^GetCameraController();

private:
    void UpdateThreadMain();

    std::string _idNameToNativeName(System::String ^contentId);
    System::String ^_nativeEngineValueToManagedContent(const std::string &nativeValue);
    std::string _managedContentToNativeEngineValue(System::String ^contentValue);

private:
    JsonRequirements ^m_requirements;
    VulkanApi ^m_api;
    VulkanRenderer ^m_renderer;
    List<GraphicsSceneInterface^> m_scenes;
    GraphicsSceneInterface ^m_activeScene;

    bool m_shouldExit;
    Thread ^m_updateThread;
    ReaderWriterLockSlim ^m_exitLock;
    ReaderWriterLockSlim ^m_fpsLock;
    f64 m_fps;

    CameraInputController ^m_cameraController;
};
