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
using System.Timers;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Windows.Threading;

using AcpiLibrary;
namespace AcpiTool
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public DisplayIDTControl idtControl = new DisplayIDTControl();
        public MainWindow()
        {
            InitializeComponent();
            MessageBox.Show("initial");
            idtControl.InvokeIDTLibrary();
        }
        private void Window_Closed_1(object sender, EventArgs e)
        {
            //MessageBox.Show(methodList2.Count.ToString());
            idtControl.Dispose();
        }
    }

}
