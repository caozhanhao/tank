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
#pragma once

#include "info.h"
#include "game_map.h"

namespace czh::bullet
{
  class Bullet;
  
  Bullet *build_bullet(const map::BulletData &data);
  
  map::BulletData get_bullet_data(Bullet *);
  
  
  class Bullet
  {
    friend Bullet *build_bullet(const map::BulletData &data);
    
    friend map::BulletData get_bullet_data(Bullet *);
  
  private:
    map::Pos pos;
    map::Direction direction;
    int from_tank_id;
    info::BulletInfo info;
  public:
    Bullet(info::BulletInfo info_, int from_tank_id_,
           map::Pos pos_, map::Direction direction_)
        : pos(pos_), direction(direction_), info(info_), from_tank_id(from_tank_id_) {}
    
    int react();
    
    std::string get_text();
    
    [[nodiscard]] bool is_alive() const;
    
    [[nodiscard]] int get_tank() const;
    
    void kill();
    
    [[nodiscard]]const map::Pos &get_pos() const;
    
    map::Pos &get_pos();
    
    [[nodiscard]] int get_lethality() const;
  };
}
#endif