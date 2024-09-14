using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;

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

            // Initialize renderer
            JsonRequirements vulkanRequirements = new JsonRequirements();
            vulkanRequirements.Initialize("resources/model-viewer-renderer.json");

            wnd.Show();
        }
    }
}
