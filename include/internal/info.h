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
#ifndef TANK_INFO_H
#define TANK_INFO_H

#include <string>

namespace czh::info
{
  struct BulletInfo
  {
    int blood;
    int lethality;
    int circle;
    int range;
  };
  enum class TankType
  {
    AUTO, NORMAL
  };
  struct TankInfo
  {
    int max_blood;
    std::string name;
    std::size_t id;
    std::size_t level; // AutoTank only
    TankType type;
    BulletInfo bullet;
  };
}
#endif