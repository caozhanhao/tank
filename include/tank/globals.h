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
#ifndef TANK_GLOBALS_H
#define TANK_GLOBALS_H
#pragma once

#include "game_map.h"
#include "tank.h"
#include "online.h"
#include <functional>
#include <string>
#include <condition_variable>
#include <atomic>
#include <map>
#include <mutex>
#include <chrono>
#include <queue>
#include <list>

namespace czh::g
{
  struct UserData
  {
    size_t user_id;
    std::set<map::Pos> map_changes;
    std::priority_queue<msg::Message> messages;
    std::chrono::steady_clock::time_point last_update;
    std::string ip;
    int port;
    size_t screen_width;
    size_t screen_height;
  };
  
  // game.cpp
  extern game::GameMode game_mode;
  extern std::map<size_t, UserData> userdata;
  extern size_t user_id;
  extern size_t next_id;
  extern std::chrono::milliseconds tick;
  extern std::chrono::milliseconds msg_ttl;
  extern std::mutex mainloop_mtx;
  extern std::mutex tank_reacting_mtx;
  extern std::map<std::size_t, tank::Tank *> tanks;
  extern std::list<bullet::Bullet *> bullets;
  extern std::vector<std::pair<std::size_t, tank::NormalTankEvent>> normal_tank_events;
  
  // term.cpp
  extern int keyboard_mode;
  
  // input.cpp
  extern std::vector<std::string> history;
  extern std::string cmd_string;
  extern size_t history_pos;
  extern size_t cmd_string_pos;
  
  // renderer.cpp
  extern bool output_inited;
  extern size_t tank_focus;
  extern game::Page curr_page;
  extern size_t help_page;
  extern map::Zone render_zone;
  extern std::size_t screen_height;
  extern std::size_t screen_width;
  extern int fps;
  extern renderer::Frame frame;
  extern std::chrono::steady_clock::time_point last_render;
  extern std::chrono::steady_clock::time_point last_message_displayed;
  extern renderer::PointView empty_point_view;
  extern renderer::PointView wall_point_view;
  extern renderer::Style style;
  
  // game_map.cpp
  extern map::Map game_map;
  extern size_t seed;
  extern map::Point empty_point;
  extern map::Point wall_point;
  
  // online.cpp
  extern online::TankServer online_server;
  extern online::TankClient online_client;
  extern std::mutex online_mtx;
  extern int client_failed_attempts;
  extern int delay; // ms
}
#endif