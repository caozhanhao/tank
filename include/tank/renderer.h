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

namespace czh::renderer
{
  std::string colorify_text(int id, std::string str);
  
  std::string colorify_tank(int id, std::string str);
  
  void render();
}
#endif //TANK_RENDERER_H