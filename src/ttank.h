#pragma once
#include "tmap.h"
#include <map>
#include <algorithm>
namespace czh::tank
{
  enum class TankType
  {
    AUTO, TANK
  };
  class Tank
  {
  private:
    int blood;
    int lethality;
    map::Pos pos;
    std::size_t id;
    map::Direction direction;
    bool hascleared;
    TankType type;
  public:
    Tank(int blood_, int lethality_, map::Map& map, std::vector<map::Pos>& changes, map::Pos pos_, std::size_t id_, TankType type_ = TankType::TANK)
      : blood(blood_), lethality(lethality_), direction(map::Direction::UP), pos(std::move(pos_)),
      hascleared(false), id(id_), type(type_)
    {
      pos.get_point(map.get_map()).add_status(map::Status::TANK);
      changes.emplace_back(pos);
    }
    bool is_auto() const
    {
      return type == TankType::AUTO;
    }
    std::size_t get_id() const
    {
      return id;
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
    int get_lethality() const { return lethality; }
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
    TankType get_type() const
    {
      return type;
    }
    void revive()
    {
      blood = 100;
      hascleared = false;
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
  class Node
  {
  private:
    map::Pos pos;
    map::Pos last;
    int G;
    bool root;
  public:
    Node() : G(0), root(false) {}
    Node(map::Pos pos_, int G_, const map::Pos& last_, bool root_ = false)
      :pos(pos_), G(G_), last(last_), root(root_) {}
    Node(const Node& node)
      : pos(node.pos), G(node.G), root(node.root), last(node.last) {}
    int get_F(const map::Pos& dest) const
    {
      return G + get_distance(dest, pos) * 10;
    }
    int& get_G()
    {
      return G;
    }
    map::Pos& get_last()
    {
      return last;
    }
    const map::Pos& get_pos() const
    {
      return pos;
    }
    bool is_root() const
    {
      return root;
    }
    std::vector<Node> get_neighbors(map::Map& map) const
    {
      std::vector<Node> ret;
      map::Pos pos_up(pos.get_x(), pos.get_y() + 1);
      map::Pos pos_down(pos.get_x(), pos.get_y() - 1);
      map::Pos pos_left(pos.get_x() - 1, pos.get_y());
      map::Pos pos_right(pos.get_x() + 1, pos.get_y());
      if (check(map, pos_up))
        ret.emplace_back(Node(std::move(pos_up), G + 10, pos));
      if (check(map, pos_down))
        ret.emplace_back(Node(std::move(pos_down), G + 10, pos));
      if (check(map, pos_left))
        ret.emplace_back(Node(std::move(pos_left), G + 10, pos));
      if (check(map, pos_right))
        ret.emplace_back(Node(std::move(pos_right), G + 10, pos));
      return ret;
    }
  private:
    bool check(map::Map& map, map::Pos& pos) const
    {
      return !pos.get_point(map.get_map()).has(map::Status::WALL) && !pos.get_point(map.get_map()).has(map::Status::TANK);
    }
  };
  bool operator<(const Node& n1, const Node& n2)
  {
    return n1.get_pos() < n2.get_pos();
  }
  class AutoTank : public Tank
  {
  private:
    map::Pos around_target;
    std::size_t target_id;
    TankType target_type;
    map::Pos target_pos;
    std::vector<map::AutoTankEvent> way;
    std::size_t waypos;
    bool found;
    std::size_t level;
    std::size_t count;
  public:
    AutoTank(int blood_, int lethality_, map::Map& map, std::vector<map::Pos>& changes, map::Pos pos_, std::size_t level_, std::size_t id_)
      :Tank(blood_, lethality_, map, changes, pos_, id_, TankType::AUTO), found(false), waypos(0),
      target_id(0), target_type(TankType::AUTO), level(level_), count(0) {}
    void target(map::Map& map, TankType target_type_, std::size_t target_id_, map::Pos target_pos_)
    {
      target_type = target_type_;
      target_id = target_id_;
      target_pos = target_pos_;
      around_target = target_pos;
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
      std::multimap<int, Node> open_list;
      std::map<map::Pos, Node> close_list;
      Node node(get_pos(), 0, { 0,0 }, true);
      open_list.insert({ node.get_F(around_target), node });
      while (!open_list.empty())
      {
        auto it = open_list.begin();
        auto curr = close_list.insert({ it->second.get_pos(), it->second });
        open_list.erase(it);
        auto nodes = curr.first->second.get_neighbors(map);
        //open_list.clear();
        for (auto& node : nodes)
        {
          auto cit = close_list.find(node.get_pos());
          auto oit = std::find_if(open_list.begin(), open_list.end(),
            [&node](const std::pair<int, Node>& p)
            {
              return p.second.get_pos() == node.get_pos();
            });
          if (cit == close_list.end())
          {
            if (oit == open_list.end())
            {
              open_list.insert({ node.get_F(around_target), node });
            }
            else
            {
              if (oit->second.get_G() > node.get_G() + 10) //less G
              {
                oit->second.get_G() = node.get_G() + 10;
                oit->second.get_last() = node.get_pos();
                int F = oit->second.get_F(around_target);
                auto n = open_list.extract(oit);
                n.key() = F;
                open_list.insert(std::move(n));
              }
            }
          }
        }
        auto itt = std::find_if(open_list.begin(), open_list.end(),
          [this](const std::pair<int, Node>& p)
          {
            return p.second.get_pos() == around_target;
          });
        if (itt != open_list.end())//found
        {
          way.clear();
          waypos = 0;
          auto& np = itt->second;
          while (!np.is_root())
          {
            way.emplace_back(get_direction(close_list[np.get_last()].get_pos(), np.get_pos()));
            np = close_list[np.get_last()];
          }
          way.emplace_back(get_direction(around_target, target_pos));
          found = true;
          return;
        }
      }
      return;
    }
    map::AutoTankEvent next()
    {
      bool delay = ++count < 10 - level;
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
    std::size_t get_target_id() const
    {
      return target_id;
    }   
    map::Pos get_around_target_pos() const
    {
      return around_target;
    }

    std::size_t target_is_auto() const
    {
      return target_type == TankType::AUTO;
    }
    bool get_found() const
    {
      return found;
    }
    int get_level() const
    {
      return level;
    }
  };
}