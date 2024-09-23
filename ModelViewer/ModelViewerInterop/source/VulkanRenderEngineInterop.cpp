#include "pch.h"
#include "VulkanRenderEngineInterop.h"

#include "JsonRequirementsInterop.h"
#include "Win32WindowSurfaceInterop.h"
#include "VulkanApiInterop.h"
#include "VulkanRendererInterop.h"
#include "VulkanBasicSceneInterop.h"

#include "WindowsFrameRateController.h"

#include "VulkanRendererScene_Basic.h"

#include "VulkanToManagedConversion.h"

#include "CameraInputController.h"

#include "../ext/glm/glm.hpp"
#include "Camera.h"

VulkanRenderEngine::VulkanRenderEngine()
  : m_requirements(nullptr),
    m_api(nullptr),
    m_renderer(nullptr),
    m_activeScene(nullptr),
    m_shouldExit(false),
    m_fps(0.0),
    m_cameraController(nullptr) {
    m_updateThread = gcnew Thread(gcnew ThreadStart(this, &VulkanRenderEngine::UpdateThreadMain));
    m_exitLock = gcnew ReaderWriterLockSlim(LockRecursionPolicy::SupportsRecursion);
    m_fpsLock = gcnew ReaderWriterLockSlim(LockRecursionPolicy::SupportsRecursion);
    m_cameraController = gcnew CameraInputController();
}

VulkanRenderEngine::~VulkanRenderEngine() {

}

std::string VulkanRenderEngine::_idNameToNativeName(System::String ^contentId) {
    for (int i = 0; i < sizeof(ContentNameMapping) / sizeof(ContentNameMapping[0]); ++i) {
        if (contentId->Equals(gcnew System::String(ContentNameMapping[i].first))) {
            return ContentNameMapping[i].second;
        }
    }

    return "";
}

System::String ^VulkanRenderEngine::_nativeEngineValueToManagedContent(const std::string &nativeValue) {
    for (int i = 0; i < sizeof(ContentValueMapping) / sizeof(ContentValueMapping[0]); ++i) {
        if (nativeValue == ContentValueMapping[i].second) {
            return gcnew System::String(ContentValueMapping[i].first);
        }
    }

    return nullptr;
}

std::string VulkanRenderEngine::_managedContentToNativeEngineValue(System::String ^contentValue) {
    for (int i = 0; i < sizeof(ContentValueMapping) / sizeof(ContentValueMapping[0]); ++i) {
        if (contentValue->Equals(gcnew System::String(ContentValueMapping[i].first))) {
            return ContentValueMapping[i].second;
        }
    }

    return "";
}

System::String ^VulkanRenderEngine::GetEngineValue(System::String ^contentId) {
    Vulkan::RendererScene_Basic *scene = static_cast<Vulkan::RendererScene_Basic *>(m_activeScene->GetNativeScene());

    if (contentId->Equals("ID_CAMERA_POS_X")) {
        return System::String::Format("{0:F3}", scene->GetCamera()->GetPosition().x);
    }
    else if (contentId->Equals("ID_CAMERA_POS_Y")) {
        return System::String::Format("{0:F3}", scene->GetCamera()->GetPosition().y);
    }
    else if (contentId->Equals("ID_CAMERA_POS_Z")) {
        return System::String::Format("{0:F3}", scene->GetCamera()->GetPosition().z);
    }
    else if (contentId->Equals("ID_POLYGON_MODE")) {
        return _nativeEngineValueToManagedContent(scene->GetPipelineStateValue(_idNameToNativeName(contentId)));
    }
    else if (contentId->Equals("ID_CULL_MODE")) {
        return _nativeEngineValueToManagedContent(scene->GetPipelineStateValue(_idNameToNativeName(contentId)));
    } 

    System::Console::WriteLine(System::String::Format("ERROR: Unable to get VK engine value of {0}", contentId));
    return nullptr;
}

void VulkanRenderEngine::SetEngineValue(System::String ^contentId, System::String ^contentValue) {
    Vulkan::RendererScene_Basic *scene = static_cast<Vulkan::RendererScene_Basic *>(m_activeScene->GetNativeScene());

    if (contentId->Equals("ID_POLYGON_MODE")) {
        scene->SetPipelineStateValue(_idNameToNativeName(contentId), _managedContentToNativeEngineValue(contentValue));
    }
    else if (contentId->Equals("ID_CULL_MODE")) {
        scene->SetPipelineStateValue(_idNameToNativeName(contentId), _managedContentToNativeEngineValue(contentValue));
    }
    else if (contentId->Equals("ID_CAMERA_FOV")) {
        scene->GetCamera()->SetVerticalFOVDeg(static_cast<f32>(System::Convert::ToDouble(contentValue)));
    }
    else {
        System::Console::WriteLine(System::String::Format("ERROR: Unable to set VK engine value for {0} = {1}", contentId, contentValue));
    }
}

void VulkanRenderEngine::Initialize(System::IntPtr hInstance, System::IntPtr hWnd) {
    /* Perform Vulkan initialization */
    //TODO: error handling
    // Load requirements
    m_requirements = gcnew JsonRequirements;
    m_requirements->Initialize("resources/model-viewer-renderer.json");

    Win32WindowSurface ^windowSurface = gcnew Win32WindowSurface;
    windowSurface->SetHWnd(hWnd);
    windowSurface->SetHInstance(hInstance);

    m_requirements->AddWindowSurface(windowSurface);

    // Initialize Vulkan API
    m_api = gcnew VulkanApi;
    m_api->Initialize(m_requirements);

    GraphicsDeviceInterface ^vulkanDevice = m_api->FindSuitableDevice(m_requirements);

    // Initialize renderer
    m_renderer = gcnew VulkanRenderer;
    m_renderer->Initialize(m_api, vulkanDevice, m_requirements);
    //TODO: Command pools are created here but being used in the update thread
    //      This theoretically shouldn't cause any problems because after initialization
    //        there is no more rendering occurring on other threads

    // Initialize and register the scene
    //TODO: Move this somewhere else to be controlled by UI
    GraphicsSceneInterface ^testScene = gcnew VulkanBasicScene;
    testScene->Initialize(m_renderer);
    m_renderer->SetSceneActive(testScene);
    m_scenes.Add(testScene);

    // Only have one scene right now
    m_activeScene = testScene;

    // Start the main rendering thread
    m_updateThread->Start();
}

void VulkanRenderEngine::Exit() {
    m_exitLock->EnterWriteLock();
    m_shouldExit = true;
    m_exitLock->ExitWriteLock();
}

f64 VulkanRenderEngine::GetFps() {
    m_fpsLock->EnterReadLock();
    f64 result = m_fps;
    m_fpsLock->ExitReadLock();
    return result;
}

CameraInputController ^VulkanRenderEngine::GetCameraController() {
    return m_cameraController;
}

void VulkanRenderEngine::UpdateThreadMain() {
    // Initialize frame rate controller
    Performance::FrameRateControllerSettings frcSettings{};
    
    frcSettings.DesiredFrameTime = 0.0;
    if (MAX_FRAME_RATE > 0.0) {
        frcSettings.DesiredFrameTime = 1.0 / MAX_FRAME_RATE;
    }

    Performance::WindowsFrameRateController frameController;
    frameController.Initialize(&frcSettings);
    frameController.Reset();

    f64 timeSinceLastFrame = frcSettings.DesiredFrameTime;

    // Start loop
    m_exitLock->EnterReadLock();
    while (!m_shouldExit) {
        m_exitLock->ExitReadLock();

        timeSinceLastFrame += frameController.GetElapsedTime();
        if (timeSinceLastFrame >= frcSettings.DesiredFrameTime) {
            // Cap frame time to prevent spikes
            f64 effectiveDt = std::min(timeSinceLastFrame, frcSettings.DesiredFrameTime != 0.0 ? 2 * frcSettings.DesiredFrameTime : 1.0 / 20);

            // Run a frame
            // Update camera controller
            if (m_cameraController->IsActive()) {
                MousePos mouseDelta = m_cameraController->GetMouseDelta();
                MovementValue cameraMovement = m_cameraController->GetMovement();
                bool useFixedUpDir = m_cameraController->IsFixedUpDir();

                Vulkan::RendererScene_Basic *scene = static_cast<Vulkan::RendererScene_Basic *>(m_activeScene->GetNativeScene());
                Graphics::Camera *nativeCamera = scene->GetCamera();

                // Rotate first and then move
                if (useFixedUpDir) {
                    nativeCamera->UpdateVerticalAngleDeg(mouseDelta.y * static_cast<float>(effectiveDt));
                    nativeCamera->UpdateHorizontalAngleDeg(mouseDelta.x * static_cast<float>(effectiveDt));
                }
                else {
                    nativeCamera->UpdateYawPitchRollDeg(mouseDelta.x * static_cast<float>(effectiveDt), mouseDelta.y * static_cast<float>(effectiveDt), 0.0f);
                }

                nativeCamera->MoveLocal(glm::vec3(cameraMovement.x, cameraMovement.y, cameraMovement.z) * static_cast<float>(effectiveDt));

                m_cameraController->ResetMousePos();
            }

            // Render the frame
            //TODO: Handle errors
            m_renderer->Update(static_cast<float>(effectiveDt));

            // Update FPS (smooth the fps using a % of current)
            m_fpsLock->EnterWriteLock();
            m_fps = m_fps * 0.95 + 0.05 / timeSinceLastFrame;
            m_fpsLock->ExitWriteLock();

            timeSinceLastFrame = 0.0;
        }

        m_exitLock->EnterReadLock();
    }
    m_exitLock->ExitReadLock();

    // Clean up
    //TODO:
}
