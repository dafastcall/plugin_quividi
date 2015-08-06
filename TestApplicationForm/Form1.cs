using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace TestApplicationForm
{
    public partial class Form1 : Form
    {
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool AllocConsole();

        [DllImport("plugin_quividi.dll", CharSet = CharSet.Ansi)]
        private static extern void Start([MarshalAs(UnmanagedType.LPStr)]string mode, [MarshalAs(UnmanagedType.LPStr)]string hostName, int hostPort);

        [DllImport("plugin_quividi.dll")]
        private static extern void Stop();

        [DllImport("plugin_quividi.dll")]
        private static extern int GetWatchersCount();

        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            AllocConsole();
        }

        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {
            Stop();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            int count = GetWatchersCount();
            textBox1.Text = count.ToString();
        }

        private void button2_Click(object sender, EventArgs e)
        {
            Start("periodic", "127.0.0.1", 1974);
        }

        private void button3_Click(object sender, EventArgs e)
        {
            Stop();
        }
    }
}
