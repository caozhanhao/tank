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
    PAUSE,
    CONTINUE,
    COMMAND
  };
  
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
    bool running;
    size_t next_id;
    std::map<std::size_t, std::size_t> id_index;
    
    bool as_server;
    size_t curr_changes_apply;
    size_t clients;
  public:
    Game() : output_inited(false), running(true), as_server(false),
             screen_height(term::get_height()), screen_width(term::get_width()),
             map(std::make_shared<map::Map>((screen_height - 1) % 2 == 0 ? screen_height - 2 : screen_height - 1,
                                            screen_width % 2 == 0 ? screen_width - 1 : screen_width)),
             bullets(std::make_shared<std::vector<bullet::Bullet>>()),
             curr_changes_apply(0), clients(0), next_id(0) {}
  
    void enable_server();
  
    std::vector<map::Change> get_changes();
  
    void changes_applied();
  
  
    std::size_t add_tank();
  
    Game &revive(std::size_t id);
  
    std::size_t add_auto_tank(std::size_t level = 1);
  
    Game &tank_react(std::size_t tankpos, tank::NormalTankEvent event);
  
    Game &react(Event event);
  
    [[nodiscard]]bool is_running() const;
  
    void run_command(const std::string &str);

  private:
    void clear_death();
  
    [[nodiscard]]std::vector<std::size_t> get_alive(std::size_t except) const;
  
    auto find_tank(std::size_t i, std::size_t j);
  
    auto find_tank_nocheck(std::size_t i, std::size_t j);
  
    map::Pos get_random_pos();
  
    void update(const map::Pos &pos);
  
    void paint();
  
    std::vector<bullet::Bullet>::iterator find_bullet(std::size_t i, std::size_t j);
  
    void for_all_bullets(std::size_t i, std::size_t j,
                         const std::function<void(std::vector<bullet::Bullet>::iterator &)> &func);
  
    std::shared_ptr<tank::Tank> id_at(size_t id);
  };
}
#endif