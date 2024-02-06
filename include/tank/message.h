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
#ifndef TANK_MESSAGE_H
#define TANK_MESSAGE_H
#pragma once

#include <vector>
#include <string>
#include <chrono>

namespace czh::msg
{
  struct Message
  {
    int from;
    std::string content;
  };
  
  void info(int id, const std::string& c);
  
  void warn(int id, const std::string& c);
  
  void error(int id, const std::string &c);
  
  int send_message(int from, int to, const std::string &msg);
}
#endif