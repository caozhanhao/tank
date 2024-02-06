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
#ifndef TANK_RENDERER_H
#define TANK_RENDERER_H
#pragma once

#include "game_map.h"
#include "game.h"

#include <string>
namespace czh::renderer
{
  std::string colorify_text(int id, std::string str);
  
  std::string colorify_tank(int id, std::string str);
  
  struct PointView
  {
    map::Pos pos;
    map::Status status;
    int tank_id;
    std::string text;
    
    bool is_empty() const;
  };
  
  bool operator<(const PointView &c1, const PointView &c2);
  struct MapView
  {
    std::map<map::Pos, PointView> view;
    
    const PointView &at(const map::Pos &i) const;
    const PointView &at(int x, int y) const;
  };
  
  struct TankView
  {
    info::TankInfo info;
    int hp;
    map::Pos pos;
    map::Direction direction;
    bool is_auto;
    bool is_alive;
  };
  
  struct Frame
  {
    MapView map;
    std::map<size_t, TankView> tanks;
    std::set<map::Pos> changes;
    map::Zone zone;
  };
  
  PointView extract_point(const map::Pos& pos);
  MapView extract_map(const map::Zone &zone);
  std::map<size_t, TankView> extract_tanks();
  
  std::optional<Frame> get_frame();
  void render(Frame f);
}
#endif //TANK_RENDERER_H
