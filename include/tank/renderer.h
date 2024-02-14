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
#include "utils.h"

#include <string>

namespace czh::renderer
{
  // Xterm 256 color
  // https://www.ditig.com/publications/256-colors-cheat-sheet
  struct Style
  {
    int background;
    int wall;
    int default_tank;
  };
  
  std::string colorify_text(size_t id, const std::string &str);
  
  std::string colorify_tank(size_t id, const std::string &str);
  
  struct PointView
  {
    map::Status status;
    int tank_id;
    std::string text;
    
    [[nodiscard]] bool is_empty() const;
  };
  
  bool operator<(const PointView &c1, const PointView &c2);
  
  struct MapView
  {
    std::map<map::Pos, PointView> view;
    size_t seed;
    
    [[nodiscard]] const PointView &at(const map::Pos &i) const;
    
    [[nodiscard]] const PointView &at(int x, int y) const;
    
    [[nodiscard]] bool is_empty() const;
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
  };
  
  extern PointView empty_point_view;
  extern PointView wall_point_view;
  
  const PointView &generate(const map::Pos &i, size_t seed);
  
  const PointView &generate(int x, int y, size_t seed);
  
  PointView extract_point(const map::Pos &pos);
  
  MapView extract_map(const map::Zone &zone);
  
  std::map<size_t, TankView> extract_tanks();
  
  map::Zone get_visible_zone(size_t w, size_t h, size_t id);
  
  int update_frame();
  
  void render();
}
#endif //TANK_RENDERER_H
