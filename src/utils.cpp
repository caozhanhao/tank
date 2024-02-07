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
  std::string to_str(const std::string& a)
  {
    return a;
  }
  
  std::string to_str(const char*& a)
  {
    return {a};
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
  
  size_t escape_code_len(const std::string::const_iterator& beg, const std::string::const_iterator& end)
  {
    size_t ret = 0;
    std::string n;
    for (auto it = beg; it < end; ++it)
    {
      if (*it == '\033')
      {
        while (it < end && *it != 'm') ++it;
        continue;
      }
      ++ret;
      n+= *it;
    }
    return ret;
  }
  
  size_t escape_code_len(const std::string &str)
  {
    return escape_code_len(str.cbegin(), str.cend());
  }
  
  std::string effect(const std::string &str, Effect effect_)
  {
    if (str.empty()) return "";
    if (effect_ == utils::Effect::bg_shadow)
      return "\033[48;5;7m" + str + "\033[49m";
    else if (effect_ == utils::Effect::bg_strong_shadow)
      return "\033[48;5;8m" + str + "\033[49m";
    
    int effect = static_cast<int>(effect_);
    int end = 0;
    if (effect >= 1 && effect <= 7)
      end = 0;
    else if (effect >= 30 && effect <= 37)
      end = 39;
    else if (effect >= 40 && effect <= 47)
      end = 49;
    return "\033[" + std::to_string(effect) + "m" + str + "\033[" + std::to_string(end) + "m";
  }
  
  std::string red(const std::string &str)
  {
    return effect(str, Effect::fg_red);
  }
  std::string green(const std::string &str)
  {
    return effect(str, Effect::fg_green);
  }
  std::string yellow(const std::string &str)
  {
    return effect(str, Effect::fg_yellow);
  }
  std::string blue(const std::string &str)
  {
    return effect(str, Effect::fg_blue);
  }
  std::string magenta(const std::string &str)
  {
    return effect(str, Effect::fg_magenta);
  }
  std::string cyan(const std::string &str)
  {
    return effect(str, Effect::fg_cyan);
  }
  std::string white(const std::string &str)
  {
    return effect(str, Effect::fg_white);
  }
}