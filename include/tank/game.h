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
#ifndef TANK_GAME_H
#define TANK_GAME_H

#include "tank.h"
#include <string>
#include <map>
#include <chrono>
#include <vector>
#include <utility>
#include <optional>
#include <list>
#include <mutex>

namespace czh::game
{
  enum class Page
  {
    GAME,
    TANK_STATUS,
    MAIN,
    HELP,
    COMMAND
  };
  
  struct Zone // [X min, X max)   [Y min, Y max)
  {
    int x_min;
    int x_max;
    int y_min;
    int y_max;
  };


  extern int keyboard_mode;
  extern std::chrono::milliseconds tick;
  extern bool output_inited;
  extern bool map_size_changed;
  extern size_t tank_focus;
  extern Zone rendered_zone;
  extern std::mutex render_mtx;
  extern std::mutex mainloop_mtx;
  extern std::size_t screen_height;
  extern std::size_t screen_width;
  extern map::Map game_map;
  extern std::map<std::size_t, tank::Tank*> tanks;
  extern std::list<bullet::Bullet*> bullets;
  extern std::vector<std::pair<std::size_t, tank::NormalTankEvent>> normal_tank_events;
  extern Page curr_page;
  extern size_t help_page;
  extern size_t next_id;
  extern std::vector<std::string> history;
  extern std::string cmd_string;
  extern size_t history_pos;
  extern size_t cmd_string_pos;
  
  std::optional<map::Pos> get_available_pos();
  void tank_assert(bool a, const std::string &err = "Assertion Failed.");
  tank::Tank* id_at(size_t id);
  void revive(std::size_t id);
  std::size_t add_auto_tank(std::size_t lvl);
  std::size_t add_tank();
  void clear_death();
  void mainloop();
  void tank_react(std::size_t id, tank::NormalTankEvent event);
  
  void load_point(const map::Pos& pos);
  void load_point(int x, int y);
  void load_zone(const Zone& zone);
  void unload_point(const map::Pos& pos);
  void unload_point(int x, int y);
  void unload_zone(const Zone& zone);
}
#endif