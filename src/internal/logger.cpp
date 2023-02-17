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
#include "internal/term.h"
#include <string>
#include <chrono>
#include <ctime>
#include <cstdio>
namespace czh::logger
{
  void output_at_bottom(const std::string &str)
  {
    term::move_cursor(term::TermPos(0, term::get_height() - 1));
    int a = term::get_width() - str.size();
    if (a > 0)
    {
      term::output(str + std::string(a, ' '));
    }
    else
    {
      term::output(str);
    }
  }
  
  std::string get_time()
  {
    auto tt = std::chrono::system_clock::to_time_t
        (std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int) ptm->tm_year + 1900, (int) ptm->tm_mon + 1, (int) ptm->tm_mday,
            (int) ptm->tm_hour, (int) ptm->tm_min, (int) ptm->tm_sec);
    return {date};
  }
}