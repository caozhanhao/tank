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
#ifndef TANK_GAME_H
#define TANK_GAME_H

#include "internal/info.h"
#include "internal/tank.h"
#include "internal/term.h"
#include "internal/game_map.h"
#include "internal/bullet.h"
#include "internal/logger.h"
#include <vector>
#include <optional>
#include <algorithm>
#include <functional>
#include <string>
#include <memory>
#include <map>

namespace czh::game
{
  enum class Event
  {
    PASS,
    START,
    PAUSE,
    CONTINUE,
    COMMAND
  };
  
  enum class Page
  {
    GAME,
    TANK_STATUS,
    MAIN,
    HELP,
    COMMAND
  };
  std::pair<size_t, size_t> get_map_size(size_t w, size_t h);
  class Game
  {
  private:
    bool output_inited;
    std::size_t screen_height;
    std::size_t screen_width;
    std::shared_ptr<map::Map> map;
    std::vector<std::shared_ptr<tank::Tank>> tanks;
    std::shared_ptr<std::vector<bullet::Bullet>> bullets;
    std::vector<std::pair<std::size_t, tank::NormalTankEvent>> normal_tank_events;
    Page curr_page;
    size_t help_page;
    size_t next_id;
    std::map<std::size_t, std::size_t> id_index;
    std::vector<std::string> history;
    std::string cmd_string;
    size_t history_pos;
    size_t cmd_string_pos;
  public:
    Game();
             
    std::size_t add_tank();
  
    Game &revive(std::size_t id);
  
    std::size_t add_auto_tank(std::size_t lvl = 1);
  
    Game &tank_react(std::size_t tankpos, tank::NormalTankEvent event);
  
    Game &react(Event event);
  
    [[nodiscard]]Page get_page() const;
  
    void run_command(const std::string &str);

    void receive_char(char c);
  private:
    void clear_death();
  
    [[nodiscard]]std::vector<std::size_t> get_alive(std::size_t except) const;
  
    auto find_tank(std::size_t i, std::size_t j);
  
    auto find_tank_nocheck(std::size_t i, std::size_t j);
  
    std::optional<map::Pos> get_available_pos();
  
    void update(const map::Pos &pos);
  
    void paint();
  
    std::vector<bullet::Bullet>::iterator find_bullet(std::size_t i, std::size_t j);
  
    void for_all_bullets(std::size_t i, std::size_t j,
                         const std::function<void(std::vector<bullet::Bullet>::iterator &)> &func);
  
    std::shared_ptr<tank::Tank> id_at(size_t id);
    void reshape(std::size_t w, std::size_t h);
  };
}
#endif