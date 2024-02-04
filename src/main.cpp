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
#include "tank/globals.h"
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

int main()
{
  czh::logger::init_logger(czh::logger::Severity::NONE, czh::logger::Output::console);
  game::add_tank({0,0});
  std::thread game_thread(
      []
      {
        std::chrono::high_resolution_clock::time_point beg, end;
        std::chrono::milliseconds cost;
        while (true)
        {
          beg = std::chrono::high_resolution_clock::now();
          game::mainloop();
          renderer::render();
          end = std::chrono::high_resolution_clock::now();
          cost = std::chrono::duration_cast<std::chrono::milliseconds>(end - beg);
          if (g::tick > cost)
          {
            std::this_thread::sleep_for(g::tick - cost);
          }
        }
      }
  );
  while (true)
  {
    if (czh::term::kbhit())
    {
      input::Input i = input::get_input();
      switch (i)
      {
        case input::Input::G_UP:
          game::tank_react(0, tank::NormalTankEvent::UP);
          break;
        case input::Input::G_DOWN:
          game::tank_react(0, tank::NormalTankEvent::DOWN);
          break;
        case input::Input::G_LEFT:
          game::tank_react(0, tank::NormalTankEvent::LEFT);
          break;
        case input::Input::G_RIGHT:
          game::tank_react(0, tank::NormalTankEvent::RIGHT);
          break;
        case input::Input::G_KEY_SPACE:
          game::tank_react(0, tank::NormalTankEvent::FIRE);
          break;
        case input::Input::G_KEY_O:
          if (g::curr_page == game::Page::GAME)
          {
            g::curr_page = game::Page::TANK_STATUS;
            g::output_inited = false;
            renderer::render();
          }
          else
          {
            g::curr_page = game::Page::GAME;
            g::output_inited = false;
            renderer::render();
          }
          break;
        case input::Input::G_KEY_L:
        {
          std::lock_guard<std::mutex> l(g::mainloop_mtx);
          game::add_auto_tank(utils::randnum<int>(1, 11));
        }
          break;
        case input::Input::G_KEY_SLASH:
          g::curr_page = game::Page::COMMAND;
          g::output_inited = false;
          renderer::render();
          break;
        case input::Input::M_KEY_ENTER:
          g::curr_page = game::Page::GAME;
          g::output_inited = false;
          renderer::render();
          break;
      }
    }
  }
}

#pragma clang diagnostic pop