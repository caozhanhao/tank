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
#include "internal/game_map.h"
#include <vector>
#include <list>
#include <set>
#include <random>
#include <cassert>

namespace czh::map
{
  int random(int a, int b)
  {
    std::random_device r;
    std::default_random_engine e(r());
    std::uniform_int_distribution<int> u(a, b - 1);
    return u(e);
  }
  
  void Point::add_status(const Status &status)
  {
    statuses.emplace_back(status);
  }
  
  void Point::remove_status(const Status &status)
  {
    statuses.erase(std::remove(statuses.begin(), statuses.end(), status), statuses.end());
  }
  
  [[nodiscard]] bool Point::has(const Status &status) const
  {
    return (std::find(statuses.cbegin(), statuses.cend(), status) != statuses.cend());
  }
  
  void Point::attacked(int lethality_)
  {
    lethality += lethality_;
  }
  
  [[nodiscard]]int Point::get_lethality() const
  {
    return lethality;
  }
  
  void Point::remove_lethality()
  {
    lethality = 0;
  }
  
  [[nodiscard]]std::size_t Point::count(const Status &status) const
  {
    return std::count(statuses.cbegin(), statuses.cend(), status);
  }
  
  std::size_t &Pos::get_x()
  {
    return x;
  }
  
  std::size_t &Pos::get_y()
  {
    return y;
  }
  
  [[nodiscard]]const std::size_t &Pos::get_x() const
  {
    return x;
  }
  
  [[nodiscard]]const std::size_t &Pos::get_y() const
  {
    return y;
  }
  
  bool Pos::operator==(const Pos &pos) const
  {
    return (x == pos.get_x() && y == pos.get_y());
  }
  
  bool Pos::operator!=(const Pos &pos) const
  {
    return !(*this == pos);
  }
  
  bool operator<(const Pos &pos1, const Pos &pos2)
  {
    if (pos1.get_x() == pos2.get_x())
    {
      return pos1.get_y() < pos2.get_y();
    }
    return pos1.get_x() < pos2.get_x();
  }
  
  std::size_t get_distance(const map::Pos &from, const map::Pos &to)
  {
    return std::abs(int(from.get_x() - to.get_x())) + std::abs(int(from.get_y() - to.get_y()));
  }
  
  
  const Pos &Change::get_pos() const { return pos; }
  
  Pos &Change::get_pos() { return pos; }
  
  Map::Map(std::size_t height_, std::size_t width_)
      : height(height_), width(width_), map(width)
  {
    for (auto &r: map)
    {
      r.resize(height);
    }
    make_maze();
  }
  
  [[nodiscard]]size_t Map::get_width() const { return width; }
  
  [[nodiscard]] size_t Map::get_height() const { return height; }
  
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
  
  [[nodiscard]]bool Map::check_pos(const Pos &pos) const
  {
    return pos.get_x() < width && pos.get_y() < height;
  }
  
  int Map::add_tank(const Pos &pos)
  {
    at(pos).add_status(Status::TANK);
    changes.emplace_back(pos);
    return 0;
  }
  
  int Map::add_bullet(const Pos &pos)
  {
    auto &p = at(pos);
    if (p.has(Status::WALL)) return -1;
    p.add_status(Status::BULLET);
    return 0;
  }
  
  void Map::remove_status(const Status &status, const Pos &pos)
  {
    at(pos).remove_status(status);
    changes.emplace_back(pos);
  }
  
  bool Map::has(const Status &status, const Pos &pos) const
  {
    return at(pos).has(status);
  }
  
  int Map::count(const Status &status, const Pos &pos) const
  {
    return at(pos).count(status);
  }
  
  void Map::attacked(int l, const Pos &pos)
  {
    at(pos).attacked(l);
  }
  
  int Map::get_lethality(const Pos &pos) const
  {
    return at(pos).get_lethality();
  }
  
  void Map::remove_lethality(const Pos &pos)
  {
    at(pos).remove_lethality();
  }
  
  const std::vector<Change> &Map::get_changes() const { return changes; };
  
  void Map::clear_changes() { changes.clear(); };
  
  Point &Map::at(const Pos &i)
  {
    assert(check_pos(i));
    return map[i.get_x()][i.get_y()];
  }
  
  const Point &Map::at(const Pos &i) const
  {
    assert(check_pos(i));
    return map[i.get_x()][i.get_y()];
  }
  
  void Map::make_maze()
  {
    for (int i = 0; i < width; ++i)
    {
      for (int j = 0; j < height; ++j)
      {
        if (i % 2 == 0 || j % 2 == 0)
        {
          map[i][j].add_status(map::Status::WALL);
        }
      }
    }
    std::list<Pos> way{Pos((std::size_t) random(0, (int) width / 2) * 2 - 1,
                           (std::size_t) random(0, (int) height / 2) * 2 - 1)};
    std::set<Pos> index{*way.begin()};
    auto it = way.begin();
    auto is_available = [this, &index](const Pos &pos)
    {
      return check_pos(pos)
             && !at(pos).has(Status::WALL)
             && index.find(pos) == index.end();
    };
    auto next = [&is_available, &way, &it, this, &index]() -> bool
    {
      std::vector<Pos> avail;
      Pos up(it->get_x(), it->get_y() + 2);
      Pos down(it->get_x(), it->get_y() - 2);
      Pos left(it->get_x() - 2, it->get_y());
      Pos right(it->get_x() + 2, it->get_y());
      if (is_available(up)) avail.emplace_back(up);
      if (is_available(down)) avail.emplace_back(down);
      if (is_available(left)) avail.emplace_back(left);
      if (is_available(right)) avail.emplace_back(right);
      if (avail.empty()) return false;
      
      auto &result = avail[(std::size_t) random(0, (int) avail.size())];
      Pos midpos((result.get_x() + it->get_x()) / 2, (result.get_y() + it->get_y()) / 2);
      at(midpos).remove_status(Status::WALL);
      
      it = way.insert(way.end(), midpos);
      index.insert(*it);
      it = way.insert(way.end(), result);
      index.insert(*it);
      return true;
    };
    while (true)
    {
      if (!next())
      {
        if (it != way.begin())
        {
          --it;
        }
        else
        {
          break;
        }
      }
    }
    add_space();
  }
  
  void Map::add_space()
  {
    for (size_t i = width / 6; i < width - width / 6; ++i)
    {
      for (size_t j = height / 6; j < height - height / 6; ++j)
      {
        map[i][j].remove_status(Status::WALL);
      }
    }
  }
  
  int Map::move(const Status &status, const Pos &pos, int direction)
  {
    if (!check_pos(pos))
    {
      return -1;
    }
    Pos new_pos = pos;
    switch (direction)
    {
      case 0:
        new_pos.get_y()++;
        break;
      case 1:
        new_pos.get_y()--;
        break;
      case 2:
        new_pos.get_x()--;
        break;
      case 3:
        new_pos.get_x()++;
        break;
    }
    if (!check_pos(new_pos))
    {
      return -1;
    }
    auto &new_point = at(new_pos);
    switch (status)
    {
      case Status::BULLET:
        if (new_point.has(Status::WALL)) return -1;
        break;
      case Status::TANK:
        if (new_point.has(Status::WALL) || new_point.has(Status::TANK)) return -1;
        break;
      default:
        break;
    }
    at(pos).remove_status(status);
    new_point.add_status(status);
    changes.emplace_back(pos);
    changes.emplace_back(new_pos);
    return 0;
  }
}
