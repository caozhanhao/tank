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
#ifndef TANK_GAME_MAP_H
#define TANK_GAME_MAP_H

#include <vector>
#include <list>
#include <set>
#include <algorithm>
#include <random>

namespace czh::map
{
  enum class Status
  {
    WALL, TANK, BULLET, END
  };
  enum class Direction
  {
    UP, DOWN, LEFT, RIGHT
  };
  
  class Point
  {
  private:
    std::vector<Status> statuses;
  public:
    void add_status(const Status &status);
    
    void remove_status(const Status &status);
    void remove_all_statuses();
    [[nodiscard]] bool has(const Status &status) const;
    
    [[nodiscard]]std::size_t count(const Status &status) const;
  };
  
  class Pos
  {
  private:
    std::size_t x;
    std::size_t y;
  public:
    Pos() : x(0), y(0) {}
  
    Pos(std::size_t x_, std::size_t y_) : x(x_), y(y_) {}
    
    std::size_t &get_x();
    
    std::size_t &get_y();
    
    [[nodiscard]]const std::size_t &get_x() const;
    
    [[nodiscard]]const std::size_t &get_y() const;
    
    bool operator==(const Pos &pos) const;
    
    bool operator!=(const Pos &pos) const;
  };
  
  bool operator<(const Pos &pos1, const Pos &pos2);
  
  std::size_t get_distance(const map::Pos &from, const map::Pos &to);
  
  class Change
  {
  private:
    Pos pos;
  public:
    explicit Change(Pos pos_) : pos(pos_) {}
    
    const Pos &get_pos() const;
    
    Pos &get_pos();
  };
  
  bool operator<(const Change &c1, const Change &c2);
  
  class Map
  {
  private:
    std::size_t height;
    std::size_t width;
    std::vector<std::vector<Point>> map;
    std::set<Change> changes;
  public:
    Map(std::size_t width_, std::size_t height_);
    
    [[nodiscard]]size_t get_width() const;
    
    [[nodiscard]] size_t get_height() const;
    
    int up(const Status &status, const Pos &pos);
    
    int down(const Status &status, const Pos &pos);
    
    int left(const Status &status, const Pos &pos);
    
    int right(const Status &status, const Pos &pos);
    
    [[nodiscard]]bool check_pos(const Pos &pos) const;
    
    int add_tank(const Pos &pos);
    
    int add_bullet(const Pos &pos);
    
    void remove_status(const Status &status, const Pos &pos);
    
    bool has(const Status &status, const Pos &pos) const;
    
    int count(const Status &status, const Pos &pos) const;
    
    const std::set<Change> & get_changes() const;
    
    void clear_changes();
  
    void clear_maze();
  
    int fill(const Pos& from, const Pos& to, const Status& status = Status::END);
  private:
    Point &at(const Pos &i);
    
    const Point &at(const Pos &i) const;
  
    void make_maze();
    
    void add_space();
    
    int move(const Status &status, const Pos &pos, int direction);
  };
}
#endif