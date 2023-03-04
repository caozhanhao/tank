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

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
using namespace czh::game;
using namespace czh::tank;
#if defined (__linux__)
#include <signal.h>
static void signal_handle(int sig)
{
  std::exit(-1);
}
#endif

int main()
{
#if defined (__linux__)
  signal(SIGINT, signal_handle);
#endif
  Game game;
  game.add_tank();
  std::chrono::high_resolution_clock::time_point beg, end;
  std::chrono::milliseconds cost(0);
  std::chrono::milliseconds sleep(20);
  char ch;
#if defined (__linux__)
  bool in_linux_like = true;
#else
  bool in_linux_like = false;
#endif
  while (true)
  {
    beg = std::chrono::high_resolution_clock::now();
    if (czh::term::kbhit())
    {
      ch = czh::term::getch();
      if (ch == -32)
      {
        ch = czh::term::getch();
        in_linux_like = false;
      }
      if (ch == 27)
      {
        czh::term::getch();
        ch = czh::term::getch();
        in_linux_like = true;
      }
      if (game.get_page() == Page::COMMAND)
      {
        // command
        // [1] up [2] down [3] run [4] left [5] right [6] backspace [7] delete [8] home [9] end
        if(in_linux_like && ch == 'A') ch = 1;
        if(in_linux_like && ch == 'B') ch = 2;
        if(in_linux_like && ch == 10) ch = 3;
        if(in_linux_like && ch == 'D') ch = 4;
        if(in_linux_like && ch == 'C') ch = 5;
        if(in_linux_like && ch == 127) ch = 6;
        if(in_linux_like && ch == 126) ch = 7;
        if(in_linux_like && ch == 72) ch = 8;
        if(in_linux_like && ch == 70) ch = 9;
        
        if(!in_linux_like && ch == 72) ch = 1;
        if(!in_linux_like && ch == 80) ch = 2;
        if(!in_linux_like && ch == 13) ch = 3;
        if(!in_linux_like && ch == 75) ch = 4;
        if(!in_linux_like && ch == 77) ch = 5;
        if(!in_linux_like && ch == 8) ch = 6;
        if(!in_linux_like && ch == 'S') ch = 7;
        if(!in_linux_like && ch == 'G') ch = 8;
        if(!in_linux_like && ch == 'O') ch = 9;
        game.receive_char(ch);
      }
      else
      {
        switch (ch)
        {
          case 'W':
          case 'w':
            game.tank_react(0, NormalTankEvent::UP);
            break;
          case 72:
            if (in_linux_like)
            {
              CZH_NOTICE("Ignored key 72");
              break;
            }
            game.tank_react(0, NormalTankEvent::UP);
            break;
          case 'S':
          case 's':
            game.tank_react(0, NormalTankEvent::DOWN);
            break;
          case 80:
            if (in_linux_like)
            {
              CZH_NOTICE("Ignored key 80");
              break;
            }
            game.tank_react(0, NormalTankEvent::DOWN);
            break;
          case 'A':
            if (in_linux_like)
              game.tank_react(0, NormalTankEvent::UP);
            else
              game.tank_react(0, NormalTankEvent::LEFT);
            break;
          case 'a':
            game.tank_react(0, NormalTankEvent::LEFT);
            break;
          case 75:
            if (in_linux_like)
            {
              CZH_NOTICE("Ignored key 75");
              break;
            }
            game.tank_react(0, NormalTankEvent::LEFT);
            break;
          case 'D':
            if (in_linux_like)
              game.tank_react(0, NormalTankEvent::LEFT);
            else
              game.tank_react(0, NormalTankEvent::RIGHT);
            break;
          case 'd':
            game.tank_react(0, NormalTankEvent::RIGHT);
            break;
          case 77:
            if (in_linux_like)
            {
              CZH_NOTICE("Ignored key 77");
              break;
            }
            game.tank_react(0, NormalTankEvent::RIGHT);
            break;
          case 'B':
            if (!in_linux_like)
            {
              CZH_NOTICE("Ignored key 76");
              break;
            }
            game.tank_react(0, NormalTankEvent::DOWN);
            break;
          case 'C':
            if (!in_linux_like)
            {
              CZH_NOTICE("Ignored key 77");
              break;
            }
            game.tank_react(0, NormalTankEvent::RIGHT);
            break;
          case ' ':
            game.tank_react(0, NormalTankEvent::FIRE);
            break;
          case 'O':
          case 'o':
            if (game.get_page() == czh::game::Page::GAME)
            {
              game.react(Event::PAUSE);
            }
            else
            {
              game.react(Event::CONTINUE);
            }
            break;
          case 13://Enter
            if (in_linux_like)
            {
              CZH_NOTICE("Ignored key 13");
              break;
            }
            game.react(Event::START);
            break;
          case 10:
            if (!in_linux_like)
            {
              CZH_NOTICE("Ignored key 10");
              break;
            }
            game.react(Event::START);
            break;
          case 'l':
            game.add_auto_tank(::czh::map::random(1, 11));
            break;
          case '/':
            game.react(Event::COMMAND);
            break;
          default:
            CZH_NOTICE("Ignored key " + std::to_string(static_cast<int>(ch)) + ".");
            break;
        }
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

#pragma clang diagnostic pop