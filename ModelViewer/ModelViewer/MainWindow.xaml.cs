using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace ModelViewer
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            m_vulkanEngine = new VulkanRenderEngine();
        }

        private void OnLoaded(object sender, RoutedEventArgs routedEventArgs)
        {
            var mainWindowHandle = new WindowInteropHelper(this).Handle;

            HwndHost host = new RenderWindowHost();
            host.Name = "RenderWindow";
            RegisterName(host.Name, host); // This register is needed or the control can't be found with FindName
            ((Border)FindName("RenderWindowParent")).Child = host;
        }

        public void InitializeVulkanEngine()
        {
            IntPtr hinstance = Marshal.GetHINSTANCE(typeof(App).Module);
            RenderWindowHost? renderWindow = FindName("RenderWindow") as RenderWindowHost;
            // Note: This still refers to the parent hwnd rather than the window created in HwndHost
            //HwndSource hwnd = (HwndSource)HwndSource.FromVisual(renderWindow);

            if (renderWindow != null)
            {
                m_vulkanEngine.Initialize(hinstance, renderWindow.GetHwnd().Handle);
            }
        }

        private VulkanRenderEngine m_vulkanEngine;

        private void OnClosing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            m_vulkanEngine.Exit();
        }
    }
}
