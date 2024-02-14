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
#pragma once

#include <string>
#include <vector>

namespace czh::input
{
  enum class Input
  {
    UNEXPECTED,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    KEY_SPACE,
    KEY_O,
    KEY_L,
    KEY_SLASH,
    KEY_CTRL_C,
    KEY_ENTER,
    COMMAND
  };
  
  enum class SpecialKey
  {
    CTRL_A = 1,
    CTRL_B = 2,
    CTRL_C = 3,
    CTRL_D = 4,
    CTRL_E = 5,
    CTRL_F = 6,
    
    CTRL_H = 8,
    TAB = 9,
    LINE_FEED = 10,
    CTRL_K = 11,
    CTRL_L = 12,
    ENTER = 13,
    CTRL_N = 14,
    
    CTRL_P = 16,
    
    CTRL_T = 20,
    CTRL_U = 21,
    
    CTRL_W = 23,
    
    ESC = 27,
    
    BACKSPACE = 127
  };
  
  void edit_refresh_line(bool with_hint = true);
  
  Input get_input();
}
#endif