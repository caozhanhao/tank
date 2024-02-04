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
#ifndef TANK_COMMAND_H
#define TANK_COMMAND_H

#include "type_list.h"
#include <variant>
#include <string>
#include <vector>
#include <tuple>
#include <stdexcept>

namespace czh::cmd
{
  namespace details
  {
    using ArgTList = type_list::TypeList<std::string, int>;
    using Arg = decltype(as_variant(ArgTList{}));
    
    template<typename T>
    requires(type_list::contains_v<T, ArgTList>)
    T arg_get(const Arg &a)
    {
      if (a.index() != type_list::index_of_v<T, ArgTList>)
      {
        throw std::runtime_error("Get wrong type.");
      }
      return std::get<T>(a);
    }
    
    template<typename List, std::size_t... index>
    auto args_get_helper(const std::vector<Arg> &v, std::index_sequence<index...>)
    {
      auto tmp = std::make_tuple(v[index]...);
      auto args = std::apply([](auto &&... elems)
                             {
                               return std::make_tuple(arg_get<type_list::index_at_t<index, List>>(elems)...);
                             }, tmp);
      return args;
    }
    
    template<typename ...Args>
    auto make_index()
    {
      return std::vector<size_t>{
          {
              type_list::index_of_v<Args, ArgTList>
          }...};
    }
  }
  
  std::tuple<std::string, std::vector<details::Arg>>
  parse(const std::string &cmd);
  
  template<typename ...Args>
  auto args_get(const std::vector<details::Arg> &v)
  {
    if (v.size() != sizeof...(Args))
    {
      throw std::runtime_error("Get wrong size.");
    }
    return details::args_get_helper<type_list::TypeList<Args...>>
        (v, std::make_index_sequence<sizeof...(Args)>());
  }
  
  template<typename ...Args>
  bool args_is(const std::vector<details::Arg> &v)
  {
    static const auto expected = details::make_index<Args...>();
    if (expected.size() != v.size()) return false;
    for (size_t i = 0; i < v.size(); ++i)
    {
      if (expected[i] != v[i].index())
      {
        return false;
      }
    }
    return true;
  }
  
  void run_command(const std::string &str);
}
#endif //TANK_COMMAND_H
