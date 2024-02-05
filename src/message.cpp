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
#include "tank/message.h"
#include "tank/globals.h"

namespace czh::msg
{
  void send_message(int from, int to, const std::string &msg)
  {
    if(to == -1)
    {
      for(auto& r : g::userdata)
        r.second.messages.emplace_back(Message{.from = from, .content = msg});
    }
    else
      g::userdata[to].messages.emplace_back(Message{.from = from, .content = msg});
  }
  
  void log_helper(int id, const std::string &s, const std::string &content)
  {
    send_message(-1, id, s + content);
  }
  
  void info(int id, const std::string &c)
  {
    log_helper(id, "[INFO] ", c);
  }
  
  void warn(int id, const std::string &c)
  {
    log_helper(id, "\x1B[93m[WARNING]\x1B[0m\x1B[0K ", c);
  }
  
  void error(int id, const std::string &c)
  {
    log_helper(id, "\x1B[91m[ERROR]\x1B[0m\x1B[0K ", c);
  }
}
