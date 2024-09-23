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

            host.MessageHook += new HwndSourceHook(_nativeControlWndProc);
        }

        public void Initialize()
        {
            m_uiLabelTimer = new DispatcherTimer(DispatcherPriority.Input);
            m_uiLabelTimer.Interval = TimeSpan.FromSeconds(0.1);

            // Initialize desired engine
            InitializeVulkanEngine();

            m_uiLabelTimer.Start();
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
            if (m_uiLabelTimer != null)
            {
                // FPS display
                Label? fpsLabel = FindName("ID_FPS_TEXT") as Label;
                if (fpsLabel != null)
                {
                    m_uiLabelTimer.Tick += (object? sender, EventArgs e) =>
                    {
                        fpsLabel.Content = m_vulkanEngine.GetFps().ToString("F2");
                    };
                }

                // Camera display
                Label? cameraLabelX = FindName("ID_CAMERA_POS_X") as Label;
                if (cameraLabelX != null)
                {
                    m_uiLabelTimer.Tick += (object? sender, EventArgs e) =>
                    {
                        cameraLabelX.Content = m_vulkanEngine.GetEngineValue("ID_CAMERA_POS_X");
                    };
                }
                Label? cameraLabelY = FindName("ID_CAMERA_POS_Y") as Label;
                if (cameraLabelY != null)
                {
                    m_uiLabelTimer.Tick += (object? sender, EventArgs e) =>
                    {
                        cameraLabelY.Content = m_vulkanEngine.GetEngineValue("ID_CAMERA_POS_Y");
                    };
                }
                Label? cameraLabelZ = FindName("ID_CAMERA_POS_Z") as Label;
                if (cameraLabelZ != null)
                {
                    m_uiLabelTimer.Tick += (object? sender, EventArgs e) =>
                    {
                        cameraLabelZ.Content = m_vulkanEngine.GetEngineValue("ID_CAMERA_POS_Z");
                    };
                }
            }

            m_cameraController = m_vulkanEngine.GetCameraController();

            // Initialize and start first scene
            OnSceneChangeVulkan();
        }

        private void OnSceneChangeVulkan()
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

            // Initialize camera
            if (m_cameraController != null)
            {
                CheckBox? cameraFixedUp = FindName("ID_CAMERA_FIXED_UP_DIR") as CheckBox;
                if (cameraFixedUp != null)
                {
                    RoutedEventHandler _changed = (object? sender, RoutedEventArgs e) =>
                    {
                        m_cameraController.SetFixedUpDir(cameraFixedUp.IsChecked ?? false);
                    };
                    cameraFixedUp.Checked += _changed;
                    cameraFixedUp.Unchecked += _changed;
                    _changed(cameraFixedUp, null);
                }

                Slider? cameraSpeed = FindName("ID_CAMERA_SPEED") as Slider;
                if (cameraSpeed != null)
                {
                    RoutedPropertyChangedEventHandler<double> _changed = (object? sender, RoutedPropertyChangedEventArgs<double> e) =>
                    {
                        m_cameraController.SetSpeed((float)cameraSpeed.Value);
                    };
                    cameraSpeed.ValueChanged += _changed;
                    _changed(cameraSpeed, null);
                }

                Slider? cameraFOV = FindName("ID_CAMERA_FOV") as Slider;
                if (cameraFOV != null)
                {
                    RoutedPropertyChangedEventHandler<double> _changed = (object? sender, RoutedPropertyChangedEventArgs<double> e) =>
                    {
                        // Note: Technically not thread-safe but FOV is functionally read-only anyway
                        m_vulkanEngine?.SetEngineValue("ID_CAMERA_FOV", cameraFOV.Value.ToString("F2"));
                    };
                    cameraFOV.ValueChanged += _changed;
                    _changed(cameraFOV, null);
                }

                Slider? cameraSensitivityX = FindName("ID_CAMERA_SENSITIVITY_X") as Slider;
                if (cameraSensitivityX != null)
                {
                    RoutedPropertyChangedEventHandler<double> _changed = (object? sender, RoutedPropertyChangedEventArgs<double> e) =>
                    {
                        m_cameraController.SetSensitivityX((float)cameraSensitivityX.Value);
                    };
                    cameraSensitivityX.ValueChanged += _changed;
                    _changed(cameraSensitivityX, null);
                }

                Slider? cameraSensitivityY = FindName("ID_CAMERA_SENSITIVITY_Y") as Slider;
                if (cameraSensitivityY != null)
                {
                    RoutedPropertyChangedEventHandler<double> _changed = (object? sender, RoutedPropertyChangedEventArgs<double> e) =>
                    {
                        m_cameraController.SetSensitivityY((float)cameraSensitivityY.Value);
                    };
                    cameraSensitivityY.ValueChanged += _changed;
                    _changed(cameraSensitivityY, null);
                }

                // Setup camera controller keys
                RenderWindowHost? renderWindow = FindName("RenderWindow") as RenderWindowHost;
                if (renderWindow != null)
                {
                    renderWindow.KeyDown += (object? sender, KeyEventArgs e) =>
                    {
                        switch (e.Key)
                        {
                            case Key.W:
                                m_cameraController.SetMovementInputForward(true);
                                break;
                            case Key.A:
                                m_cameraController.SetMovementInputLeft(true);
                                break;
                            case Key.S:
                                m_cameraController.SetMovementInputBackward(true);
                                break;
                            case Key.D:
                                m_cameraController.SetMovementInputRight(true);
                                break;
                            case Key.E:
                                m_cameraController.SetMovementInputUp(true);
                                break;
                            case Key.Q:
                                m_cameraController.SetMovementInputDown(true);
                                break;
                        }
                    };
                    renderWindow.KeyUp += (object? sender, KeyEventArgs e) =>
                    {
                        switch (e.Key)
                        {
                            case Key.W:
                                m_cameraController.SetMovementInputForward(false);
                                break;
                            case Key.A:
                                m_cameraController.SetMovementInputLeft(false);
                                break;
                            case Key.S:
                                m_cameraController.SetMovementInputBackward(false);
                                break;
                            case Key.D:
                                m_cameraController.SetMovementInputRight(false);
                                break;
                            case Key.E:
                                m_cameraController.SetMovementInputUp(false);
                                break;
                            case Key.Q:
                                m_cameraController.SetMovementInputDown(false);
                                break;
                        }
                    };
                }
            }
        }

        // Engine-specific
        private VulkanRenderEngine? m_vulkanEngine;
        private CameraInputController? m_cameraController;

        private DispatcherTimer? m_uiLabelTimer;

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

        [DllImport("user32.dll")]
        private static extern int ShowCursor(bool bShow);

        private IntPtr _nativeControlWndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
        {
            handled = false;

            switch (msg)
            {
                // Camera controls
                case WindowMessageTypes.WM_MOUSEMOVE:
                    if ((wParam.ToInt32() & WindowMessageTypes.MK_LBUTTON) == WindowMessageTypes.MK_LBUTTON)
                    {
                        int xPos = lParam.ToInt32() & 0xFFFF;
                        int yPos = (lParam.ToInt32() >> 16) & 0xFFFF;
                        Point screenPos = PointToScreen(new Point(xPos, yPos));
                        m_cameraController?.SetMousePos((int)screenPos.X, (int)screenPos.Y);
                    }
                    break;

                case WindowMessageTypes.WM_LBUTTONDOWN:
                    if (!(m_cameraController?.IsActive() ?? false))
                    {
                        int xPos = lParam.ToInt32() & 0xFFFF;
                        int yPos = (lParam.ToInt32() >> 16) & 0xFFFF;
                        Point screenPos = PointToScreen(new Point(xPos, yPos));
                        m_cameraController?.BeginControl((int)screenPos.X, (int)screenPos.Y);
                        RenderWindowHost? renderWindow = FindName("RenderWindow") as RenderWindowHost;
                        renderWindow?.Focus();
                        ShowCursor(false);
                    }
                    break;

                case WindowMessageTypes.WM_LBUTTONUP:
                    if (m_cameraController?.IsActive() ?? false)
                    {
                        int xPos = lParam.ToInt32() & 0xFFFF;
                        int yPos = (lParam.ToInt32() >> 16) & 0xFFFF;
                        Point screenPos = PointToScreen(new Point(xPos, yPos));
                        m_cameraController?.EndControl((int)screenPos.X, (int)screenPos.Y);
                        Keyboard.ClearFocus();
                        ShowCursor(true);
                    }
                    break;
            }

            return IntPtr.Zero;
        }
    }
}
