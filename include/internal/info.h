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
#ifndef TANK_INFO_H
#define TANK_INFO_H

#include <functional>
#include <string>

namespace czh::info
{
  struct BulletInfo
  {
    int hp;
    int lethality;
    int range;
    std::function<std::string(int)> text;
  };
  enum class TankType
  {
    AUTO, NORMAL
  };
  struct TankInfo
  {
    int max_hp;
    std::string name;
    std::size_t id;
    int gap;
    TankType type;
    std::function<BulletInfo()> bullet;
  };
}
#endif