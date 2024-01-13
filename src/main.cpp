//   Copyright 2022-2024 tank - caozhanhao
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
#include "tank/term.h"
#include "tank/game.h"
#include "tank/renderer.h"
#include "tank/input.h"
#include "tank/utils.h"
#include "tank/tank.h"
#include "tank/logger.h"
#include <chrono>
#include <thread>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"


using namespace czh;
using namespace czh::game;
using namespace czh::input;
using namespace czh::renderer;

int main()
{
  czh::logger::init_logger(czh::logger::Severity::NONE, czh::logger::Output::console);
  add_tank();
  std::thread game_thread(
      []{
        std::chrono::high_resolution_clock::time_point beg, end;
        std::chrono::milliseconds cost;
        while(true)
        {
          beg = std::chrono::high_resolution_clock::now();
          mainloop();
          render();
          end = std::chrono::high_resolution_clock::now();
          cost = std::chrono::duration_cast<std::chrono::milliseconds>(end - beg);
          if (tick > cost)
            std::this_thread::sleep_for(tick - cost);
        }
      }
      );
  game_thread.detach();
  while (true)
  {
    if (czh::term::kbhit())
    {
      Input i = get_input();
      switch (i)
      {
        case Input::G_UP:
          tank_react(0, tank::NormalTankEvent::UP);
          break;
        case Input::G_DOWN:
          tank_react(0, tank::NormalTankEvent::DOWN);
          break;
        case Input::G_LEFT:
          tank_react(0, tank::NormalTankEvent::LEFT);
          break;
        case Input::G_RIGHT:
          tank_react(0, tank::NormalTankEvent::RIGHT);
          break;
        case Input::G_KEY_SPACE:
          tank_react(0, tank::NormalTankEvent::FIRE);
          break;
        case Input::G_KEY_O:
          if (curr_page == Page::GAME)
          {
            curr_page = Page::TANK_STATUS;
            output_inited = false;
            render();
          }
          else
          {
            curr_page = Page::GAME;
            output_inited = false;
            render();
          }
          break;
        case Input::G_KEY_L:
          add_auto_tank(utils::randnum<int>(1, 11));
          break;
        case Input::G_KEY_SLASH:
          curr_page = Page::COMMAND;
          output_inited = false;
          render();
          break;
        case Input::M_KEY_ENTER:
          curr_page = Page::GAME;
          output_inited = false;
          render();
          break;
      }
    }
  }
}
#pragma clang diagnostic pop