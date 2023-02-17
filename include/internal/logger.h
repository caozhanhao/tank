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
#ifndef TANK_LOGGER_H
#define TANK_LOGGER_H

#include "term.h"
#include <stdexcept>
#include <string>
#include <iostream>
#include <chrono>
#include <ctime>
#include <cstdio>

#define CZH_NOTICE(msg) czh::logger::output_at_bottom(std::string("N: ") + msg);
namespace czh::logger
{
  void output_at_bottom(const std::string &str);
  
  std::string get_time();
}
#endif