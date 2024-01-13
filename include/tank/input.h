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
#ifndef TANK_INPUT_H
#define TANK_INPUT_H

#include <string>

namespace czh::input
{
  enum class Input
  {
    CHAR,
    C_KEY_UP, C_KEY_DOWN, C_KEY_LEFT, C_KEY_RIGHT, C_KEY_ENTER,
    C_KEY_BACKSPACE, C_KEY_DELETE, C_KEY_HOME, C_KEY_END,
    G_UP, G_DOWN, G_LEFT, G_RIGHT, G_KEY_SPACE, G_KEY_O, G_KEY_L, G_KEY_SLASH,
    M_KEY_ENTER,
    EMPTY
  };
  Input get_input();
}
#endif