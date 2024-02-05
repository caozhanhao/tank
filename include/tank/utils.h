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
#ifndef TANK_UTILS_H
#define TANK_UTILS_H
#pragma once

#include <string_view>
#include <string>
#include <stdexcept>
#include <random>

namespace czh::utils
{
  template<typename T>
  T randnum(T a, T b)// [a, b)
  {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<T> u(a, b - 1);
    return u(gen);
  }
  
  template<typename BeginIt, typename EndIt>
  concept ItRange =
  requires(BeginIt begin_it, EndIt end_it)
  {
    { ++begin_it };
    { *begin_it };
    requires std::is_same_v<std::decay_t<decltype(*begin_it)>, std::string_view>;
    { begin_it != end_it };
  };
  template<typename T>
  concept Container =
  requires(T value)
  {
    { value.begin() };
    { value.end() };
    requires ItRange<decltype(value.begin()), decltype(value.end())>;
  };
  
  template<Container T>
  T split(std::string_view str, std::string_view delims = " ")
  {
    T ret;
    size_t first = 0;
    while (first < str.size())
    {
      const auto second = str.find_first_of(delims, first);
      if (first != second)
      {
        ret.insert(ret.end(), str.substr(first, second - first));
      }
      if (second == std::string_view::npos)
      {
        break;
      }
      first = second + 1;
    }
    return ret;
  }
  
  template<Container T>
  T fit_to_screen(const T &container, int w)
  {
    T ret;
    for (auto it = container.begin(); it < container.end(); ++it)
    {
      if (it->size() > w)
      {
        ret.insert(ret.end(), it->substr(0, w));
        ret.insert(ret.end(), it->substr(w));
      }
      else
      {
        ret.insert(ret.end(), *it);
      }
    }
    return ret;
  }
  
  void tank_assert(bool b, const std::string &detail_ = "Assertion failed.");
}
#endif
