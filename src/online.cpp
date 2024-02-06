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
#include "tank/message.h"
#include "tank/command.h"
#include "tank/game_map.h"
#include "tank/game.h"
#include "tank/globals.h"
#include "tank/renderer.h"
#include "tank/utils.h"

#include "tank/bundled/cpp-httplib/httplib.h"

#include <string>
#include <chrono>
#include <vector>

namespace czh::g
{
  online::Server online_server{};
  online::Client online_client{};
  int client_failed_attempts = 0;
  int delay = -1;
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
  
  std::string serialize_tanksview(const std::map<size_t, renderer::TankView>& view)
  {
    if(view.empty()) return "empty";
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
  
  std::map<size_t, renderer::TankView> deserialize_tanksview(const std::string &str)
  {
    if(str == "empty") return {};
    auto s1 = czh::utils::split<std::vector<std::string_view>>(str, "\n");
    std::map<size_t, renderer::TankView> ret;
    for (auto &r: s1)
    {
      auto s2 = czh::utils::split<std::vector<std::string_view>>(r, ",");
      renderer::TankView c;
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
  
  std::string serialize_mapview(const renderer::MapView& view)
  {
    if(view.view.empty()) return "empty";
    std::string ret;
    for(auto& b : view.view)
    {
      ret += std::to_string(b.second.pos.x) + "," + std::to_string(b.second.pos.y) + ","
             + std::to_string(b.second.tank_id) + "," + (b.second.text.empty() ? "empty text" : b.second.text) + "," +
             std::to_string(static_cast<int>(b.second.status)) + "\n";
    }
    return ret;
  }
  
  renderer::MapView deserialize_mapview(const std::string &str)
  {
    if(str == "empty") return {};
    auto s1 = czh::utils::split<std::vector<std::string_view>>(str, "\n");
    renderer::MapView ret;
    for (auto &r: s1)
    {
      auto s2 = czh::utils::split<std::vector<std::string_view>>(r, ",");
      czh::renderer::PointView c;
      c.pos.x = std::stoi(std::string{s2[0]});
      c.pos.y = std::stoi(std::string{s2[1]});
      c.tank_id = std::stoi(std::string{s2[2]});
      c.text = s2[3] == "empty text" ? "" : std::string{s2[3]};
      c.status = static_cast<czh::map::Status>(std::stoi(std::string{s2[4]}));
      ret.view.insert({c.pos, c});
    }
    return ret;
  }
  
  
  std::string serialize_messages(const std::vector<msg::Message>& msg)
  {
    if(msg.empty()) return "empty";
    std::string ret;
    for (auto &b: msg)
    {
      ret += std::to_string(b.from) + "," + b.content +"\n";
    }
    return ret;
  }
  
  std::vector<msg::Message> deserialize_messages(const std::string &str)
  {
    if(str == "empty") return {};
    auto s1 = czh::utils::split<std::vector<std::string_view>>(str, "\n");
    std::vector<msg::Message> ret;
    for (auto &r: s1)
    {
      auto s2 = czh::utils::split<std::vector<std::string_view>>(r, ",");
      msg::Message c;
      c.from = std::stoi(std::string{s2[0]});
      c.content = std::string{s2[1]};
      ret.emplace_back(c);
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
                auto beg  = std::chrono::steady_clock::now();
                std::lock_guard<std::mutex> l(g::mainloop_mtx);
                size_t id = std::stoi(req.get_param_value("id"));
                map::Zone zone = deserialize_zone(req.get_param_value("zone"));
                renderer::MapView map_view = renderer::extract_map(zone);
                std::set<map::Pos> changes;
                for (auto &r: g::userdata[id].map_changes)
                {
                  if (zone.contains(r))
                    changes.insert(r);
                }
                auto d = std::chrono::duration_cast<std::chrono::milliseconds>
                    (std::chrono::steady_clock::now() - beg);
                res.body = std::to_string(d.count())
                    + "<>" + serialize_changes(changes)
                    + "<>" + serialize_mapview(map_view)
                    + "<>" + serialize_tanksview(renderer::extract_tanks())
                    + "<>" + serialize_messages(g::userdata[id].messages);
                g::userdata[id].map_changes.clear();
                g::userdata[id].messages.clear();
                g::userdata[id].last_update = std::chrono::steady_clock::now();
              });
      
      svr.Get("register",
                               [](const httplib::Request &req, httplib::Response &res)
                               {
                                 std::lock_guard<std::mutex> l(g::mainloop_mtx);
                                 auto id = game::add_tank();
                                 g::userdata[g::user_id] = game::UserData{.user_id = g::user_id};
                                 msg::info(g::user_id, "Client connected as " + std::to_string(id));
                                 res.body = std::to_string(id);
                               });
      svr.Get("deregister",
              [](const httplib::Request &req, httplib::Response &res)
              {
                std::lock_guard<std::mutex> l(g::mainloop_mtx);
                int id = std::stoi(req.get_param_value("id"));
                msg::info(-1, std::to_string(id) + " disconnected.");
                g::tanks[id]->kill();
                g::tanks[id]->clear();
                delete g::tanks[id];
                g::tanks.erase(id);
                g::userdata.erase(id);
              });
      svr.Get("add_auto_tank",
              [](const httplib::Request &req, httplib::Response &res)
              {
                std::lock_guard<std::mutex> l(g::mainloop_mtx);
                int lvl = std::stoi(req.get_param_value("lvl"));
                int pos_x = std::stoi(req.get_param_value("pos_x"));
                int pos_y = std::stoi(req.get_param_value("pos_y"));
                game::add_auto_tank(lvl, {pos_x, pos_y});
              });
      svr.Get("run_command",
              [](const httplib::Request &req, httplib::Response &res)
              {
                int id = std::stoi(req.get_param_value("id"));
                std::string command = req.get_param_value("cmd");
                cmd::run_command(id, command);
              });
    }
  
  void Server::start(int port)
  {
    std::thread th{
        [this, port] { svr.listen("0.0.0.0", port); }
    };
    th.detach();
  }
  
  void Server::stop()
  {
    svr.stop();
  }
  
  Client::Client(): cli(nullptr) {}
  Client::~Client()
  {
    if(cli)
      delete cli;
  }
  std::optional<size_t> Client::connect(const std::string &addr_, int port_)
  {
    host = addr_;
    port = port_;
    cli = new httplib::Client(addr_ + ":" + std::to_string(port_));
    cli->set_keep_alive(true);
    auto ret = cli->Get("register");
    if (!ret)
    {
      msg::error(g::user_id, to_string(ret.error()));
      return std::nullopt;
    }
    else
    {
      utils::tank_assert(ret->status == 200);
      return std::stoul(ret->body);
    }
  }
  
  void Client::disconnect()
  {
    if(cli)
    {
      cli->set_keep_alive(false);
      cli->Get("deregister",
               httplib::Params{{"id", std::to_string(g::user_id)}}, httplib::Headers{});
      cli->stop();
      delete cli;
      cli = nullptr;
    }
  }
  
  int Client::tank_react(tank::NormalTankEvent e)
  {
    httplib::Params params{
        {"id",    std::to_string(g::user_id)},
        {"event", std::to_string(static_cast<int>(e))}
    };
    auto ret = cli->Get("tank_react", params, httplib::Headers{});
    if(ret) return 0;
    msg::error(g::user_id, to_string(ret.error()));
    return -1;
  }
  
  std::tuple<int, renderer::Frame> Client::update()
  {
    auto beg  = std::chrono::steady_clock::now();
    httplib::Params params{
        {"id",   std::to_string(g::user_id)},
        {"zone", serialize_zone(g::render_zone.bigger_zone(10))}
    };
    auto ret = cli->Get("update", params, httplib::Headers{});
    
    if (ret && ret->status == 200)
    {
      auto s = utils::split<std::vector<std::string_view>>(ret->body, "<>");
      auto curr_delay = std::chrono::duration_cast<std::chrono::milliseconds>
                            (std::chrono::steady_clock::now() - beg).count() - std::stoi(std::string{s[0]});
      g::delay = (g::delay + 0.1 * curr_delay) / 1.1;
      renderer::Frame f{
          .map = deserialize_mapview(std::string{s[2]}),
          .tanks = deserialize_tanksview(std::string{s[3]}),
          .changes = deserialize_changes(std::string{s[1]}),
          .zone = g::render_zone.bigger_zone(10)
      };
      auto msgs = deserialize_messages(std::string{s[4]});
      g::userdata[g::user_id].messages.insert(g::userdata[g::user_id].messages.end(),
                                              msgs.begin(), msgs.end());
      return {0, f};
    }
    g::delay = -1;
    msg::error(g::user_id, to_string(ret.error()));
    g::output_inited = false;
    return {-1, {}};
  }
  
  int Client::add_auto_tank(size_t lvl)
  {
    auto pos = game::get_available_pos();
    if (!pos.has_value())
    {
      msg::error(g::user_id, "No available space.");
      return -1;
    }
    httplib::Params params{
        {"id",    std::to_string(g::user_id)},
        {"pos_x", std::to_string(pos->x)},
        {"pos_y", std::to_string(pos->y)},
        {"lvl", std::to_string(lvl)}
    };
    auto ret = cli->Get("add_auto_tank", params, httplib::Headers{});
    if(ret) return 0;
    msg::error(g::user_id, to_string(ret.error()));
    return -1;
  }
  
  int Client::run_command(const std::string & str)
  {
    httplib::Params params{
        {"id",    std::to_string(g::user_id)},
        {"cmd", str}
    };
    auto ret = cli->Get("run_command", params, httplib::Headers{});
    if(ret) return 0;
    msg::error(g::user_id, to_string(ret.error()));
    return -1;
  }
}