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
#include "tank/utils.h"

namespace czh::utils
{
  void tank_assert(bool b, const std::string &detail_)
  {
    if (!b)
    {
      throw std::runtime_error(detail_);
    }
  }
  
  std::string to_str(const std::string &a)
  {
    return a;
  }
  
  std::string to_str(const char *&a)
  {
    return {a};
  }
  
  std::string to_str(char a)
  {
    return std::string(1, a);
  }
  
  bool begin_with(const std::string &a, const std::string &b)
  {
    if (a.size() < b.size()) return false;
    for (size_t i = 0; i < b.size(); ++i)
    {
      if (a[i] != b[i])
      {
        return false;
      }
    }
    return true;
  }
  
  size_t escape_code_len(const std::string::const_iterator &beg, const std::string::const_iterator &end)
  {
    size_t ret = 0;
    std::string n;
    for (auto it = beg; it < end; ++it)
    {
      if (*it == '\x1b')
      {
        while (it < end && *it != 'm') ++it;
        continue;
      }
      ++ret;
      n += *it;
    }
    return ret;
  }
  
  size_t escape_code_len(const std::string &str)
  {
    return escape_code_len(str.cbegin(), str.cend());
  }
  
  std::string color_256_fg(const std::string &str, int color)
  {
    return "\x1b[38;5;" + std::to_string(color) + "m" + str + "\x1b[0m";
  }
  
  std::string color_256_bg(const std::string &str, int color)
  {
    return "\x1b[48;5;" + std::to_string(color) + "m" + str + "\x1b[0m";
  }

//  std::string color_rgb_fg(const std::string &str, const RGB& rgb)
//  {
//    return "\x1b[38;2;" + std::to_string(rgb.r) + ";"
//           + std::to_string(rgb.g) + ";"
//           + std::to_string(rgb.b) + "m"
//           + str + "\x1b[0m";
//  }
//  std::string color_rgb_bg(const std::string &str, const RGB& rgb)
//  {
//    return "\x1b[48;2;" + std::to_string(rgb.r) + ";"
//           + std::to_string(rgb.g) + ";"
//           + std::to_string(rgb.b) + "m"
//           + str + "\x1b[0m";
//  }
}