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
#pragma once

#include "tank.h"
#include "message.h"
#include <string>
#include <map>
#include <vector>
#include <utility>
#include <optional>
#include <chrono>
#include <deque>
#include <list>

namespace czh::game
{
  enum class GameMode
  {
    NATIVE, SERVER, CLIENT,
  };
  
  enum class Page
  {
    GAME,
    TANK_STATUS,
    MAIN,
    HELP,
    COMMAND
  };
  
  struct UserData
  {
    size_t user_id;
    std::set<map::Pos> map_changes;
    std::deque<msg::Message> messages;
    std::chrono::steady_clock::time_point last_update;
  };
  
  std::optional<map::Pos> get_available_pos();
  
  tank::Tank *id_at(size_t id);
  
  void revive(std::size_t id);
  
  std::size_t add_auto_tank(std::size_t lvl);
  
  std::size_t add_auto_tank(std::size_t lvl, const map::Pos& pos);
  
  std::size_t add_tank(const map::Pos& pos);
  
  std::size_t add_tank();
  
  void clear_death();
  
  void mainloop();
  
  void tank_react(std::size_t id, tank::NormalTankEvent event);
}
#endif