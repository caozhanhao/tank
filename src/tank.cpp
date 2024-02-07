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
#include "tank/game.h"
#include "tank/tank.h"
#include "tank/game_map.h"
#include "tank/globals.h"
#include "tank/bullet.h"
#include "tank/utils.h"
#include <map>
#include <set>
#include <list>
#include <functional>
#include <variant>

namespace czh::tank
{
  Tank::Tank(info::TankInfo info_, map::Pos pos_)
      : info(info_), direction(map::Direction::UP),
        pos(pos_), hp(info_.max_hp), hascleared(false)
  {
    g::game_map.add_tank(this, pos);
  }
  
  void Tank::kill()
  {
    attacked(hp);
  }
  
  int Tank::up()
  {
    direction = map::Direction::UP;
    int ret = g::game_map.tank_up(pos);
    if (ret == 0)
    {
      pos.y++;
    }
    return ret;
  }
  
  int Tank::down()
  {
    direction = map::Direction::DOWN;
    int ret = g::game_map.tank_down(pos);
    if (ret == 0)
    {
      pos.y--;
    }
    return ret;
  }
  
  int Tank::left()
  {
    direction = map::Direction::LEFT;
    int ret = g::game_map.tank_left(pos);
    if (ret == 0)
    {
      pos.x--;
    }
    return ret;
  }
  
  int Tank::right()
  {
    direction = map::Direction::RIGHT;
    int ret = g::game_map.tank_right(pos);
    if (ret == 0)
    {
      pos.x++;
    }
    return ret;
  }
  
  int Tank::fire()
  {
    g::bullets.emplace_back(
        new bullet::Bullet(info.bullet, info.id, get_pos(), get_direction()));
    int ret = g::game_map.add_bullet(g::bullets.back(), get_pos());
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
  
  void Tank::clear()
  {
    g::game_map.remove_status(map::Status::TANK, get_pos());
    hascleared = true;
  }
  
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
    g::game_map.add_tank(this, pos);
  }
  
  AutoTankEvent get_pos_direction(const map::Pos &from, const map::Pos &to)
  {
    int x = (int) from.x - (int) to.x;
    int y = (int) from.y - (int) to.y;
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
  
  std::vector<Node> Node::get_neighbors() const
  {
    if (G + 10 > 100) return {};
    std::vector<Node> ret;
    
    map::Pos pos_up(pos.x, pos.y + 1);
    map::Pos pos_down(pos.x, pos.y - 1);
    map::Pos pos_left(pos.x - 1, pos.y);
    map::Pos pos_right(pos.x + 1, pos.y);
    if (check(pos_up))
    {
      ret.emplace_back(Node(pos_up, G + 10, pos));
    }
    if (check(pos_down))
    {
      ret.emplace_back(Node(pos_down, G + 10, pos));
    }
    if (check(pos_left))
    {
      ret.emplace_back(Node(pos_left, G + 10, pos));
    }
    if (check(pos_right))
    {
      ret.emplace_back(Node(pos_right, G + 10, pos));
    }
    return ret;
  }
  
  bool Node::check(map::Pos &pos) const
  {
    return !g::game_map.has(map::Status::WALL, pos) && !g::game_map.has(map::Status::TANK, pos);
  }
  
  bool operator<(const Node &n1, const Node &n2)
  {
    return n1.get_pos() < n2.get_pos();
  }
  
  bool is_in_firing_line(int range, const map::Pos &pos, const map::Pos &target_pos)
  {
    int x = target_pos.x - pos.x;
    int y = target_pos.y - pos.y;
    if (x == 0 && std::abs(y) > 0 && std::abs(y) < range)
    {
      int a = y > 0 ? pos.y : target_pos.y;
      int b = y < 0 ? pos.y : target_pos.y;
      for (int i = a + 1; i < b; ++i)
      {
        map::Pos tmp = {pos.x, i};
        if (g::game_map.has(map::Status::WALL, tmp)
            || g::game_map.has(map::Status::TANK, tmp))
        {
          return false;
        }
      }
    }
    else if (y == 0 && std::abs(x) > 0 && std::abs(x) < range)
    {
      int a = x > 0 ? pos.x : target_pos.x;
      int b = x < 0 ? pos.x : target_pos.x;
      for (int i = a + 1; i < b; ++i)
      {
        map::Pos tmp = {i, pos.y};
        if (g::game_map.has(map::Status::WALL, tmp)
            || g::game_map.has(map::Status::TANK, tmp))
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
    if (map::get_distance(target_pos_, pos) > 30)
    {
      return;
    }
    target_id = target_id_;
    target_pos = target_pos_;
    std::multimap<int, Node> open_list;
    std::map<map::Pos, Node> close_list;
    // fire_line
    std::set<map::Pos> fire_line;
    for (int i = 0; i <= target_pos.x; ++i)
    {
      map::Pos tmp(i, target_pos.y);
      if (is_in_firing_line(info.bullet.range, tmp, target_pos))
      {
        fire_line.insert(tmp);
      }
    }
    for (int i = 0; i <= target_pos.y; ++i)
    {
      map::Pos tmp(target_pos.x, i);
      if (is_in_firing_line(info.bullet.range, tmp, target_pos))
      {
        fire_line.insert(tmp);
      }
    }
    
    if (fire_line.empty()) return;
    destination_pos = *std::min_element(fire_line.begin(), fire_line.end(),
                                        [this](auto &&a, auto &&b)
                                        {
                                          return map::get_distance(a, pos) < map::get_distance(b, pos);
                                        });
    
    
    Node beg(get_pos(), 0, {0, 0}, true);
    open_list.insert({beg.get_F(destination_pos), beg});
    int i = 0;
    while (!open_list.empty())
    {
      auto it = open_list.begin();
      auto curr = close_list.insert({it->second.get_pos(), it->second});
      open_list.erase(it);
      auto neighbors = curr.first->second.get_neighbors();
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
        return;
      }
    }
    return;
  }
  
  void AutoTank::generate_random_way()
  {
    way.clear();
    waypos = 0;
    auto check = [this](map::Pos p)
    {
      return !g::game_map.has(map::Status::WALL, p) && !g::game_map.has(map::Status::TANK, p);
    };
    auto p = pos;
    int i = 0;
    while (way.size() < 10 && i++ < 10)
    {
      map::Pos pos_up(p.x, p.y + 1);
      map::Pos pos_down(p.x, p.y - 1);
      map::Pos pos_left(p.x - 1, p.y);
      map::Pos pos_right(p.x + 1, p.y);
      switch (utils::randnum<int>(0, 4))
      {
        case 0:
          if (check(pos_up))
          {
            p = pos_up;
            way.insert(way.end(), 5, AutoTankEvent::UP);
          }
          break;
        case 1:
          if (check(pos_down))
          {
            p = pos_down;
            way.insert(way.end(), 5, AutoTankEvent::DOWN);
          }
          break;
        case 2:
          if (check(pos_left))
          {
            p = pos_left;
            way.insert(way.end(), 5, AutoTankEvent::LEFT);
          }
          break;
        case 3:
          if (check(pos_right))
          {
            p = pos_right;
            way.insert(way.end(), 5, AutoTankEvent::RIGHT);
          }
          break;
      }
    }
  }
  
  void AutoTank::attacked(int lethality_)
  {
    Tank::attacked(lethality_);
    generate_random_way();
  }
  
  void AutoTank::react()
  {
    if (++gap_count < info.gap) return;
    
    gap_count = 0;
    
    if (auto tp = game::id_at(target_id);
        tp != nullptr && tank::is_in_firing_line(info.bullet.range, pos, tp->get_pos()))
    {
      gap_count = info.gap - 5;
      waypos = 0;
      way.clear();
      correct_direction(tp->get_pos());
      fire();
    }
    else
    {
      if (waypos >= way.size())
      {
        generate_random_way();
      }
      auto w = way[waypos];
      ++waypos;
      switch (w)
      {
        case tank::AutoTankEvent::UP:
          up();
          break;
        case tank::AutoTankEvent::DOWN:
          down();
          break;
        case tank::AutoTankEvent::LEFT:
          left();
          break;
        case tank::AutoTankEvent::RIGHT:
          right();
          break;
        default:
          break;
      }
    }
  }
  
  [[nodiscard]]bool AutoTank::has_arrived() const
  {
    return waypos == way.size();
  }
  
  void AutoTank::correct_direction(const map::Pos &target)
  {
    int x = (int) get_pos().x - (int) target.x;
    int y = (int) get_pos().y - (int) target.y;
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
  
  Tank *build_tank(const map::TankData &data)
  {
    if (data.is_auto())
    {
      auto ret = new AutoTank(data.info, data.pos);
      ret->hp = data.hp;
      ret->direction = data.direction;
      ret->hascleared = data.hascleared;
      
      auto &d = std::get<map::AutoTankData>(data.data);
      ret->target_id = d.target_id;
      ret->target_pos = d.target_pos;
      ret->destination_pos = d.destination_pos;
      
      ret->way = d.way;
      ret->waypos = d.waypos;
      
      ret->gap_count = d.gap_count;
      return ret;
    }
    else
    {
      auto ret = new NormalTank(data.info, data.pos);
      ret->hp = data.hp;
      ret->direction = data.direction;
      ret->hascleared = data.hascleared;
      //auto& d = std::get<map::NormalTankData>(data.data);
      return ret;
    }
    return nullptr;
  }
  
  map::TankData get_tank_data(Tank *t)
  {
    map::TankData ret;
    ret.info = t->info;
    ret.pos = t->pos;
    ret.hp = t->hp;
    ret.direction = t->direction;
    ret.hascleared = t->hascleared;
    if (t->is_auto())
    {
      auto tank = dynamic_cast<tank::AutoTank *>(t);
      map::AutoTankData data;
      
      data.target_id = tank->target_id;
      data.target_pos = tank->target_pos;
      data.destination_pos = tank->destination_pos;
      
      data.way = tank->way;
      data.waypos = tank->waypos;
      
      data.gap_count = tank->gap_count;
      ret.data.emplace<map::AutoTankData>(data);
    }
    else
    {
      // auto tank = dynamic_cast<tank::NormalTank *>(t);
      map::NormalTankData data;
      ret.data.emplace<map::NormalTankData>(data);
    }
    return ret;
  }
  
}