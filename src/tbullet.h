#pragma once
#include "tmap.h"
namespace czh::bullet
{
  class Bullet
  {
  private:
    map::Pos pos;
    map::Direction direction;
    std::size_t id;
    bool from_auto_tank;
    int blood;
    int lethality;
    int circle;
    int remained_range;
  public:
    Bullet(bool from_auto_tank_, std::size_t id_, map::Pos pos_, map::Direction direction_, int lethality_, int circle_, int blood_, int range_)
      :from_auto_tank(from_auto_tank_), pos(std::move(pos_)), direction(direction_), id(id_),
      blood(blood_), lethality(lethality_), circle(circle_), remained_range(range_) {}
    void move(map::Map& map, std::vector<map::Pos>& changes)
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
          changes.emplace_back(pos);
          changes.emplace_back(map::Pos(pos.get_x(), pos.get_y() - 1));
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
          changes.emplace_back(pos);
          changes.emplace_back(map::Pos(pos.get_x(), pos.get_y() + 1));
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
          changes.emplace_back(pos);
          changes.emplace_back(map::Pos(pos.get_x() + 1, pos.get_y()));
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
          changes.emplace_back(pos);
          changes.emplace_back(map::Pos(pos.get_x() - 1, pos.get_y()));
        }
        break;
      }
    }
    std::string get_text()
    {
      switch (direction)
      {
      case map::Direction::UP:
      case map::Direction::DOWN:
        return "|";
      }
      return "-";
    }
    bool is_alive() const 
    { 
      return blood > 0 && remained_range > 0; 
    }
    bool is_from_auto_tank() const
    {
      return from_auto_tank;
    }
    void kill()
    {
      blood = 0;
    }
    void attacked(int lethality_)
    {
      blood -= lethality_;
    }
    const map::Pos& get_pos() const
    {
      return pos;
    }
    std::size_t get_id() const
    {
      return id;
    }
    map::Pos& get_pos() 
    {
      return pos;
    }
    int get_lethality() const
    {
      return lethality;
    }
    int get_circle() const
    {
      return circle;
    }
  };
}