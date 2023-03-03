//   Copyright 2022-2023 tank - caozhanhao
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
#include "internal/term.h"
#include "game.h"
#include <chrono>
#include <thread>

using namespace czh::game;
using namespace czh::tank;

int main()
{
  Game game;
  game.add_tank();
  std::chrono::high_resolution_clock::time_point beg, end;
  std::chrono::milliseconds cost(0);
  std::chrono::milliseconds sleep(20);
  char ch;
  while (true)
  {
    beg = std::chrono::high_resolution_clock::now();
    if (czh::term::kbhit())
    {
      switch (ch = czh::term::getch())
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
        case 27://ESC
          if (game.is_running())
          {
            game.react(Event::PAUSE);
          }
          else
          {
            game.react(Event::CONTINUE);
          }
          break;
        case 'l':
          game.add_auto_tank(::czh::map::random(1, 11));
          break;
        case '/':
          game.react(Event::COMMAND);
          break;
        default:
          CZH_NOTICE("ignored key '" + std::string(1, ch) + "'.");
          break;
      }
    }
    game.react(Event::PASS);
    end = std::chrono::high_resolution_clock::now();
    cost = std::chrono::duration_cast<std::chrono::milliseconds>(end - beg);
    if (sleep > cost)
    {
      std::this_thread::sleep_for(sleep - cost);
    }
  }
  return 0;
}
