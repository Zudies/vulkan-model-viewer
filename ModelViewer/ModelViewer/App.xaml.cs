using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Controls;

namespace ModelViewer
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        private void Application_Startup(object sender, StartupEventArgs e)
        {
            Console.WriteLine("Application_Startup");

            MainWindow wnd = new MainWindow();

            if (e.Args.Length > 0)
            {
                //TODO: Command line args
            }

            wnd.Show();

            // Load requirements
            RendererRequirementsInterface vulkanRequirements = new JsonRequirements();
            vulkanRequirements.Initialize("resources/model-viewer-renderer.json");

            Win32WindowSurface windowSurface = new Win32WindowSurface();
            HwndSource hwnd = (HwndSource)HwndSource.FromVisual((wnd.FindName("RenderWindow") as Canvas));
            IntPtr hinstance = Marshal.GetHINSTANCE(typeof(App).Module);
            windowSurface.SetHWnd(hwnd.Handle);
            windowSurface.SetHInstance(hinstance);
            vulkanRequirements.AddWindowSurface(windowSurface);

            // Initialize Vulkan API
            //TODO:
            GraphicsApiInterface vulkanApi = new VulkanApi();
            vulkanApi.Initialize(vulkanRequirements);
            GraphicsDeviceInterface vulkanDevice = vulkanApi.FindSuitableDevice(vulkanRequirements);

            // Initialize renderer
            GraphicsRendererInterface vulkanRenderer = new VulkanRenderer();
            vulkanRenderer.Initialize(vulkanApi, vulkanDevice, vulkanRequirements);

            // Initialize and register the scene
            //TODO:
            GraphicsSceneInterface testScene = new VulkanBasicScene();
            testScene.Initialize(vulkanRenderer);
            vulkanRenderer.SetSceneActive(testScene);

            //TODO: where to host main loop?

        }
    }
}
