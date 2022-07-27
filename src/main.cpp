#include "tgame.h"
#include <chrono>
#include <thread>
using namespace std;
using namespace czh::game;
using namespace czh::tank;
int main()
{
  Game game;
  game.add_tank();
  std::chrono::high_resolution_clock::time_point beg, end;
  std::chrono::milliseconds cost(0);
  std::chrono::milliseconds sleep(30);
  while (true)
  {
    beg = std::chrono::high_resolution_clock::now();
    if (czh::term::kbhit())
    {
      switch (czh::term::getch())
      {
        case 'w':
        case 28:
        case 72:
          game.tank_react(0, NormalTankEvent::UP);
          break;
        case 's':
        case 40:
        case 80:
          game.tank_react(0, NormalTankEvent::DOWN);
          break;
        case 'a':
        case 37:
        case 75:
          game.tank_react(0, NormalTankEvent::LEFT);
          break;
        case 'd':
        case 39:
        case 77:
          game.tank_react(0, NormalTankEvent::RIGHT);
          break;
        case ' ':
          game.tank_react(0, NormalTankEvent::FIRE);
          break;
        case 'q':
          game.react(Event::QUIT);
          return 0;
          break;
        case 27://ESC
          if (game.is_running())
            game.react(Event::PAUSE);
          else
            game.react(Event::CONTINUE);
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
          game.react(Event::NOTHING);
          break;
      }
    }
    else
      game.react(Event::NOTHING);
    end = std::chrono::high_resolution_clock::now();
    cost = std::chrono::duration_cast<std::chrono::milliseconds>(end - beg);
    if (sleep > cost)
      std::this_thread::sleep_for(sleep - cost);
  }
  return 0;
}
