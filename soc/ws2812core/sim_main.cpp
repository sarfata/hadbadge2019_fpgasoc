#include <iostream>

#include "Vws2812.h"
#include "verilated.h"

using namespace std;

int main(int argc, char **argv, char **env)
{
  Verilated::commandArgs(argc, argv);
  Vws2812 *ws2812 = new Vws2812;
  while (!Verilated::gotFinish())
  {
    if (ws2812->clk)
      cout << "led num is " << ws2812->led_num << endl;
    ws2812->clk ^= 1;
    ws2812->eval();
  }
  delete ws2812;
  exit(0);
}
