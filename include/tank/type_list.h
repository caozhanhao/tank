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
#ifndef TANK_TYPE_LIST_H
#define TANK_TYPE_LIST_H

#include <string>
#include <variant>

namespace czh::type_list
{
  template<typename... List>
  struct TypeList {};
  
  struct TypeListError {};
  
  template<typename... List>
  std::variant<List...> as_variant(TypeList<List...>);
  
  template<typename List1, typename List2>
  struct link;
  template<typename ... Args1, typename ... Args2>
  struct link<TypeList<Args1...>, TypeList<Args2...>>
  {
    using type = TypeList<Args1..., Args2...>;
  };
  
  template<typename L1, typename L2>
  using link_t = typename link<L1, L2>::type;
  
  template<typename T, typename List>
  struct contains : std::true_type {};
  template<typename T, typename First, typename... Rest>
  struct contains<T, TypeList<First, Rest...>>
      : std::conditional<std::is_same_v<T, First>, std::true_type,
          contains<T, TypeList<Rest...>>>::type
  {
  };
  template<typename T>
  struct contains<T, TypeList<>> : std::false_type {};
  
  template<typename T, typename List>
  constexpr bool contains_v = contains<T, List>::value;
  
  template<typename T, typename List>
  struct index_of;
  template<typename First, typename ... Rest>
  struct index_of<First, TypeList<First, Rest...>>
  {
    static constexpr int value = 0;
  };
  template<typename T>
  struct index_of<T, TypeList<>>
  {
    static constexpr int value = -1;
  };
  template<typename T, typename First, typename ...Rest>
  struct index_of<T, TypeList<First, Rest...>>
  {
    static constexpr int temp = index_of<T, TypeList<Rest...>>
    ::value;
    static constexpr int value = temp == -1 ? -1 : temp + 1;
  };
  
  template<typename T, typename List>
  constexpr int index_of_v = index_of<T, List>::value;
  
  template<int index, typename List>
  struct index_at;
  template<int index>
  struct index_at<index, TypeList<>>
  {
    using type = TypeListError;
  };
  template<typename First, typename ... Rest>
  struct index_at<0, TypeList<First, Rest...>>
  {
    using type = First;
  };
  template<int index, typename First, typename ... Rest>
  struct index_at<index, TypeList<First, Rest...>>
  {
    using type = typename index_at<index - 1, TypeList<Rest...>>::type;
  };
  
  template<int index, typename List>
  using index_at_t = typename index_at<index, List>::type;
  
  template<typename List, size_t sz = 0>
  struct size_of;
  template<typename First, typename ...Rest, size_t sz>
  struct size_of<TypeList<First, Rest...>, sz>
  {
    static constexpr size_t value = sizeof...(Rest) + 1;
  };
  template<typename List>
  constexpr size_t size_of_v = size_of<List>::value;
}
#endif
