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

#include "tank/online.h"
#include "tank/logger.h"
#include "tank/game_map.h"
#include "tank/game.h"
#include "tank/globals.h"
#include "tank/utils.h"

#include "tank/bundled/cpp-httplib/httplib.h"

#include <string>
#include <vector>

namespace czh::g
{
  online::Server online_server{};
  online::Client online_client{};
}
namespace czh::online
{
  std::string serialize_zone(const czh::map::Zone &b)
  {
    return std::to_string(b.x_min) + "," + std::to_string(b.x_max) + ","
           + std::to_string(b.y_min) + "," +  std::to_string(b.y_max);
  }
  
  czh::map::Zone deserialize_zone(const std::string &str)
  {
    auto s = czh::utils::split<std::vector<std::string_view>>(str, ",");
    czh::map::Zone c;
    c.x_min = std::stoi(std::string{s[0]});
    c.x_max = std::stoi(std::string{s[1]});
    c.y_min = std::stoi(std::string{s[2]});
    c.y_max = std::stoi(std::string{s[3]});
    return c;
  }
  
  
  std::string serialize_changes(const std::set<map::Pos>& changes)
  {
    if(changes.empty()) return "empty";
    std::string ret;
    for(auto& b : changes)
    {
      ret += std::to_string(b.x) + "," + std::to_string(b.y) + "\n";
    }
    return ret;
  }
  
  std::set<map::Pos> deserialize_changes(const std::string &str)
  {
    if(str == "empty") return {};
    auto s1 = czh::utils::split<std::vector<std::string_view>>(str, "\n");
    std::set<map::Pos> ret;
    for (auto &r: s1)
    {
      auto s2 = czh::utils::split<std::vector<std::string_view>>(r, ",");
      ret.insert({std::stoi(std::string{s2[0]}), std::stoi(std::string{s2[1]})});
    }
    return ret;
  }
  
  std::string serialize_tanksview(const std::map<size_t, game::TankView>& view)
  {
    std::string ret;
    for(auto& b : view)
    {
      ret += b.second.info.name + ","
             + std::to_string(b.second.info.bullet.hp) + ","
             + std::to_string(b.second.info.bullet.lethality) + ","
             + std::to_string(b.second.info.bullet.range) + ","
             + std::to_string(b.second.info.gap) + ","
             + std::to_string(b.second.info.id) + ","
             + std::to_string(b.second.info.max_hp) + ","
             + std::to_string(static_cast<int>(b.second.info.type)) + ","
             + std::to_string(b.second.hp) + ","
             + std::to_string(b.second.is_alive) + ","
             + std::to_string(b.second.is_auto) + ","
             + std::to_string(static_cast<int>(b.second.direction)) + ","
             + std::to_string(b.second.pos.x) + ","
             + std::to_string(b.second.pos.y) + "\n";
    }
    return ret;
  }
  
  std::map<size_t, game::TankView> deserialize_tanksview(const std::string &str)
  {
    auto s1 = czh::utils::split<std::vector<std::string_view>>(str, "\n");
    std::map<size_t, game::TankView> ret;
    for (auto &r: s1)
    {
      auto s2 = czh::utils::split<std::vector<std::string_view>>(r, ",");
      game::TankView c;
      c.info.name = s2[0];
      c.info.bullet.hp = std::stoi(std::string {s2[1]});
      c.info.bullet.lethality = std::stoi(std::string {s2[2]});
      c.info.bullet.range = std::stoi(std::string {s2[3]});
      c.info.gap = std::stoi(std::string {s2[4]});
      c.info.id = std::stoi(std::string {s2[5]});
      c.info.max_hp = std::stoi(std::string {s2[6]});
      c.info.type = static_cast<info::TankType>(std::stoi(std::string {s2[7]}));
      c.hp = std::stoi(std::string {s2[8]});
      c.is_alive = static_cast<bool>(std::stoi(std::string {s2[9]}));
      c.is_auto = static_cast<bool>(std::stoi(std::string {s2[10]}));
      c.direction = static_cast<map::Direction>(std::stoi(std::string {s2[11]}));
      c.pos.x = std::stoi(std::string {s2[12]});
      c.pos.y = std::stoi(std::string {s2[13]});
      
      ret.insert({c.info.id, c});
    }
    return ret;
  }
  
  std::string serialize_mapview(const map::MapView& view)
  {
    std::string ret;
    for(auto& b : view.view)
    {
      ret += std::to_string(b.second.pos.x) + "," + std::to_string(b.second.pos.y) + ","
             + std::to_string(b.second.tank_id) + "," + (b.second.text.empty() ? "empty text" : b.second.text) + "," +
             std::to_string(static_cast<int>(b.second.status)) + "\n";
    }
    return ret;
  }
  
  map::MapView deserialize_mapview(const std::string &str)
  {
    auto s1 = czh::utils::split<std::vector<std::string_view>>(str, "\n");
    map::MapView ret;
    for (auto &r: s1)
    {
      auto s2 = czh::utils::split<std::vector<std::string_view>>(r, ",");
      czh::map::PointView c;
      c.pos.x = std::stoi(std::string{s2[0]});
      c.pos.y = std::stoi(std::string{s2[1]});
      c.tank_id = std::stoi(std::string{s2[2]});
      c.text = s2[3] == "empty text" ? "" : std::string{s2[3]};
      c.status = static_cast<czh::map::Status>(std::stoi(std::string{s2[4]}));
      ret.view.insert({c.pos, c});
    }
    return ret;
  }
  
    Server::Server()
    {
      svr.Get("tank_react",
              [](const httplib::Request &req, httplib::Response &res)
              {
                size_t id = std::stoi(req.get_param_value("id"));
                tank::NormalTankEvent event =
                    static_cast<tank::NormalTankEvent>(std::stoi(req.get_param_value("event")));
                game::tank_react(id, event);
              });
      svr.Get("update",
              [](const httplib::Request &req, httplib::Response &res)
              {
                std::lock_guard<std::mutex> l(g::mainloop_mtx);
                size_t id = std::stoi(req.get_param_value("id"));
                map::Zone zone = deserialize_zone(req.get_param_value("zone"));
                map::MapView map_view = g::game_map.extract(zone);
                std::set<map::Pos> changes;
                for (auto &r: g::userdata[id].map_changes)
                {
                  if (zone.contains(r))
                    changes.insert(r);
                }
                g::userdata[id].map_changes.clear();
                res.body = serialize_changes(changes) + "<>" + serialize_mapview(map_view)
                    + "<>" + serialize_tanksview(game::extract_tanks());
              });
      svr.Get("register_tank",
              [](const httplib::Request &req, httplib::Response &res)
              {
                std::lock_guard<std::mutex> l(g::mainloop_mtx);
                auto id = game::add_tank();
                g::userdata[g::user_id] = game::UserData{.user_id = g::user_id};
                logger::info("Client connected as ", id);
                res.body = std::to_string(id);
              });
    }
  
  void Server::start(int port)
  {
    std::thread th{
        [this, port] { svr.listen("0.0.0.0", port); }
    };
    th.detach();
  }
  
  Client::Client() {}
  
  size_t Client::connect(const std::string &addr_, int port_)
  {
    host = addr_;
    port = port_;
    httplib::Client cli(addr_ + ":" + std::to_string(port_));
    auto ret = cli.Get("register_tank");
    utils::tank_assert(ret->status == 200);
    return std::stoul(ret->body);
  }
  
  void Client::tank_react(tank::NormalTankEvent e)
  {
    httplib::Client cli(host + ":" + std::to_string(port));
    httplib::Params params{
        {"id",    std::to_string(g::user_id)},
        {"event", std::to_string(static_cast<int>(e))}
    };
    auto ret = cli.Get("tank_react", params, httplib::Headers{});
    utils::tank_assert(ret->status == 200);
  }
  
  void Client::update()
  {
    httplib::Client cli(host + ":" + std::to_string(port));
    httplib::Params params{
        {"id", std::to_string(g::user_id)},
        {"zone", serialize_zone(g::render_zone.bigger_zone(3))}
    };
    auto ret = cli.Get("update", params, httplib::Headers{});
    if(ret->status == 200)
    {
      auto s = utils::split<std::vector<std::string_view>>(ret->body, "<>");
      g::userdata[g::user_id].map_changes = deserialize_changes(std::string{s[0]});
      g::map_view = deserialize_mapview(std::string{s[1]});
      g::tanks_view = deserialize_tanksview(std::string{s[2]});
    }
    else
    {
      g::output_inited = false;
    }
  }
}