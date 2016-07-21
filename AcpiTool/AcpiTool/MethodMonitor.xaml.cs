using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

using System.ComponentModel;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Windows.Threading;
using AcpiLibrary;
using System.Collections;
using System.Text.RegularExpressions;
namespace AcpiTool
{
    /// <summary>
    /// Interaction logic for MethodMonitor.xaml
    /// </summary>
    public partial class MethodMonitor : Page
    {
        DisplayIDTControl idtControl;

        public MethodMonitor()
        {
            InitializeComponent();
            idtControl = (App.Current.MainWindow as MainWindow).idtControl;
        }

        private void MethodMonitorStart(object sender, RoutedEventArgs e)
        {
            buttonStart.IsEnabled = false;
            idtControl.InvokeIDTLibrary();
            buttonStop.IsEnabled = true;
        }
    }    
}
