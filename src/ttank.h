#pragma once
#include "tmap.h"
#include <vector>
#include <algorithm>
namespace czh::tank
{
  class Tank
  {
  private:
    int blood;
    map::Pos pos;
    map::Direction direction;
    bool hascleared;
  public:
    Tank(map::Map& map, std::vector<map::Pos>& changes, map::Pos pos_) 
      : blood(100), direction(map::Direction::UP), pos(std::move(pos_)), hascleared(false)
    {
      pos.get_point(map.get_map()).add_status(map::Status::TANK);
      changes.emplace_back(pos);
    }
    void up(map::Map& map, std::vector<map::Pos>& changes)
    {
      int a = map.up(map::Status::TANK, pos);
      direction = map::Direction::UP;
      if (a == 0)
      {
        changes.emplace_back(pos);
        changes.emplace_back(map::Pos(pos.get_x(), pos.get_y() - 1));
      }
    }
    void down(map::Map& map, std::vector<map::Pos>& changes)
    {
      int a = map.down(map::Status::TANK, pos);
      direction = map::Direction::DOWN;
      if (a == 0)
      {
        changes.emplace_back(pos);
        changes.emplace_back(map::Pos(pos.get_x(), pos.get_y() + 1));
      }
    }
    void left(map::Map& map, std::vector<map::Pos>& changes)
    {
      int a = map.left(map::Status::TANK, pos);
      direction = map::Direction::LEFT;
      if (a == 0)
      {
        changes.emplace_back(pos);
        changes.emplace_back(map::Pos(pos.get_x() + 1, pos.get_y()));
      }
    }
    void right(map::Map& map, std::vector<map::Pos>& changes)
    {
      int a = map.right(map::Status::TANK, pos);
      direction = map::Direction::RIGHT;
      if (a == 0)
      {
        changes.emplace_back(pos);
        changes.emplace_back(map::Pos(pos.get_x() - 1, pos.get_y()));
      }
    }
    int get_blood() const { return blood; }
    bool is_alive() const
    {
      return blood > 0;
    }
    bool has_cleared() const
    {
      return hascleared;
    }
    void clear() { hascleared = true; }
    map::Pos& get_pos()
    {
      return pos;
    }
    void attacked(int lethality)
    {
      blood -= lethality;
      if (blood < 0) blood = 0;
    }
    const map::Pos& get_pos() const
    {
      return pos;
    }
    map::Direction get_Direction() const
    {
      return direction;
    }
    
  };
  map::AutoTankEvent get_direction(const map::Pos& from, const map::Pos& to)
  {
    int x = from.get_x() - to.get_x();
    int y = from.get_y() - to.get_y();
    if (x > 0)
      return map::AutoTankEvent::LEFT;
    else if (x < 0)
      return map::AutoTankEvent::RIGHT;
    else if (y > 0)
      return map::AutoTankEvent::DOWN;
    return map::AutoTankEvent::UP;
  }
  std::size_t get_distance(const map::Pos& from, const map::Pos& to)
  {
    return std::abs(int(from.get_x() - to.get_x())) + std::abs(int(from.get_y() - to.get_y()));
  }
  class AutoTank : public Tank
  {
  private:
    class Node
    {
    private:
      map::Pos pos;
      int G;
    public:
      Node(map::Pos pos_, int G_ = 0)
        :pos(pos_), G(G_) {}
      Node(const Node& node)
        :  pos(node.pos), G(node.G) {}
      int get_F(const map::Pos& dest) const
      {
        return G +  get_distance(dest, pos);
      }
      const map::Pos& get_pos() const
      {
        return pos;
      }
      std::vector<Node> get_neighbors(map::Map& map) const
      {
        std::vector<Node> ret;
        map::Pos pos_up(pos.get_x(), pos.get_y() + 1);
        map::Pos pos_down(pos.get_x(), pos.get_y() - 1);
        map::Pos pos_left(pos.get_x() - 1, pos.get_y());
        map::Pos pos_right(pos.get_x() + 1, pos.get_y());
        if (check(map, pos_up))
          ret.emplace_back(Node(std::move(pos_up), G + 1));
        if (check(map, pos_down))
          ret.emplace_back(Node(std::move(pos_down), G + 1));
        if (check(map, pos_left))
          ret.emplace_back(Node(std::move(pos_left), G + 1));
        if (check(map, pos_right))
          ret.emplace_back(Node(std::move(pos_right), G + 1));
        return ret;
      }
    private:
      bool check(map::Map& map, map::Pos& pos) const
      {
        return !pos.get_point(map.get_map()).has(map::Status::WALL) && !pos.get_point(map.get_map()).has(map::Status::TANK);
      }
    };
  private:
    map::Pos target_pos;
    map::Pos around_target;
    std::size_t target_id;
    std::vector<map::AutoTankEvent> way;
    std::size_t waypos;
    bool found;
    std::size_t level;
    std::size_t count;
  public:
    AutoTank(map::Map& map, std::vector<map::Pos>& changes, map::Pos pos_, std::size_t level_)
      :Tank(map, changes, pos_), found(false), waypos(0), target_id(0), level(level_), count(0) {}
    bool target(map::Map& map, const map::Pos& tpos, std::size_t id)
    {
      target_id = id;
      target_pos = tpos;
      around_target = tpos;
      switch (map::random(0, 4))
      {
      case 0:
        around_target.get_x() -= 9;
        break;
      case 1:
        around_target.get_x() += 9;
        break;
      case 2:
        around_target.get_y() -= 9;
        break;
      default:
        around_target.get_y() += 9;
        break;
      }
      std::vector<Node> open_list{ Node(get_pos()) };
      std::vector<Node> close_list;
      while (!open_list.empty())
      {
        auto it = std::min_element(open_list.begin(), open_list.end(), [this, &map](const Node& n1, const Node& n2)
          {
            return n1.get_F(around_target) < n2.get_F(around_target);
          });
        close_list.emplace_back(*it);
        open_list.erase(it);
        auto& curr = close_list[close_list.size() - 1];
        auto nodes = curr.get_neighbors(map);
        open_list.clear();
        for (auto& node : nodes)
        {
          auto it1 = std::find_if(open_list.begin(), open_list.end(), [&node](const Node& n)
            {
              return n.get_pos() == node.get_pos();
            });
          auto it2 = std::find_if(close_list.begin(), close_list.end(), [&node](const Node& n)
            {
              return n.get_pos() == node.get_pos();
            });      
          if (it1 == open_list.end() && it2 == close_list.end())
          {
            open_list.emplace_back(node);
          }
        }
        auto itt = std::find_if(open_list.begin(), open_list.end(), [this](const Node& n)
          {
            return n.get_pos() == around_target;
          });
        if (itt != open_list.end())//found
        {
          way.clear();
          waypos = 0;
          for (auto it = close_list.rbegin(); it < close_list.rend() - 1; ++it)
          {
            way.emplace_back(get_direction((it + 1)->get_pos(), it->get_pos()));
          }
          way.emplace_back(get_direction(close_list[0].get_pos(), around_target));
          way.emplace_back(get_direction(around_target, target_pos));
          found = true;
          return true;
        }
      }
      found = false;
      return false;
    }
    map::AutoTankEvent next()
    {
      bool delay = ++count < 5 - level;
      if (delay)
        return map::AutoTankEvent::NOTHING;
      count = 0;
      if (found && waypos < way.size())
      {
        auto ret = way[waypos];
        ++waypos;
        return ret;
      }
      else
      {
        return map::AutoTankEvent::STOP;
      }
    }
    map::Pos get_target_pos() const
    {
      return target_pos;
    }
    map::Pos get_around_target_pos() const
    {
      return around_target;
    }
    std::size_t get_target_id() const
    {
      return target_id;
    }
    bool get_found() const
    {
      return found;
    }
  };
}