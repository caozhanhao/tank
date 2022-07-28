#pragma once
#include "tmap.h"
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
    tank::Tank *from;
    int blood;
    int lethality;
    int circle;
    int remained_range;
  public:
    Bullet(tank::Tank *from_, std::vector<map::Change> &changes, map::Pos pos_,
           map::Direction direction_, int lethality_, int circle_, int blood_, int range_)
        : from(from_), pos(pos_), direction(direction_),
          blood(blood_), lethality(lethality_), circle(circle_), remained_range(range_)
    {
      changes.emplace_back(pos);
    }
  
    int move(map::Map &map, std::vector<map::Change> &changes)
    {
      int ret = -1;
      switch (direction)
      {
        case map::Direction::UP:
          ret = map.up(map::Status::BULLET, pos);
          if (ret != 0)
          {
            blood -= 1;
            direction = map::Direction::DOWN;
          }
          else
          {
            remained_range -= 1;
            changes.emplace_back(map::Change(pos));
            changes.emplace_back(map::Change(map::Pos(pos.get_x(), pos.get_y() - 1)));
          }
          break;
        case map::Direction::DOWN:
          ret = map.down(map::Status::BULLET, pos);
          if (ret != 0)
          {
            blood -= 1;
            direction = map::Direction::UP;
          }
          else
          {
            remained_range -= 1;
            changes.emplace_back(map::Change(pos));
            changes.emplace_back(map::Change(map::Pos(pos.get_x(), pos.get_y() + 1)));
          }
          break;
        case map::Direction::LEFT:
          ret = map.left(map::Status::BULLET, pos);
          if (ret != 0)
          {
            blood -= 1;
            direction = map::Direction::RIGHT;
          }
          else
          {
            remained_range -= 1;
            changes.emplace_back(map::Change(pos));
            changes.emplace_back(map::Change(map::Pos(pos.get_x() + 1, pos.get_y())));
          }
          break;
        case map::Direction::RIGHT:
          ret = map.right(map::Status::BULLET, pos);
          if (ret != 0)
          {
            blood -= 1;
            direction = map::Direction::LEFT;
          }
          else
          {
            remained_range -= 1;
            changes.emplace_back(map::Change(pos));
            changes.emplace_back(map::Change(map::Pos(pos.get_x() - 1, pos.get_y())));
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
          break;
        default:
          break;
      }
      return "-";
    }
    
    [[nodiscard]] bool is_alive() const
    {
      return blood > 0 && remained_range > 0;
    }
  
    [[nodiscard]] tank::Tank *get_from() const
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