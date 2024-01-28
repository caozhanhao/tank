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
#include "tank/game_map.h"
#include <vector>
#include <list>
#include <set>
#include <cassert>
#include <stdexcept>
#include <variant>


czh::map::Point czh::map::empty_point{};
namespace czh::map
{
  bool Point::is_active() const
  {
    return data.index() == 0;
  }
  
  tank::Tank* Point::get_tank_instance() const
  {
    assert(is_active());
    return std::get<ActivePointData>(data).tank;
  }
  
  const std::vector<bullet::Bullet*>& Point::get_bullets_instance() const
  {
    assert(is_active());
    return std::get<ActivePointData>(data).bullets;
  }
  
  const TankData& Point::get_tank_data() const
  {
    assert(!is_active());
    return std::get<InactivePointData>(data).tank;
  }
  
  const std::vector<BulletData>& Point::get_bullets_data() const
  {
    assert(!is_active());
    return std::get<InactivePointData>(data).bullets;
  }
  
  
  void Point::activate(const ActivePointData& d)
  {
    data.emplace<ActivePointData>(d);
  }
  
  void Point::deactivate(const InactivePointData& d)
  {
    data.emplace<InactivePointData>(d);
  }
  
  void Point::add_status(const Status &status, void* ptr)
  {
    assert(is_active());
    statuses.emplace_back(status);
    if(ptr != nullptr)
    switch (status)
    {
      case Status::BULLET:
        std::get<ActivePointData>(data).bullets.emplace_back(static_cast<bullet::Bullet*>(ptr));
        break;
      case Status::TANK:
        std::get<ActivePointData>(data).tank = static_cast<tank::Tank*>(ptr);
        break;
    }
  }
  
  void Point::remove_status(const Status &status)
  {
    assert(is_active());
    statuses.erase(std::remove(statuses.begin(), statuses.end(), status), statuses.end());
    switch (status)
    {
      case Status::BULLET:
        std::get<ActivePointData>(data).bullets.clear();
        break;
      case Status::TANK:
        std::get<ActivePointData>(data).tank = nullptr;
        break;
    }
  }
  
  void Point::remove_all_statuses()
  {
    assert(is_active());
    statuses.clear();
    std::get<ActivePointData>(data).bullets.clear();
    std::get<ActivePointData>(data).tank = nullptr;
  }
  
  [[nodiscard]] bool Point::has(const Status &status) const
  {
    assert(is_active());
    return (std::find(statuses.cbegin(), statuses.cend(), status) != statuses.cend());
  }
  
  [[nodiscard]]std::size_t Point::count(const Status &status) const
  {
    assert(is_active());
    return std::count(statuses.cbegin(), statuses.cend(), status);
  }
  
  bool Pos::operator==(const Pos &pos) const
  {
    return (x == pos.x && y == pos.y);
  }
  
  bool Pos::operator!=(const Pos &pos) const
  {
    return !(*this == pos);
  }
  
  bool operator<(const Pos &pos1, const Pos &pos2)
  {
    if (pos1.x == pos2.x)
    {
      return pos1.y < pos2.y;
    }
    return pos1.x < pos2.x;
  }
  
  
  bool operator<(const Change &c1, const Change &c2)
  {
    return c1.get_pos() < c2.get_pos();
  }
  
  std::size_t get_distance(const map::Pos &from, const map::Pos &to)
  {
    return std::abs(int(from.x - to.x)) + std::abs(int(from.y - to.y));
  }
  
  
  const Pos &Change::get_pos() const { return pos; }
  
  Pos &Change::get_pos() { return pos; }
  
  Map::Map() {}
  
  int Map::up(const Status &status, const Pos &pos)
  {
    return move(status, pos, 0);
  }
  
  int Map::down(const Status &status, const Pos &pos)
  {
    return move(status, pos, 1);
  }
  
  int Map::left(const Status &status, const Pos &pos)
  {
    return move(status, pos, 2);
  }
  
  int Map::right(const Status &status, const Pos &pos)
  {
    return move(status, pos, 3);
  }
  
  int Map::add_tank(tank::Tank* t, const Pos &pos)
  {
    map[pos].add_status(Status::TANK, t);
    changes.insert(Change{pos});
    return 0;
  }
  
  int Map::add_bullet(bullet::Bullet* b, const Pos &pos)
  {
    auto &p = map[pos];
    if (p.has(Status::WALL)) return -1;
    p.add_status(Status::BULLET, b);
    changes.insert(Change{pos});
    return 0;
  }
  
  void Map::remove_status(const Status &status, const Pos &pos)
  {
    map[pos].remove_status(status);
    changes.insert(Change{pos});
  }
  
  bool Map::has(const Status &status, const Pos &pos) const
  {
    return at(pos).has(status);
  }
  
  int Map::count(const Status &status, const Pos &pos) const
  {
    return at(pos).count(status);
  }
  
  const std::set<Change> &Map::get_changes() const { return changes; };
  
  void Map::clear_changes() { changes.clear(); };
  
  const Point &Map::at(int x, int y) const
  {
    return at(Pos(x, y));
  }
  
  const Point &Map::at(const Pos &i) const
  {
    if(map.find(i) != map.end())
      return map.at(i);
    return empty_point;
  }
  
  int Map::fill(const Pos& from, const Pos& to, const Status& status)
  {
    int bx = std::max(from.x, to.x);
    int sx = std::min(from.x, to.x);
    int by = std::max(from.y, to.y);
    int sy = std::min(from.y, to.y);
  
    for (int i = sx; i <= bx; ++i)
    {
      for (int j = sy; j <= by; ++j)
      {
        Pos p(i, j);
        map[p].remove_all_statuses();
        if(status != Status::END)
          map[p].add_status(status, nullptr);
        changes.insert(Change{p});
      }
    }
    return 0;
  }
  
  int Map::move(const Status &status, const Pos &pos, int direction)
  {
    Pos new_pos = pos;
    switch (direction)
    {
      case 0:
        new_pos.y++;
        break;
      case 1:
        new_pos.y--;
        break;
      case 2:
        new_pos.x--;
        break;
      case 3:
        new_pos.x++;
        break;
    }
    auto &new_point = map[new_pos];
    auto &old_point = map[pos];
    assert(new_point.is_active() && old_point.is_active());
    switch (status)
    {
      case Status::BULLET:
        if (new_point.has(Status::WALL)) return -1;
        new_point.add_status(Status::BULLET, nullptr);
        std::get<ActivePointData>(new_point.data).bullets = std::get<ActivePointData>(old_point.data).bullets;
        old_point.remove_status(Status::BULLET);
        break;
      case Status::TANK:
        if (new_point.has(Status::WALL) || new_point.has(Status::TANK)) return -1;
        new_point.add_status(Status::TANK, std::get<ActivePointData>(old_point.data).tank);
        old_point.remove_status(Status::TANK);
        break;
      default:
        throw std::runtime_error("Unreachable");
        break;
    }
    changes.insert(Change{pos});
    changes.insert(Change{new_pos});
    return 0;
  }
}
