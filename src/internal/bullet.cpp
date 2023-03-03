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
#include "internal/bullet.h"
#include "internal/info.h"
#include <memory>

namespace czh::bullet
{
  int Bullet::move()
  {
    int ret = -1;
    switch (direction)
    {
      case map::Direction::UP:
        ret = map->up(map::Status::BULLET, pos);
        if (ret != 0)
        {
          info.blood -= 1;
          direction = map::Direction::DOWN;
        }
        else
        {
          info.range -= 1;
          pos.get_y()++;
        }
        break;
      case map::Direction::DOWN:
        ret = map->down(map::Status::BULLET, pos);
        if (ret != 0)
        {
          info.blood -= 1;
          direction = map::Direction::UP;
        }
        else
        {
          info.range -= 1;
          pos.get_y()--;
        }
        break;
      case map::Direction::LEFT:
        ret = map->left(map::Status::BULLET, pos);
        if (ret != 0)
        {
          info.blood -= 1;
          direction = map::Direction::RIGHT;
        }
        else
        {
          info.range -= 1;
          pos.get_x()--;
        }
        break;
      case map::Direction::RIGHT:
        ret = map->right(map::Status::BULLET, pos);
        if (ret != 0)
        {
          info.blood -= 1;
          direction = map::Direction::LEFT;
        }
        else
        {
          info.range -= 1;
          pos.get_x()++;
        }
        break;
    }
    return ret;
  }
  
  std::string Bullet::get_text()
  {
    switch (direction)
    {
      case map::Direction::UP:
      case map::Direction::DOWN:
        return "|";
      default:
        break;
    }
    return "-";
  }
  
  [[nodiscard]] bool Bullet::is_alive() const
  {
    return info.blood > 0 && info.range > 0;
  }
  
  [[nodiscard]] std::shared_ptr<tank::Tank> Bullet::get_from() const
  {
    return from_tank;
  }
  
  void Bullet::kill()
  {
    info.blood = 0;
  }
  
  void Bullet::attacked(int lethality_)
  {
    info.blood -= lethality_;
  }
  
  [[nodiscard]]const map::Pos &Bullet::get_pos() const
  {
    return pos;
  }
  
  map::Pos &Bullet::get_pos()
  {
    return pos;
  }
  
  [[nodiscard]] int Bullet::get_lethality() const
  {
    return info.lethality;
  }
}
