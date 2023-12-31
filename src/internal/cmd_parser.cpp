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
#include "internal/cmd_parser.h"
namespace czh::cmd
{
  std::tuple<std::string, std::vector<details::Arg>>
  parse(const std::string &cmd)
  {
    if(cmd.empty()) return {};
    auto it = cmd.begin() + 1; // skip '/'
    auto skip_space = [&it, &cmd]{while(std::isspace(*it) && it < cmd.end()) ++it;};
    skip_space();
    
    std::string name;
    while(!std::isspace(*it) && it < cmd.end())
      name += *it++;
    
    std::vector<details::Arg> args;
    while(it < cmd.end())
    {
      skip_space();
      std::string temp;
      bool is_int = true;
      while (!std::isspace(*it) && it < cmd.end())
      {
        if(!std::isdigit(*it) && *it != '+' && *it != '-') is_int = false;
        temp += *it++;
      }
      if(!temp.empty())
      {
        if (is_int)
          args.emplace_back(std::stoi(temp));
        else
          args.emplace_back(temp);
      }
    }
    return {name, args};
  }
}