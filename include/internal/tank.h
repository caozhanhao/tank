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
#ifndef TANK_TANK_H
#define TANK_TANK_H

#include "internal/game_map.h"
#include "internal/bullet.h"
#include <map>
#include <set>
#include <list>
#include <algorithm>
#include <functional>
#include <memory>

namespace czh::tank
{
  enum class NormalTankEvent
  {
    UP, DOWN, LEFT, RIGHT, FIRE
  };
  enum class AutoTankEvent
  {
    UP, DOWN, LEFT, RIGHT, FIRE, PASS
  };
  
  class Tank : public std::enable_shared_from_this<Tank>
  {
  protected:
    info::TankInfo info;
    int hp;
    map::Pos pos;
    map::Direction direction;
    bool hascleared;
    std::shared_ptr<map::Map> map;
    std::shared_ptr<std::vector<bullet::Bullet>> bullets;
  public:
    Tank(info::TankInfo info_, std::shared_ptr<map::Map> map_,
         std::shared_ptr<std::vector<bullet::Bullet>> bullets_,
         map::Pos pos_);
    
    virtual ~Tank() = default; // polymorphic
    
    void kill();
    
    int up();
  
    int down();
  
    int left();
  
    int right();
  
    int fire();
    
    [[nodiscard]]bool is_auto() const;
    
    [[nodiscard]]std::size_t get_id() const;
    
    std::string &get_name();
    
    const std::string &get_name() const;
    
    [[nodiscard]]int get_hp() const;
    
    [[nodiscard]]int &get_hp();
    
    [[nodiscard]]int get_max_hp() const;
    
    [[nodiscard]]bool is_alive() const;
    
    [[nodiscard]]bool has_cleared() const;
    
    void clear();
    
    map::Pos &get_pos();
    
    void attacked(int lethality_);
    
    [[nodiscard]]const map::Pos &get_pos() const;
    
    [[nodiscard]]map::Direction &get_direction();
    
    [[nodiscard]]const map::Direction &get_direction() const;
    
    [[nodiscard]]info::TankType get_type() const;
    
    [[nodiscard]]const info::TankInfo &get_info() const;
    
    [[nodiscard]]info::TankInfo &get_info();
    
    void revive(const map::Pos &newpos);
    
    std::string colorify_text(const std::string &str);
    
    std::string colorify_tank();
  };
  
  class NormalTank : public Tank
  {
  public:
    NormalTank(info::TankInfo info_, std::shared_ptr<map::Map> map_,
               const std::shared_ptr<std::vector<bullet::Bullet>> &bullets_,
               map::Pos pos_)
        : Tank(info_, std::move(map_), std::move(bullets_), pos_) {}
  };
  
  AutoTankEvent get_pos_direction(const map::Pos &from, const map::Pos &to);
  
  class Node
  {
  private:
    map::Pos pos;
    map::Pos last;
    int G;
    bool root;
  public:
    Node() : G(0), root(false) {}
    
    Node(map::Pos pos_, int G_, const map::Pos &last_, bool root_ = false)
        : pos(pos_), G(G_), last(last_), root(root_) {}
    
    Node(const Node &node) = default;
    
    [[nodiscard]]int get_F(const map::Pos &dest) const;
    
    int &get_G();
    
    map::Pos &get_last();
    
    [[nodiscard]]const map::Pos &get_pos() const;
    
    [[nodiscard]]bool is_root() const;
    
    std::vector<Node> get_neighbors(const std::shared_ptr<map::Map> &map) const;
  
  private:
    static bool check(const std::shared_ptr<map::Map> &map, map::Pos &pos);
  };
  
  bool operator<(const Node &n1, const Node &n2);
  
  bool is_in_firing_line(const std::shared_ptr<map::Map> &map, const map::Pos &pos, const map::Pos &target_pos);
  
  class AutoTank : public Tank
  {
  private:
    std::size_t target_id;
    map::Pos target_pos;
    map::Pos destination_pos;
    
    std::vector<AutoTankEvent> way;
    std::size_t waypos;
    bool found;
    
    bool got_stuck_in_its_way;
    std::size_t gap_count;
  public:
    AutoTank(info::TankInfo info_, std::shared_ptr<map::Map> map_,
             std::shared_ptr<std::vector<bullet::Bullet>> bullets_,
             map::Pos pos_)
        : Tank(info_, std::move(map_), std::move(bullets_), pos_),
          found(false), got_stuck_in_its_way(false),
          waypos(0), target_id(0), gap_count(0) {}
    
    void target(std::size_t target_id_, const map::Pos &target_pos_);
    
    AutoTankEvent next();
    
    std::size_t &get_target_id();
    
    void correct_direction();
    void stuck();
    void no_stuck();
    [[nodiscard]]bool get_found() const;
    [[nodiscard]]bool has_arrived();
  };
}
#endif