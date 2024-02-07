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
}