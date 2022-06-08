#include <iostream>
#include "app.hpp" 

#define WINDOW_WINDTH 1000
#define WINDOW_HEIGHT 500

int main()
{
  std::cout << "Start SPH simulation \n";

  app sphSim(WINDOW_WINDTH, WINDOW_HEIGHT);

  sphSim.execute();
}