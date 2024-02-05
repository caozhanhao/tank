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
#ifndef TANK_ONLINE_H
#define TANK_ONLINE_H
#pragma once

#include "tank.h"
#include "game_map.h"
#include "bundled/cpp-httplib/httplib.h"

namespace czh::online
{
  class Server
  {
  private:
    httplib::Server svr;
  public:
    Server();
    
    void start(int port);
  };
  
  class Client
  {
  private:
    std::string host;
    int port;
  public:
    Client();
    unsigned long connect(const std::string &addr_, int port_);
    void tank_react(tank::NormalTankEvent e);
    void update();
  };
}
#endif