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
#ifndef TANK_BULLET_H
#define TANK_BULLET_H

#include "info.h"
#include "game_map.h"

namespace czh::tank
{
  class Tank;
}
namespace czh::bullet
{
  class Bullet
  {
  private:
    map::Pos pos;
    map::Direction direction;
    tank::Tank* from_tank;
    info::BulletInfo info;
  public:
    Bullet(info::BulletInfo info_, tank::Tank* from_tank_,
           map::Pos pos_, map::Direction direction_)
        : pos(pos_), direction(direction_), info(info_), from_tank(from_tank_) {}
    
    int move();
  
    std::string get_text();
    
    [[nodiscard]] bool is_alive() const;
    
    [[nodiscard]] tank::Tank* get_from() const;
    
    void kill();
    
    void attacked(int lethality_);
    
    [[nodiscard]]const map::Pos &get_pos() const;
    
    map::Pos &get_pos();
    
    [[nodiscard]] int get_lethality() const;
    
    [[nodiscard]] int get_circle() const;
  };
}
#endif