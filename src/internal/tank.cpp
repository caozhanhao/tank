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
#include "internal/tank.h"
#include "internal/game_map.h"
#include "internal/bullet.h"
#include "internal/utils.h"
#include <map>
#include <set>
#include <list>
#include <algorithm>
#include <functional>
#include <memory>

namespace czh::tank
{
  Tank::Tank(info::TankInfo info_, std::shared_ptr<map::Map> map_,
             std::shared_ptr<std::vector<bullet::Bullet>> bullets_,
             map::Pos pos_)
      : info(info_), map(std::move(map_)), bullets(std::move(bullets_)),
        direction(map::Direction::UP), pos(pos_), hp(info_.max_hp),
        hascleared(false)
  {
    map->add_tank(pos);
  }
  
  void Tank::kill()
  {
    attacked(hp);
  }
  
  int Tank::up()
  {
    direction = map::Direction::UP;
    int ret = map->up(map::Status::TANK, pos);
    if (ret == 0)
      pos.get_y()++;
    return ret;
  }
  
  int Tank::down()
  {
    direction = map::Direction::DOWN;
    int ret = map->down(map::Status::TANK, pos);
    if (ret == 0)
      pos.get_y()--;
    return ret;
  }
  
  int Tank::left()
  {
    direction = map::Direction::LEFT;
    int ret = map->left(map::Status::TANK, pos);
    if (ret == 0)
      pos.get_x()--;
    return ret;
  }
  
  int Tank::right()
  {
    direction = map::Direction::RIGHT;
    int ret = map->right(map::Status::TANK, pos);
    if (ret == 0)
      pos.get_x()++;
    return ret;
  }
  
  int Tank::fire()
  {
    int ret = map->add_bullet(get_pos());
    if (ret == 0)
    {
      bullets->emplace_back(
          bullet::Bullet(info.bullet(), map, shared_from_this(), get_pos(), get_direction()));
    }
    return ret;
  }
  
  [[nodiscard]]bool Tank::is_auto() const
  {
    return info.type == info::TankType::AUTO;
  }
  
  [[nodiscard]]std::size_t Tank::get_id() const
  {
    return info.id;
  }
  
  std::string &Tank::get_name()
  {
    return info.name;
  }
  
  const std::string &Tank::get_name() const
  {
    return info.name;
  }
  
  [[nodiscard]]int Tank::get_hp() const { return hp; }
  
  [[nodiscard]]int &Tank::get_hp() { return hp; }
  
  [[nodiscard]]int Tank::get_max_hp() const { return info.max_hp; }
  
  [[nodiscard]]const info::TankInfo &Tank::get_info() const { return info; }
  
  [[nodiscard]]info::TankInfo &Tank::get_info() { return info; }
  
  [[nodiscard]]bool Tank::is_alive() const
  {
    return hp > 0;
  }
  
  [[nodiscard]]bool Tank::has_cleared() const
  {
    return hascleared;
  }
  
  void Tank::clear() { hascleared = true; }
  
  map::Pos &Tank::get_pos()
  {
    return pos;
  }
  
  void Tank::attacked(int lethality_)
  {
    hp -= lethality_;
    if (hp < 0) hp = 0;
    if (hp > info.max_hp) hp = info.max_hp;
  }
  
  [[nodiscard]]const map::Pos &Tank::get_pos() const
  {
    return pos;
  }
  
  [[nodiscard]]map::Direction &Tank::get_direction()
  {
    return direction;
  }
  
  [[nodiscard]]const map::Direction &Tank::get_direction() const
  {
    return direction;
  }
  
  [[nodiscard]]info::TankType Tank::get_type() const
  {
    return info.type;
  }
  
  void Tank::revive(const map::Pos &newpos)
  {
    if (is_alive() && !hascleared) return;
    hp = info.max_hp;
    hascleared = false;
    pos = newpos;
    map->add_tank(pos);
  }
  
  std::string Tank::colorify_text(const std::string &str)
  {
    std::string ret = "\033[0;";
    ret += std::to_string(info.id % 6 + 32);
    ret += "m";
    ret += str;
    ret += "\033[0m";
    return ret;
  }
  
  std::string Tank::colorify_tank()
  {
    std::string ret = "\033[0;";
    ret += std::to_string(info.id % 6 + 42);
    ret += ";36m";
    ret += "  \033[0m";
    return ret;
  }
  
  int Tank::tanks_nearby() const
  {
    int ret = 0;
    map::Pos pos_up(pos.get_x(), pos.get_y() + 1);
    map::Pos pos_down(pos.get_x(), pos.get_y() - 1);
    map::Pos pos_left(pos.get_x() - 1, pos.get_y());
    map::Pos pos_right(pos.get_x() + 1, pos.get_y());
    if(map->check_pos(pos_up) && map->has(map::Status::TANK, pos_up)) ret += 1;
    if(map->check_pos(pos_down) && map->has(map::Status::TANK, pos_down)) ret += 1;
    if(map->check_pos(pos_left) && map->has(map::Status::TANK, pos_left)) ret += 1;
    if(map->check_pos(pos_right) && map->has(map::Status::TANK, pos_right)) ret += 1;
    return ret;
  }
  
  AutoTankEvent get_pos_direction(const map::Pos &from, const map::Pos &to)
  {
    int x = (int) from.get_x() - (int) to.get_x();
    int y = (int) from.get_y() - (int) to.get_y();
    if (x > 0)
    {
      return AutoTankEvent::LEFT;
    }
    else if (x < 0)
    {
      return AutoTankEvent::RIGHT;
    }
    else if (y > 0)
    {
      return AutoTankEvent::DOWN;
    }
    return AutoTankEvent::UP;
  }
  
  [[nodiscard]]int Node::get_F(const map::Pos &dest) const
  {
    return G + (int) map::get_distance(dest, pos) * 10;
  }
  
  int &Node::get_G()
  {
    return G;
  }
  
  map::Pos &Node::get_last()
  {
    return last;
  }
  
  [[nodiscard]]const map::Pos &Node::get_pos() const
  {
    return pos;
  }
  
  [[nodiscard]]bool Node::is_root() const
  {
    return root;
  }
  
  std::vector<Node> Node::get_neighbors(const std::shared_ptr<map::Map> &map) const
  {
    std::vector<Node> ret;
    map::Pos pos_up(pos.get_x(), pos.get_y() + 1);
    map::Pos pos_down(pos.get_x(), pos.get_y() - 1);
    map::Pos pos_left(pos.get_x() - 1, pos.get_y());
    map::Pos pos_right(pos.get_x() + 1, pos.get_y());
    if (check(map, pos_up))
    {
      ret.emplace_back(Node(pos_up, G + 10, pos));
    }
    if (check(map, pos_down))
    {
      ret.emplace_back(Node(pos_down, G + 10, pos));
    }
    if (check(map, pos_left))
    {
      ret.emplace_back(Node(pos_left, G + 10, pos));
    }
    if (check(map, pos_right))
    {
      ret.emplace_back(Node(pos_right, G + 10, pos));
    }
    return ret;
  }
  
  bool Node::check(const std::shared_ptr<map::Map> &map, map::Pos &pos)
  {
    return map->check_pos(pos)
           && !map->has(map::Status::WALL, pos) && !map->has(map::Status::TANK, pos);
  }
  
  bool operator<(const Node &n1, const Node &n2)
  {
    return n1.get_pos() < n2.get_pos();
  }
  
  bool is_in_firing_line(const std::shared_ptr<map::Map> &map, const map::Pos &pos, const map::Pos &target_pos)
  {
    int x = (int) target_pos.get_x() - (int) pos.get_x();
    int y = (int) target_pos.get_y() - (int) pos.get_y();
    if (x == 0 && std::abs(y) > 0)
    {
      std::size_t small = y > 0 ? pos.get_y() : target_pos.get_y();
      std::size_t big = y < 0 ? pos.get_y() : target_pos.get_y();
      for (std::size_t i = small + 1; i < big; ++i)
      {
        map::Pos tmp = {pos.get_x(), i};
        if (map->has(map::Status::WALL, tmp)
            || map->has(map::Status::TANK, tmp))
        {
          return false;
        }
      }
    }
    else if (y == 0 && std::abs(x) > 0)
    {
      std::size_t small = x > 0 ? pos.get_x() : target_pos.get_x();
      std::size_t big = x < 0 ? pos.get_x() : target_pos.get_x();
      for (std::size_t i = small + 1; i < big; ++i)
      {
        map::Pos tmp = {i, pos.get_y()};
        if (map->has(map::Status::WALL, tmp)
            || map->has(map::Status::TANK, tmp))
        {
          return false;
        }
      }
    }
    else
    {
      return false;
    }
    return true;
  }
  
  void AutoTank::target(std::size_t target_id_, const map::Pos &target_pos_)
  {
    found = false;
    target_id = target_id_;
    target_pos = target_pos_;
    std::multimap<int, Node> open_list;
    std::map<map::Pos, Node> close_list;
    // fire_line
    std::set<map::Pos> fire_line;
    for (int i = 0; i <= target_pos.get_x(); ++i)
    {
      map::Pos tmp(i, target_pos.get_y());
      if (is_in_firing_line(map, tmp, target_pos))
      {
        fire_line.insert(tmp);
      }
    }
    for (int i = 0; i <= target_pos.get_y(); ++i)
    {
      map::Pos tmp(target_pos.get_x(), i);
      if (is_in_firing_line(map, tmp, target_pos))
      {
        fire_line.insert(tmp);
      }
    }
  
    destination_pos = *std::min_element(fire_line.begin(), fire_line.end(),
                                  [this](auto&& a, auto&& b)
                                  {
                                    return map::get_distance(a, pos) < map::get_distance(b, pos);
                                  });
    
    
    Node beg(get_pos(), 0, {0, 0}, true);
    open_list.insert({beg.get_F(destination_pos), beg});
    while (!open_list.empty())
    {
      auto it = open_list.begin();
      auto curr = close_list.insert({it->second.get_pos(), it->second});
      open_list.erase(it);
      auto neighbors = curr.first->second.get_neighbors(map);
      for (auto &node: neighbors)
      {
        auto cit = close_list.find(node.get_pos());
        if (cit != close_list.end()) continue;
        auto oit = std::find_if(open_list.begin(), open_list.end(),
                                [&node](auto &&p)
                                {
                                  return p.second.get_pos() == node.get_pos();
                                });
        if (oit == open_list.end())
        {
          open_list.insert({node.get_F(destination_pos), node});
        }
        else
        {
          if (oit->second.get_G() > node.get_G() + 10) //less G
          {
            oit->second.get_G() = node.get_G() + 10;
            oit->second.get_last() = node.get_pos();
            int F = oit->second.get_F(destination_pos);
            auto n = open_list.extract(oit);
            n.key() = F;
            open_list.insert(std::move(n));
          }
        }
      }
      auto itt = std::find_if(open_list.begin(), open_list.end(),
                              [&fire_line](auto &&p) -> bool
                              {
                                return fire_line.find(p.second.get_pos()) != fire_line.end();
                              });
      if (itt != open_list.end())//found
      {
        way.clear();
        waypos = 0;
        auto &np = itt->second;
        while (!np.is_root() && np.get_pos() != np.get_last())
        {
          way.insert(way.begin(), get_pos_direction(close_list[np.get_last()].get_pos(), np.get_pos()));
          np = close_list[np.get_last()];
        }
        found = true;
        return;
      }
    }
  }
  
  bool AutoTank::is_in_retreat() const
  {
    return in_retreat;
  }
  
  void AutoTank::attacked(int lethality_)
  {
    Tank::attacked(lethality_);
    in_retreat = true;
    way.clear();
    waypos = 0;
    auto check = [this](map::Pos p)
    {
      return map->check_pos(p) && !map->has(map::Status::WALL, p) && !map->has(map::Status::TANK, p)
                                                                     && !map->has(map::Status::BULLET, p);
    };
    auto p = pos;
    int i = 0;
    while (way.size() < 10 && i++ < 10)
    {
      map::Pos pos_up(p.get_x(), p.get_y() + 1);
      map::Pos pos_down(p.get_x(), p.get_y() - 1);
      map::Pos pos_left(p.get_x() - 1, p.get_y());
      map::Pos pos_right(p.get_x() + 1, p.get_y());
      switch (utils::randnum<int>(0, 4))
      {
        case 0:
          if(check(pos_up))
          {
            p = pos_up;
            way.insert(way.end(), 5, AutoTankEvent::UP);
          }
          break;
        case 1:
          if(check(pos_down))
          {
            p = pos_down;
            way.insert(way.end(), 5, AutoTankEvent::DOWN);
          }
          break;
        case 2:
          if(check(pos_left))
          {
            p = pos_left;
            way.insert(way.end(), 5, AutoTankEvent::LEFT);
          }
          break;
        case 3:
          if(check(pos_right))
          {
            p = pos_right;
            way.insert(way.end(), 5, AutoTankEvent::RIGHT);
          }
          break;
      }
    }
  }
  
  AutoTankEvent AutoTank::next()
  {
    if (!found)
    {
      return AutoTankEvent::PASS;
    }
    if (++gap_count < info.gap)
      return AutoTankEvent::PASS;
    else
      gap_count = 0;
  
    if (waypos < way.size())
    {
      auto ret = way[waypos];
      ++waypos;
      return ret;
    }
    else if(in_retreat) in_retreat = false;
    
    gap_count = info.gap - 5;
    return AutoTankEvent::FIRE;
  }
  
  void AutoTank::clear_way()
  {
    waypos = 0;
    way.clear();
  }
  
  
  void AutoTank::correct_direction(const map::Pos& target)
  {
    int x = (int) get_pos().get_x() - (int) target.get_x();
    int y = (int) get_pos().get_y() - (int) target.get_y();
    if (x > 0)
    {
      get_direction() = map::Direction::LEFT;
    }
    else if (x < 0)
    {
      get_direction() = map::Direction::RIGHT;
    }
    else if (y < 0)
    {
      get_direction() = map::Direction::UP;
    }
    else if (y > 0)
    {
      get_direction() = map::Direction::DOWN;
    }
  }
  
  std::size_t &AutoTank::get_target_id()
  {
    return target_id;
  }
  
  [[nodiscard]]bool AutoTank::get_found() const
  {
    return found;
  }
  
  bool AutoTank::has_arrived()
  {
    return waypos == way.size();
  }
  
  void AutoTank::stuck()
  {
    if(waypos != 0) --waypos;
    way.insert(way.begin() + waypos, AutoTankEvent::FIRE);
  }
}