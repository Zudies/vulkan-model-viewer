using System;
using System.Collections.Generic;
using System.Linq;
using System.Resources;
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
using System.Windows.Threading;

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
            Title = "Vulkan Model Viewer v" + Version.String;
        }

        private void OnLoaded(object sender, RoutedEventArgs routedEventArgs)
        {
            var mainWindowHandle = new WindowInteropHelper(this).Handle;

            HwndHost host = new RenderWindowHost();
            host.Name = "RenderWindow";
            RegisterName(host.Name, host); // This register is needed or the control can't be found with FindName
            ((Border)FindName("RenderWindowParent")).Child = host;
        }

        public void Initialize()
        {
            m_fpsLabelTimer = new DispatcherTimer();
            m_fpsLabelTimer.Interval = TimeSpan.FromSeconds(0.5);

            // Initialize desired engine
            InitializeVulkanEngine();

            m_fpsLabelTimer.Start();
        }

        public void InitializeVulkanEngine()
        {
            m_vulkanEngine = new VulkanRenderEngine();

            IntPtr hinstance = Marshal.GetHINSTANCE(typeof(App).Module);
            RenderWindowHost? renderWindow = FindName("RenderWindow") as RenderWindowHost;
            // Note: This still refers to the parent hwnd rather than the window created in HwndHost
            //HwndSource hwnd = (HwndSource)HwndSource.FromVisual(renderWindow);

            if (renderWindow != null)
            {
                m_vulkanEngine.Initialize(hinstance, renderWindow.GetHwnd().Handle);
            }

            /* Register UI controls */
            // FPS display
            Label? fpsLabel = FindName("ID_FPS_TEXT") as Label;
            if (m_fpsLabelTimer != null && fpsLabel != null)
            {
                m_fpsLabelTimer.Tick += (object? sender, EventArgs e) =>
                {
                    fpsLabel.Content = m_vulkanEngine.GetFps().ToString("F2");
                };
            }

            // Initialize and start first scene
            OnSceneChange();
        }

        private void OnSceneChange()
        {
            // Initialize UI elements
            ComboBox? polygonMode = FindName("ID_POLYGON_MODE") as ComboBox;
            if (polygonMode != null)
            {
                polygonMode.SelectedValuePath = "Key";
                polygonMode.DisplayMemberPath = "Value";
                polygonMode.Items.Add(new KeyValuePair<string, string>("PolygonModeFill", Localization.Localization.PolygonModeFill));
                polygonMode.Items.Add(new KeyValuePair<string, string>("PolygonModeLine", Localization.Localization.PolygonModeLine));
                polygonMode.Items.Add(new KeyValuePair<string, string>("PolygonModePoint", Localization.Localization.PolygonModePoint));

                _setInitialComboBoxValue(polygonMode);
                polygonMode.SelectionChanged += _onComboBoxChanged;
            }

            ComboBox? cullMode = FindName("ID_CULL_MODE") as ComboBox;
            if (cullMode != null)
            {
                cullMode.SelectedValuePath = "Key";
                cullMode.DisplayMemberPath = "Value";
                cullMode.Items.Add(new KeyValuePair<string, string>("CullModeNone", Localization.Localization.CullModeNone));
                cullMode.Items.Add(new KeyValuePair<string, string>("CullModeFront", Localization.Localization.CullModeFront));
                cullMode.Items.Add(new KeyValuePair<string, string>("CullModeBack", Localization.Localization.CullModeBack));
                cullMode.Items.Add(new KeyValuePair<string, string>("CullModeFrontAndBack", Localization.Localization.CullModeFrontAndBack));

                _setInitialComboBoxValue(cullMode);
                cullMode.SelectionChanged += _onComboBoxChanged;
            }
        }

        // Engine-specific
        private VulkanRenderEngine? m_vulkanEngine;

        private DispatcherTimer? m_fpsLabelTimer;

        private void OnClosing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            m_vulkanEngine?.Exit();
        }

        private void _setInitialComboBoxValue(ComboBox item)
        {
            item.SelectedValue = m_vulkanEngine?.GetEngineValue(item.Name);
        }

        private void _onComboBoxChanged(object? sender, SelectionChangedEventArgs e)
        {
            ComboBox? owner = sender as ComboBox;
            if (owner != null)
            {
                object? value = e.AddedItems[0];
                if (value != null)
                {
                    m_vulkanEngine?.SetEngineValue(owner.Name, ((KeyValuePair<string, string>)value).Key);
                }
            }
        }
    }
}
