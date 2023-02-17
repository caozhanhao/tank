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
#include "qwrpc/qwrpc.hpp"
#include <chrono>
#include <thread>

using namespace qwrpc;

int main()
{
  qwrpc::RpcClient cli("127.0.0.1", 8765);
  auto id = cli.call<size_t>("add_tank");
  char ch;
  while (true)
  {
    if (czh::term::kbhit())
    {
      switch (ch = czh::term::getch())
      {
        case 'w':
          cli.call<void>("up", id);
          break;
        case 's':
          cli.call<void>("down", id);
          break;
        case 'a':
          cli.call<void>("left", id);
          break;
        case 'd':
          cli.call<void>("right", id);
          break;
        case ' ':
          cli.call<void>("fire", id);
          break;
      }
    }
  }
  return 0;
}
