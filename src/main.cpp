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
#include "tank/globals.h"
#include "tank/command.h"
#include "tank/game.h"
#include "tank/drawing.h"
#include "tank/input.h"
#include "tank/utils.h"
#include "tank/tank.h"
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include <csignal>

using namespace czh;

void react(tank::NormalTankEvent event)
{
  if (g::snapshot.tanks[g::user_id].is_alive)
  {
    if (g::game_mode == game::GameMode::CLIENT)
    {
      g::online_client.tank_react(event);
    }
    else
    {
      game::tank_react(g::user_id, event);
    }
  }
}

#ifdef SIGCONT
void sighandler(int)
{
  g::keyboard.init();
  g::output_inited = false;
  g::game_suspend = false;
}
#endif

int main()
{
#ifdef SIGCONT
  signal(SIGCONT, sighandler);
#endif
  std::thread game_thread(
      []
      {
        std::chrono::steady_clock::time_point beg, end;
        std::chrono::milliseconds cost;
        while (true)
        {
          beg = std::chrono::steady_clock::now();
          if (g::game_mode == game::GameMode::NATIVE)
          {
            game::mainloop();
          }
          else if (g::game_mode == game::GameMode::SERVER)
          {
            game::mainloop();
            std::vector<size_t> disconnected;
            for (auto &r: g::userdata)
            {
              if (r.first == 0) continue;
              auto d = std::chrono::duration_cast<std::chrono::seconds>
                  (std::chrono::steady_clock::now() - r.second.last_update);
              if (d.count() > 5)
              {
                disconnected.emplace_back(r.first);
              }
            }
            for (auto &r: disconnected)
            {
              msg::info(-1, g::userdata[r].ip + " (" + std::to_string(r) + ") disconnected.");
              g::tanks[r]->kill();
              g::tanks[r]->clear();
              delete g::tanks[r];
              g::tanks.erase(r);
              g::userdata.erase(r);
            }
          }
          else
          {
            if (g::client_failed_attempts > 10)
            {
              g::online_client.disconnect();
              g::game_mode = game::GameMode::NATIVE;
              g::userdata[0].messages = g::userdata[g::user_id].messages;
              g::user_id = 0;
              g::tank_focus = g::user_id;
              g::output_inited = false;
              g::client_failed_attempts = 0;
              msg::critical(g::user_id, "Disconnected due to network issues.");
            }
          }
          auto frame = drawing::update_snapshot();
          if (frame == 0) drawing::draw();
          
          end = std::chrono::steady_clock::now();
          cost = std::chrono::duration_cast<std::chrono::milliseconds>(end - beg);
          if (g::tick > cost)
          {
            std::this_thread::sleep_for(g::tick - cost);
          }
        }
      }
  );
  game::add_tank({0, 0});
  while (true)
  {
    input::Input i = input::get_input();
    if (g::curr_page == game::Page::GAME)
    {
      switch (i)
      {
        case input::Input::UP:
          react(tank::NormalTankEvent::UP);
          break;
        case input::Input::DOWN:
          react(tank::NormalTankEvent::DOWN);
          break;
        case input::Input::LEFT:
          react(tank::NormalTankEvent::LEFT);
          break;
        case input::Input::RIGHT:
          react(tank::NormalTankEvent::RIGHT);
          break;
        case input::Input::KEY_SPACE:
          react(tank::NormalTankEvent::FIRE);
          break;
        case input::Input::KEY_O:
          g::curr_page = game::Page::TANK_STATUS;
          g::output_inited = false;
          break;
        case input::Input::KEY_L:
        {
          if (g::game_mode == game::GameMode::CLIENT)
          {
            g::online_client.add_auto_tank(utils::randnum<int>(1, 11));
          }
          else
          {
            std::lock_guard<std::mutex> l(g::mainloop_mtx);
            game::add_auto_tank(utils::randnum<int>(1, 11));
          }
        }
          break;
      }
    }
    else if (g::curr_page == game::Page::HELP)
    {
      switch (i)
      {
        case input::Input::UP:
          if (g::help_lineno != 1)
          {
            g::help_lineno--;
            g::output_inited = false;
          }
          break;
        case input::Input::DOWN:
          if (g::help_lineno < g::help_text.size())
          {
            g::help_lineno++;
            g::output_inited = false;
          }
          break;
      }
    }
    else if (g::curr_page == game::Page::TANK_STATUS)
    {
      switch (i)
      {
        case input::Input::UP:
          if (g::status_lineno != 1)
          {
            g::status_lineno--;
          }
          break;
        case input::Input::DOWN:
          if (g::status_lineno < g::snapshot.tanks.size())
          {
            g::status_lineno++;
          }
          break;
        case input::Input::KEY_O:
          g::curr_page = game::Page::GAME;
          g::output_inited = false;
          break;
      }
    }
    switch (i)
    {
      case input::Input::KEY_SLASH:
        g::typing_command = true;
        g::cmd_line.clear();
        g::cmd_pos = 0;
        g::history.emplace_back("");
        g::history_pos = g::history.size() - 1;
        input::edit_refresh_line();
        if (auto c = input::get_input(); c == input::Input::COMMAND)
        {
          cmd::run_command(g::user_id, g::cmd_line);
        }
        g::typing_command = false;
        break;
      case input::Input::KEY_ENTER:
        g::curr_page = game::Page::GAME;
        g::output_inited = false;
        break;
      case input::Input::KEY_CTRL_C:
      {
        std::lock_guard<std::mutex> l(g::mainloop_mtx);
        game::quit();
        std::exit(0);
      }
        break;
#ifdef SIGCONT
        case input::Input::KEY_CTRL_Z:
        {
          std::lock_guard<std::mutex> l1(g::drawing_mtx);
          std::lock_guard<std::mutex> l2(g::mainloop_mtx);
          if (g::game_mode == game::GameMode::CLIENT)
          {
            g::online_client.disconnect();
            g::user_id = 0;
            g::tank_focus = g::user_id;
            g::output_inited = false;
            g::game_mode = game::GameMode::NATIVE;
          }
          else if (g::game_mode == game::GameMode::SERVER)
          {
            g::online_server.stop();
            for (auto &r: g::userdata)
            {
              if (r.first == 0) continue;
              g::tanks[r.first]->kill();
              g::tanks[r.first]->clear();
              delete g::tanks[r.first];
              g::tanks.erase(r.first);
            }
            g::userdata = {{0, g::userdata[0]}};
            g::game_mode = game::GameMode::NATIVE;
          }
          g::game_suspend = true;
          g::keyboard.deinit();
          raise(SIGSTOP);
        }
          break;
#endif
      default:
        break;
    }
  }
}
