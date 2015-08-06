using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace TestApplication
{
    class Program
    {
        [DllImport("plugin_quividi.dll", CharSet = CharSet.Ansi)]
        private static extern void Start([MarshalAs(UnmanagedType.LPStr)]string mode, [MarshalAs(UnmanagedType.LPStr)]string hostName, int hostPort);

        [DllImport("plugin_quividi.dll")]
        private static extern void Stop();

        static void Main(string[] args)
        {
            Start("periodic", "127.0.0.1", 1974);

            Console.WriteLine("\nPress ESC to stop");
            do
            {
                while (!Console.KeyAvailable)
                {
                    Thread.Sleep(1000);
                }
            } while (Console.ReadKey(true).Key != ConsoleKey.Escape);

            Stop();
        }
    }
}
