#include "tgame.h"
#include <chrono>
#include <thread>
using namespace std;
using namespace czh::game;

int main()
{
  Game game;
  game.add_tank();
  std::chrono::high_resolution_clock::time_point beg, end;
  std::chrono::milliseconds cost(0);
  std::chrono::milliseconds sleep(17);
  while (game.is_running())
  {
    beg = std::chrono::high_resolution_clock::now();
    if (czh::term::kbhit())
    {
      switch (czh::term::getch())
      {
        case 'w':
          game.react(0, Event::TANK_UP);
          break;
        case 's':
          game.react(0, Event::TANK_DOWN);
          break;
        case 'a':
          game.react(0, Event::TANK_LEFT);
          break;
        case 'd':
          game.react(0, Event::TANK_RIGHT);
          break;
        case 'f':
          game.react(0, Event::TANK_FIRE);
          break;
        case 'q':
          game.react(0, Event::QUIT);
          break;
        case 'l':
          game.add_auto_tank(1, ::czh::map::random(1, 11));
          break;
        case 'b':
          game.add_auto_boss();
          break;
        case 'p':
          game.revive(0);
          break;
        default:
          game.react(0, Event::NOTHING);
          break;
      }
    } else
      game.react(0, Event::NOTHING);
    end = std::chrono::high_resolution_clock::now();
    cost = std::chrono::duration_cast<std::chrono::milliseconds>(end - beg);
    if (sleep > cost)
      std::this_thread::sleep_for(sleep - cost);
  }
  return 0;
}
