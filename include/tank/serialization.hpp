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
#ifndef TANK_SERIALIZATION_H
#define TANK_SERIALIZATION_H
#pragma once

#include "utils.h"

#include <type_traits>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <queue>
#include <iterator>

namespace czh::ser
{
  template<typename T>
  T deserialize(const std::string &str);
  
  template<typename T>
  std::string serialize(const T &item);
  
  namespace details
  {
    struct Any
    {
      template<typename T>
      operator T();
    };
  
    template<typename T>
    consteval auto field_num(auto&&... args)
    {
      if constexpr (!requires{ T{ args... }; })
      {
        return sizeof ...(args) - 1;
      }
      else
      {
        return field_num<T>(args..., Any{});
      }
    }
    
    constexpr int supported_field_num = 64;
    template<typename T, typename Fn> requires std::is_aggregate_v<std::decay_t<T>>
    constexpr auto field_for_each(T&& t, Fn&& fn)
    {
      // Python:
      //fmt = "else if constexpr (num == {0}) {{auto&& [{1}] = std::forward<T>(t);{2}}}"
      //
      //print(fmt)
      //num = int(input("num: "))
      //for i in range(2, num + 2):
      //    str0 = str(i - 1)
      //    str1 = ""
      //    for j in range(1, i):
      //        str1 += "_" + str(j) + ", "
      //    str1 = str1[:-2]
      //    str2 = ""
      //    for j in range(1, i):
      //        str2 += "fn(" + "_" + str(j) + ");"
      //    print(fmt.format(str0, str1, str2))
      constexpr auto num = field_num<std::decay_t<T>>();
      if constexpr (num == 1) {auto&& [_1] = std::forward<T>(t);fn(_1);}
      else if constexpr (num == 2) {auto&& [_1, _2] = std::forward<T>(t);fn(_1);fn(_2);}
      else if constexpr (num == 3) {auto&& [_1, _2, _3] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);}
      else if constexpr (num == 4) {auto&& [_1, _2, _3, _4] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);}
      else if constexpr (num == 5) {auto&& [_1, _2, _3, _4, _5] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);}
      else if constexpr (num == 6) {auto&& [_1, _2, _3, _4, _5, _6] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);}
      else if constexpr (num == 7) {auto&& [_1, _2, _3, _4, _5, _6, _7] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);}
      else if constexpr (num == 8) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);}
      else if constexpr (num == 9) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);}
      else if constexpr (num == 10) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);}
      else if constexpr (num == 11) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);}
      else if constexpr (num == 12) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);}
      else if constexpr (num == 13) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);}
      else if constexpr (num == 14) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);}
      else if constexpr (num == 15) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);}
      else if constexpr (num == 16) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);}
      else if constexpr (num == 17) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);}
      else if constexpr (num == 18) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);}
      else if constexpr (num == 19) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);}
      else if constexpr (num == 20) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);}
      else if constexpr (num == 21) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);}
      else if constexpr (num == 22) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);}
      else if constexpr (num == 23) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);}
      else if constexpr (num == 24) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);}
      else if constexpr (num == 25) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);}
      else if constexpr (num == 26) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);}
      else if constexpr (num == 27) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);}
      else if constexpr (num == 28) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);}
      else if constexpr (num == 29) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);}
      else if constexpr (num == 30) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);}
      else if constexpr (num == 31) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);}
      else if constexpr (num == 32) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);}
      else if constexpr (num == 33) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);}
      else if constexpr (num == 34) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);}
      else if constexpr (num == 35) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);}
      else if constexpr (num == 36) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);}
      else if constexpr (num == 37) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);}
      else if constexpr (num == 38) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);}
      else if constexpr (num == 39) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);}
      else if constexpr (num == 40) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);}
      else if constexpr (num == 41) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);}
      else if constexpr (num == 42) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);}
      else if constexpr (num == 43) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);}
      else if constexpr (num == 44) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);}
      else if constexpr (num == 45) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);fn(_45);}
      else if constexpr (num == 46) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);fn(_45);fn(_46);}
      else if constexpr (num == 47) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);fn(_45);fn(_46);fn(_47);}
      else if constexpr (num == 48) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);fn(_45);fn(_46);fn(_47);fn(_48);}
      else if constexpr (num == 49) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);fn(_45);fn(_46);fn(_47);fn(_48);fn(_49);}
      else if constexpr (num == 50) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);fn(_45);fn(_46);fn(_47);fn(_48);fn(_49);fn(_50);}
      else if constexpr (num == 51) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);fn(_45);fn(_46);fn(_47);fn(_48);fn(_49);fn(_50);fn(_51);}
      else if constexpr (num == 52) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);fn(_45);fn(_46);fn(_47);fn(_48);fn(_49);fn(_50);fn(_51);fn(_52);}
      else if constexpr (num == 53) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);fn(_45);fn(_46);fn(_47);fn(_48);fn(_49);fn(_50);fn(_51);fn(_52);fn(_53);}
      else if constexpr (num == 54) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);fn(_45);fn(_46);fn(_47);fn(_48);fn(_49);fn(_50);fn(_51);fn(_52);fn(_53);fn(_54);}
      else if constexpr (num == 55) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);fn(_45);fn(_46);fn(_47);fn(_48);fn(_49);fn(_50);fn(_51);fn(_52);fn(_53);fn(_54);fn(_55);}
      else if constexpr (num == 56) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);fn(_45);fn(_46);fn(_47);fn(_48);fn(_49);fn(_50);fn(_51);fn(_52);fn(_53);fn(_54);fn(_55);fn(_56);}
      else if constexpr (num == 57) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);fn(_45);fn(_46);fn(_47);fn(_48);fn(_49);fn(_50);fn(_51);fn(_52);fn(_53);fn(_54);fn(_55);fn(_56);fn(_57);}
      else if constexpr (num == 58) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);fn(_45);fn(_46);fn(_47);fn(_48);fn(_49);fn(_50);fn(_51);fn(_52);fn(_53);fn(_54);fn(_55);fn(_56);fn(_57);fn(_58);}
      else if constexpr (num == 59) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);fn(_45);fn(_46);fn(_47);fn(_48);fn(_49);fn(_50);fn(_51);fn(_52);fn(_53);fn(_54);fn(_55);fn(_56);fn(_57);fn(_58);fn(_59);}
      else if constexpr (num == 60) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);fn(_45);fn(_46);fn(_47);fn(_48);fn(_49);fn(_50);fn(_51);fn(_52);fn(_53);fn(_54);fn(_55);fn(_56);fn(_57);fn(_58);fn(_59);fn(_60);}
      else if constexpr (num == 61) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);fn(_45);fn(_46);fn(_47);fn(_48);fn(_49);fn(_50);fn(_51);fn(_52);fn(_53);fn(_54);fn(_55);fn(_56);fn(_57);fn(_58);fn(_59);fn(_60);fn(_61);}
      else if constexpr (num == 62) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);fn(_45);fn(_46);fn(_47);fn(_48);fn(_49);fn(_50);fn(_51);fn(_52);fn(_53);fn(_54);fn(_55);fn(_56);fn(_57);fn(_58);fn(_59);fn(_60);fn(_61);fn(_62);}
      else if constexpr (num == 63) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);fn(_45);fn(_46);fn(_47);fn(_48);fn(_49);fn(_50);fn(_51);fn(_52);fn(_53);fn(_54);fn(_55);fn(_56);fn(_57);fn(_58);fn(_59);fn(_60);fn(_61);fn(_62);fn(_63);}
      else if constexpr (num == 64) {auto&& [_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64] = std::forward<T>(t);fn(_1);fn(_2);fn(_3);fn(_4);fn(_5);fn(_6);fn(_7);fn(_8);fn(_9);fn(_10);fn(_11);fn(_12);fn(_13);fn(_14);fn(_15);fn(_16);fn(_17);fn(_18);fn(_19);fn(_20);fn(_21);fn(_22);fn(_23);fn(_24);fn(_25);fn(_26);fn(_27);fn(_28);fn(_29);fn(_30);fn(_31);fn(_32);fn(_33);fn(_34);fn(_35);fn(_36);fn(_37);fn(_38);fn(_39);fn(_40);fn(_41);fn(_42);fn(_43);fn(_44);fn(_45);fn(_46);fn(_47);fn(_48);fn(_49);fn(_50);fn(_51);fn(_52);fn(_53);fn(_54);fn(_55);fn(_56);fn(_57);fn(_58);fn(_59);fn(_60);fn(_61);fn(_62);fn(_63);fn(_64);}      else
      {
        static_assert(num <= supported_field_num, "too many fields");
      }
    }
  
    struct NotImplementedTag {};
    struct TriviallyCopyableTag {};
    struct ContainerTag {};
    struct StdStringTag {};
    struct StdMapTag {};
    struct StdPairTag {};
    struct StructTag {};
    template<typename T>
    struct TagDispatch;
    
    template<typename T>
    struct is_serializable;
  
    template<typename T>
    constexpr bool is_serializable_v = is_serializable<T>::value;
  
    template <typename T>
    struct is_stdmap : public std::false_type {};
    template <typename... Arg>
    struct is_stdmap<std::map<Arg...>> : public std::true_type {};
    template <typename... Arg>
    struct is_stdmap<std::multimap<Arg...>> : public std::true_type {};
    template<typename T>
    constexpr bool is_stdmap_v = is_stdmap<T>::value;
  
    template <typename T>
    struct is_stdpair : public std::false_type {};
    template <typename F, typename S>
    struct is_stdpair<std::pair<F, S>> : public std::true_type {};
    template <typename T>
    constexpr bool is_stdpair_v = is_stdpair<T>::value;
    
    template<typename T>
    constexpr bool is_serializable_basic_type_v =
        std::is_trivially_copyable_v<T> || std::is_same_v<T, std::string> || is_stdmap_v<T> || is_stdpair_v<T>;
  
  
    template<typename BeginIt, typename EndIt>
    concept ItRange =
    requires(BeginIt begin_it, EndIt end_it)
    {
      { ++begin_it };
      { *begin_it };
      requires !std::is_void_v<decltype(*begin_it)>;
      { begin_it != end_it };
    };
  
    template<typename T>
    concept Container =
    requires(T value)
    {
      { std::begin(value) };
      { std::end(value) };
      requires ItRange<decltype(std::begin(value)), decltype(std::end(
          value))>;
    };
    
    template<typename T>
    concept SerializableContainer =
    Container<T> && requires(T value)
    {
      { value.insert(std::end(value), std::declval<decltype(*std::begin(value))>()) };
      { value.size() };
      //requires(is_serializable_v<std::remove_cvref_t<decltype(*std::begin(value))>>);
    };
  
    template<typename T, typename U = void>
    struct is_container : public std::false_type {};
    template<typename T> requires Container<T>
    struct is_container<T> : public std::true_type {};
    template<typename T>
    constexpr bool is_container_v = is_container<T>::value;
    
    template<typename T, typename U = void>
    struct is_serializable_container : public std::false_type {};
    template<typename T> requires SerializableContainer<T>
    struct is_serializable_container<T> : public std::true_type {};
    template<typename T>
    constexpr bool is_serializable_container_v = is_serializable_container<T>::value;
  
    template<typename T>
    consteval bool field_serializable()
    {
      bool ok = true;
      field_for_each(std::declval<T>(), [&ok](auto&& t)
      {
        if(!is_serializable_v<decltype(t)>)
          ok = false;
      });
      return ok;
    }
  
    template<typename T>
    constexpr bool is_serializable_struct_v =
    !is_container_v<T>
    && std::is_aggregate_v<T>;
    //&& field_num<T>() <= supported_field_num
    //&& field_serializable<T>();
    
    template<typename T>
    struct is_serializable
    {
      static constexpr bool value = (is_serializable_container_v<T>
                                     || is_serializable_basic_type_v<T>
                                     || is_serializable_struct_v<T>);
    };
  
    template<typename T>
    concept Serializable = is_serializable_v<T>;
    template<typename T>
    struct TagDispatch
    {
      using tag = std::conditional_t<is_serializable_v<T>, std::conditional_t<std::is_trivially_copyable_v<std::decay_t<T>>, TriviallyCopyableTag,
          std::conditional_t<std::is_same_v<std::decay_t<T>, std::string>, StdStringTag,
              std::conditional_t<is_stdmap_v<std::decay_t<T>>, StdMapTag,
                  std::conditional_t<is_stdpair_v<std::decay_t<T>>, StdPairTag,
                      std::conditional_t<is_serializable_container_v<std::decay_t<T>>, ContainerTag,
                          std::conditional_t<is_serializable_struct_v<std::decay_t<T>>, StructTag,
                              NotImplementedTag>>>>>>,
          NotImplementedTag>;
    };
    
    template<typename T>
    std::string internal_serialize(NotImplementedTag, const T &item);
    
    template<typename T>
    T internal_deserialize(NotImplementedTag, const std::string &str);
    
    template<typename T>
    std::string internal_serialize(TriviallyCopyableTag, const T &item)
    {
      std::string data;
      data.resize(sizeof(T));
      std::memcpy(data.data(), &item, sizeof(T));
      return data;
    }
    
    template<typename T>
    T internal_deserialize(TriviallyCopyableTag, const std::string &str)
    {
      utils::tank_assert(str.size() == sizeof(T));
      T item;
      std::memcpy(&item, str.data(), sizeof(T));
      return item;
    }
    
    template<typename T>
    std::string internal_serialize(StdStringTag, const T &item)
    {
      return item;
    }
    
    template<typename T>
    T internal_deserialize(StdStringTag, const std::string &str)
    {
      return str;
    }
  
  
    template<typename T>
    std::string internal_serialize(StdMapTag, const T &item)
    {
      std::vector<std::pair<std::remove_const_t<typename T::key_type>,
          typename T::mapped_type>> v;
      for(auto& r : item)
        v.emplace_back(r);
      return serialize(v);
    }
  
    template<typename T>
    T internal_deserialize(StdMapTag, const std::string &str)
    {
      auto v = deserialize<std::vector<std::pair<std::remove_const_t<typename T::key_type>,
          typename T::mapped_type>>>(str);
      T ret;
      for(auto& r : v)
        ret.emplace_hint(ret.end(), r);
      return ret;
    }
    
    struct Header
    {
      uint32_t length;
    };
    
    template<typename T>
    void item_serialize_helper(std::string& buf, size_t& pos, const T& i)
    {
      auto data = serialize<T>(static_cast<T>(const_cast<std::remove_const_t<decltype(i)>>(i)));
      if (buf.size() - pos < data.size() + sizeof(Header))
      {
        buf.resize(buf.size() + data.size() + sizeof(Header) + 128);
      }
      Header header{.length = static_cast<uint32_t>(data.size())};
      std::memcpy(buf.data() + pos, &header, sizeof(Header));
      pos += sizeof(Header);
      for (size_t i = 0; i < data.size(); ++i, ++pos)
        buf[pos] = data[i];
    }
  
    template<typename T>
    auto item_deserialize_helper(const std::string& str, size_t& pos)
    {
      Header header;
      std::memcpy(&header, str.data() + pos, sizeof(Header));
      pos += sizeof(Header);
      std::string buf;
      buf.resize(header.length);
      std::memcpy(buf.data(), str.data() + pos, header.length);
      pos += header.length;
      return deserialize<std::remove_cvref_t<T>>(buf);
    }
  
    template<typename T>
    std::string internal_serialize(StdPairTag, const T &item)
    {
      std::string buf;
      buf.resize(128);
      size_t pos = sizeof(Header);
  
      item_serialize_helper<typename T::first_type>(buf, pos, item.first);
      item_serialize_helper<typename T::second_type>(buf, pos, item.second);
      
      Header header = {.length = static_cast<uint32_t>(pos - sizeof(Header))};
      std::memcpy(buf.data(), &header, sizeof(Header));
      return buf.substr(0, pos);
    }
  
    template<typename T>
    [[nodiscard]]T internal_deserialize(StdPairTag, const std::string &str)
    {
      if (str.empty()) return T{};
      T ret;
      Header header;
      std::memcpy(&header, str.data(), sizeof(Header));
      utils::tank_assert(header.length == str.size() - sizeof(Header));
      size_t pos = sizeof(Header);
      auto first = item_deserialize_helper<typename T::first_type>(str, pos);
      auto second = item_deserialize_helper<typename T::second_type>(str, pos);
      // Note:
      // The order of evaluation of function arguments is unspecified in C++,
      // so don't do something like this:
      //      return std::make_pair(item_deserialize_helper<typename T::first_type>(str, pos),
      //                            item_deserialize_helper<typename T::second_type>(str, pos));
      return std::make_pair(first, second);
    }
    
    template<typename T>
    std::string internal_serialize(ContainerTag, const T &item)
    {
      using value_type = std::remove_cvref_t<decltype(*std::begin(std::declval<T>()))>;
      std::string buf;
      buf.resize(128);
      size_t pos = sizeof(Header);
      for (auto &r: item)
        item_serialize_helper(buf, pos, r);
      Header header{.length = static_cast<uint32_t>(pos - sizeof(Header))};
      std::memcpy(buf.data(), &header, sizeof(Header));
      return buf.substr(0, pos);
    }

    template<typename T>
    T internal_deserialize(ContainerTag, const std::string &str)
    {
      if(str.empty()) return T{};
      using value_type = std::remove_cvref_t<decltype(*std::begin(std::declval<T>()))>;
      T ret;
      Header header;
      std::memcpy(&header, str.data(), sizeof(Header));
      utils::tank_assert(header.length == str.size() - sizeof(Header));
      for(size_t i = sizeof(Header); i < str.size();)
      {
        ret.insert(std::end(ret),
                   std::move(item_deserialize_helper<typename T::value_type>(str, i)));
      }
      return ret;
    }
  
    template<typename T>
    std::string internal_serialize(StructTag, const T &item)
    {
      std::string buf;
      buf.resize(128);
      size_t pos = sizeof(Header);
      field_for_each(item, [&buf, &pos](auto&& r){ item_serialize_helper(buf, pos, r);});
      Header h{.length = static_cast<uint32_t>(pos - sizeof(Header))};
      std::memcpy(buf.data(), &h, sizeof(Header));
      return buf.substr(0, pos);
    }
  
    template<typename T>
    T internal_deserialize(StructTag, const std::string &str)
    {
      if(str.empty()) return T{};
      T ret;
      Header header;
      std::memcpy(&header, str.data(), sizeof(Header));
      utils::tank_assert(header.length == str.size() - sizeof(Header));
      size_t pos = sizeof(Header);
      field_for_each(ret, [&str, &pos](auto&& r){
        r = std::move(item_deserialize_helper<decltype(r)>(str, pos));
      });
      return ret;
    }
  }
  
  template<typename T>
  std::string serialize(const T& item)
  {
    static_assert(!std::is_same_v<typename details::TagDispatch<T>::tag, details::NotImplementedTag>,
                  "Type must overload ser::serialize() and ser::deserialize()");
    return details::internal_serialize<T>(typename details::TagDispatch<T>::tag{}, item);
  }
  
  template<typename T>
  T deserialize(const std::string &str)
  {
    static_assert(!std::is_same_v<typename details::TagDispatch<T>::tag, details::NotImplementedTag>,
                  "Type must overload ser::serialize() and ser::deserialize()");
    return details::internal_deserialize<T>(typename details::TagDispatch<T>::tag{}, str);
  }
  
  template<>
  inline std::string serialize(const std::priority_queue<msg::Message, std::vector<msg::Message>>& t)
  {
    if(t.empty()) return "";
    std::vector<msg::Message> o;
    o.resize(t.size());
    std::copy(&(t.top()), &(t.top()) + t.size(), &o[0]);
    return serialize(o);
  }
  template<>
  inline std::priority_queue<msg::Message, std::vector<msg::Message>> deserialize(const std::string& str)
  {
    if(str.empty()) return {};
    auto o = deserialize<std::vector<msg::Message>>(str);
    return std::priority_queue<msg::Message, std::vector<msg::Message>>(std::make_move_iterator(o.begin()),
                                             std::make_move_iterator(o.end()));
  }
}
#endif