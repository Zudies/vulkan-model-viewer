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

            //TODO: Does this need to go elsewhere for UI control?
            wnd.InitializeVulkanEngine();
        }
    }
}
