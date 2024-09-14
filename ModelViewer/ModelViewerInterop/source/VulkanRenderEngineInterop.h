ref class JsonRequirements;
ref class VulkanApi;
ref class VulkanRenderer;
interface class GraphicsSceneInterface;

using namespace System::Collections::Generic;
using namespace System::Threading;

public ref class VulkanRenderEngine {

    //TODO: Read from UI
    // Set the max FPS of the rendering engine
    // Set to 0 to unlock frame rate
    static f64 MAX_FRAME_RATE = 0.0f;// 60.0f;

public:
    VulkanRenderEngine();
    ~VulkanRenderEngine();

    void Initialize(System::IntPtr hInstance, System::IntPtr hWnd);
    void Exit();

    f64 GetFps();

private:
    void UpdateThreadMain();

private:
    JsonRequirements ^m_requirements;
    VulkanApi ^m_api;
    VulkanRenderer ^m_renderer;
    List<GraphicsSceneInterface^> m_scenes;

    bool m_shouldExit;
    Thread ^m_updateThread;
    ReaderWriterLockSlim ^m_exitLock;
    ReaderWriterLockSlim ^m_fpsLock;
    f64 m_fps;
};
