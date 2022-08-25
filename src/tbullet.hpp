//   Copyright 2022 tank - caozhanhao
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
#ifndef TANK_TBULLET_HPP
#define TANK_TBULLET_HPP
#include "tmap.hpp"
#include <memory>
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
    int blood;
    int lethality;
    int circle;
    int remained_range;
    std::shared_ptr<std::vector<map::Change>> changes;
    std::shared_ptr<map::Map> map;
    std::shared_ptr<tank::Tank> from;
  public:
    Bullet(std::shared_ptr<map::Map> map_, std::shared_ptr<std::vector<map::Change>> changes_,
           std::shared_ptr<tank::Tank> from_, map::Pos pos_, map::Direction direction_, int lethality_, int circle_,
           int blood_, int range_)
        : map(std::move(map_)), changes(std::move(changes_)), from(std::move(from_)), pos(pos_), direction(direction_),
          blood(blood_), lethality(lethality_), circle(circle_), remained_range(range_)
    {
      changes->emplace_back(pos);
    }
    
    int move()
    {
      int ret = -1;
      switch (direction)
      {
        case map::Direction::UP:
          ret = map->up(map::Status::BULLET, pos);
          if (ret != 0)
          {
            blood -= 1;
            direction = map::Direction::DOWN;
          }
          else
          {
            remained_range -= 1;
            changes->emplace_back(map::Change(pos));
            changes->emplace_back(map::Change(map::Pos(pos.get_x(), pos.get_y() - 1)));
          }
          break;
        case map::Direction::DOWN:
          ret = map->down(map::Status::BULLET, pos);
          if (ret != 0)
          {
            blood -= 1;
            direction = map::Direction::UP;
          }
          else
          {
            remained_range -= 1;
            changes->emplace_back(map::Change(pos));
            changes->emplace_back(map::Change(map::Pos(pos.get_x(), pos.get_y() + 1)));
          }
          break;
        case map::Direction::LEFT:
          ret = map->left(map::Status::BULLET, pos);
          if (ret != 0)
          {
            blood -= 1;
            direction = map::Direction::RIGHT;
          }
          else
          {
            remained_range -= 1;
            changes->emplace_back(map::Change(pos));
            changes->emplace_back(map::Change(map::Pos(pos.get_x() + 1, pos.get_y())));
          }
          break;
        case map::Direction::RIGHT:
          ret = map->right(map::Status::BULLET, pos);
          if (ret != 0)
          {
            blood -= 1;
            direction = map::Direction::LEFT;
          }
          else
          {
            remained_range -= 1;
            changes->emplace_back(map::Change(pos));
            changes->emplace_back(map::Change(map::Pos(pos.get_x() - 1, pos.get_y())));
          }
          break;
      }
      return ret;
    }
    
    std::string get_text()
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
    
    [[nodiscard]] bool is_alive() const
    {
      return blood > 0 && remained_range > 0;
    }
    
    [[nodiscard]] std::shared_ptr<tank::Tank> get_from() const
    {
      return from;
    }
    
    void kill()
    {
      blood = 0;
    }
    
    void attacked(int lethality_)
    {
      blood -= lethality_;
    }
    
    [[nodiscard]]const map::Pos &get_pos() const
    {
      return pos;
    }
    
    map::Pos &get_pos()
    {
      return pos;
    }
    
    [[nodiscard]] int get_lethality() const
    {
      return lethality;
    }
    
    [[nodiscard]] int get_circle() const
    {
      return circle;
    }
  };
}
#endif