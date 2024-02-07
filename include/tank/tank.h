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
#ifndef TANK_TANK_H
#define TANK_TANK_H
#pragma once

#include "game_map.h"
#include "bullet.h"
#include <map>
#include <set>
#include <list>
#include <algorithm>
#include <utility>
#include <variant>
#include <functional>

namespace czh::tank
{
  class Tank;
  
  Tank *build_tank(const map::TankData &data);
  
  map::TankData get_tank_data(Tank *);
  
  class Tank
  {
    friend Tank *build_tank(const map::TankData &data);
    
    friend map::TankData get_tank_data(Tank *);
  
  protected:
    info::TankInfo info;
    int hp;
    map::Pos pos;
    map::Direction direction;
    bool hascleared;
  public:
    Tank(info::TankInfo info_, map::Pos pos_);
    
    virtual ~Tank() = default;
    
    void kill();
    
    int up();
    
    int down();
    
    int left();
    
    int right();
    
    int fire();
    
    [[nodiscard]]bool is_auto() const;
    
    [[nodiscard]]std::size_t get_id() const;
    
    std::string &get_name();
    
    [[nodiscard]] const std::string &get_name() const;
    
    [[nodiscard]]int get_hp() const;
    
    [[nodiscard]]int &get_hp();
    
    [[nodiscard]]int get_max_hp() const;
    
    [[nodiscard]]bool is_alive() const;
    
    [[nodiscard]]bool has_cleared() const;
    
    void clear();
    
    map::Pos &get_pos();
    
    virtual void attacked(int lethality_);
    
    [[nodiscard]]const map::Pos &get_pos() const;
    
    [[nodiscard]]map::Direction &get_direction();
    
    [[nodiscard]]const map::Direction &get_direction() const;
    
    [[nodiscard]]info::TankType get_type() const;
    
    [[nodiscard]]const info::TankInfo &get_info() const;
    
    [[nodiscard]]info::TankInfo &get_info();
    
    void revive(const map::Pos &newpos);
    
  };
  
  class NormalTank : public Tank
  {
    friend Tank *build_tank(const map::TankData &data);
    
    friend map::TankData get_tank_data(Tank *);
  
  public:
    NormalTank(info::TankInfo info_, map::Pos pos_)
        : Tank(std::move(info_), pos_) {}
    
    ~NormalTank() override = default;
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
    
    [[nodiscard]] std::vector<Node> get_neighbors() const;
  
  private:
    bool check(map::Pos &pos) const;
  };
  
  bool operator<(const Node &n1, const Node &n2);
  
  bool is_in_firing_line(int range, const map::Pos &pos, const map::Pos &target_pos);
  
  class AutoTank : public Tank
  {
    friend Tank *build_tank(const map::TankData &data);
    
    friend map::TankData get_tank_data(Tank *);
  
  private:
    std::size_t target_id;
    map::Pos target_pos;
    map::Pos destination_pos;
    
    std::vector<AutoTankEvent> way;
    std::size_t waypos;
    
    int gap_count;
  public:
    AutoTank(info::TankInfo info_, map::Pos pos_)
        : Tank(std::move(info_), pos_), waypos(0), target_id(0), gap_count(0) {}
    
    ~AutoTank() override = default;
    
    void target(std::size_t target_id_, const map::Pos &target_pos_);
    
    void react();
    
    std::size_t &get_target_id();
    
    void correct_direction(const map::Pos &target);
    
    [[nodiscard]]bool has_arrived() const;
    
    void attacked(int lethality_) override;
    
    void generate_random_way();
  };
}
#endif