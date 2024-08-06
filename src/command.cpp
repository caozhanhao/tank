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
#include "tank/globals.h"
#include "tank/game.h"
#include "tank/term.h"
#include "tank/command.h"
#include <string>
#include <vector>
#include <mutex>
#include <regex>
#include <set>

namespace czh::g
{
  const std::set<std::string> client_cmds
  {
    "fill", "tp", "kill", "clear", "summon", "revive", "set", "tell", "pause", "continue"
  };
  const std::vector<cmd::CommandInfo> commands{
    {"help", "[line]"},
    {"server", "start [port] (or stop)"},
    {"connect", "[ip] [port]"},
    {"disconnect", ""},
    {"fill", "[status] [A x,y] [B x,y optional]"},
    {"tp", "[A id] ([B id] or [B x,y])"},
    {"revive", "id"},
    {"summon", "[n] [level]"},
    {"observe", "[id]"},
    {"kill", "[id optional]"},
    {"clear", "[id optional] (or death)"},
    {"set", "[id] (bullet) [attr] [value]"},
    {"tell", "[id, optional], [msg]"},
    {"pause", ""},
    {"continue", ""},
    {"quit", ""}
  };
}

namespace czh::cmd
{
  namespace helper
  {
    bool is_ip(const std::string &s)
    {
      std::regex ipv4("^(?:(?:25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)($|(?!\\.$)\\.)){4}$");
      std::regex ipv6("^(?:(?:[\\da-fA-F]{1,4})($|(?!:$):)){8}$");
      return std::regex_search(s, ipv4) || std::regex_search(s, ipv6);
    }

    bool is_port(int p)
    {
      return p > 0 && p < 65536;
    }

    bool is_valid_id(int id)
    {
      return game::id_at(id) != nullptr;
    }

    bool is_alive_id(int id)
    {
      if (!is_valid_id(id)) return false;
      return game::id_at(id)->is_alive();
    }
  }

  CmdCall parse(const std::string &cmd)
  {
    if (cmd.empty()) return {};
    auto it = cmd.begin();
    auto skip_space = [&it, &cmd] { while (it < cmd.end() && std::isspace(*it)) ++it; };
    skip_space();

    std::string name;
    while (it < cmd.end() && !std::isspace(*it))
      name += *it++;

    std::vector<details::Arg> args;
    while (it < cmd.end())
    {
      skip_space();
      std::string temp;
      bool maybe_int = true;
      while (it < cmd.end() && !std::isspace(*it))
      {
        if (!std::isdigit(*it) && *it != '+' && *it != '-') maybe_int = false;
        temp += *it++;
      }
      if (!temp.empty())
      {
        if (maybe_int)
        {
          bool stoi_success = true;
          int a = 0;
          try
          {
            a = std::stoi(temp);
          }
          catch (...)
          {
            stoi_success = false;
          }
          if (stoi_success)
            args.emplace_back(a);
          else
            args.emplace_back(temp);
        }
        else
        {
          args.emplace_back(temp);
        }
      }
    }
    return CmdCall{.name = name, .args = args};
  }

  void run_command(size_t user_id, const std::string &str)
  {
    auto call = parse(str);
    if (g::game_mode == game::GameMode::CLIENT)
    {
      if (g::client_cmds.find(call.name) != g::client_cmds.end())
      {
        g::online_client.run_command(str);
        return;
      }
    }

    if (call.is("help"))
    {
      if (call.args.empty())
        g::help_lineno = 1;
      else if (auto v = call.get_if<int>([](int i) { return i >= 1 && i < g::help_text.size(); }); v)
      {
        int i = std::get<0>(*v);
        g::help_lineno = i;
      }
      else goto invalid_args;
      g::curr_page = game::Page::HELP;
      g::output_inited = false;
    }
    else if (call.is("quit"))
    {
      if (call.args.empty())
      {
        std::lock_guard<std::mutex> l(g::mainloop_mtx);
        term::move_cursor({0, g::screen_height + 1});
        term::output("\033[?25h");
        msg::info(user_id, "Quitting.");
        term::flush();
        game::quit();
        std::exit(0);
      }
      else goto invalid_args;
    }
    else if (call.is("pause"))
    {
      if (call.args.empty())
      {
        g::game_running = false;
        msg::info(user_id, "Stopped.");
      }
      else goto invalid_args;
    }
    else if (call.is("continue"))
    {
      if (call.args.empty())
      {
        g::game_running = true;
        msg::info(user_id, "Continuing.");
      }
      else goto invalid_args;
    }
    else if (call.is("fill"))
    {
      std::lock_guard<std::mutex> l(g::mainloop_mtx);
      int from_x;
      int from_y;
      int to_x;
      int to_y;
      int is_wall = 0;
      if (auto v = call.get_if<int, int, int>([](int w, int, int) { return w == 0 || w == 1; }); v)
      {
        std::tie(is_wall, from_x, from_y) = *v;
        to_x = from_x;
        to_y = from_y;
      }
      else if (auto v = call.get_if<int, int, int, int, int>
          ([](int w, int, int, int, int) { return w == 0 || w == 1; }); v)
      {
        std::tie(is_wall, from_x, from_y, to_x, to_y) = *v;
      }
      else goto invalid_args;

      map::Zone zone = {
        (std::min)(from_x, to_x), (std::max)(from_x, to_x) + 1,
        (std::min)(from_y, to_y), (std::max)(from_y, to_y) + 1
      };

      for (int i = zone.x_min; i < zone.x_max; ++i)
      {
        for (int j = zone.y_min; j < zone.y_max; ++j)
        {
          if (g::game_map.has(map::Status::TANK, {i, j}))
          {
            if (auto t = g::game_map.at(i, j).get_tank(); t != nullptr)
            {
              t->kill();
            }
          }
          else if (g::game_map.has(map::Status::BULLET, {i, j}))
          {
            auto bullets = g::game_map.at(i, j).get_bullets();
            for (auto &r: bullets)
            {
              r->kill();
            }
          }
          game::clear_death();
        }
      }
      if (is_wall)
      {
        g::game_map.fill(zone, map::Status::WALL);
      }
      else
      {
        g::game_map.fill(zone);
      }
      msg::info(user_id, "Filled from (" + std::to_string(from_x) + ","
                         + std::to_string(from_y) + ") to (" + std::to_string(to_x) + "," + std::to_string(to_y) +
                         ").");
    }
    else if (call.is("tp"))
    {
      std::lock_guard<std::mutex> l(g::mainloop_mtx);
      int id = -1;
      map::Pos to_pos;
      auto check = [](const map::Pos &p)
      {
        return !g::game_map.has(map::Status::WALL, p) && !g::game_map.has(map::Status::TANK, p);
      };

      if (auto v = call.get_if<int, int>(
        [](int id, int to_id) { return helper::is_alive_id(id) && helper::is_alive_id(to_id); }); v)
      {
        int to_id;
        std::tie(id, to_id) = *v;

        auto pos = game::id_at(to_id)->get_pos();
        map::Pos pos_up(pos.x, pos.y + 1);
        map::Pos pos_down(pos.x, pos.y - 1);
        map::Pos pos_left(pos.x - 1, pos.y);
        map::Pos pos_right(pos.x + 1, pos.y);
        if (check(pos_up))
          to_pos = pos_up;
        else if (check(pos_down))
          to_pos = pos_down;
        else if (check(pos_left))
          to_pos = pos_left;
        else if (check(pos_right))
          to_pos = pos_right;
        else goto invalid_args;
      }
      else if (auto v = call.get_if<int, int, int>(
        [&check](int id, int x, int y) { return helper::is_alive_id(id) && check(map::Pos(x, y)); }); v)
      {
        std::tie(id, to_pos.x, to_pos.y) = *v;
      }
      else goto invalid_args;

      g::game_map.remove_status(map::Status::TANK, game::id_at(id)->get_pos());
      g::game_map.add_tank(game::id_at(id), to_pos);
      game::id_at(id)->get_pos() = to_pos;
      msg::info(user_id, game::id_at(id)->get_name() + " was teleported to ("
                         + std::to_string(to_pos.x) + "," + std::to_string(to_pos.y) + ").");
    }
    else if (call.is("revive"))
    {
      std::lock_guard<std::mutex> l(g::mainloop_mtx);
      int id;
      if (call.args.empty())
      {
        for (auto &r: g::tanks)
        {
          if (!r.second->is_alive()) game::revive(r.second->get_id());
        }
        msg::info(user_id, "Revived all tanks.");
        return;
      }
      else if (auto v = call.get_if<int>([](int id) { return helper::is_valid_id(id); }); v)
      {
        std::tie(id) = *v;
      }
      else goto invalid_args;
      game::revive(id);
      msg::info(user_id, game::id_at(id)->get_name() + " revived.");
    }
    else if (call.is("summon"))
    {
      std::lock_guard<std::mutex> l(g::mainloop_mtx);
      int num, lvl;
      if (auto v = call.get_if<int, int>(
        [](int num, int lvl) { return num > 0 && lvl <= 10 && lvl >= 1; }); v)
      {
        std::tie(num, lvl) = *v;
      }
      else goto invalid_args;
      for (size_t i = 0; i < num; ++i)
      {
        game::add_auto_tank(lvl);
      }
      msg::info(user_id, "Added " + std::to_string(num) + " AutoTanks, Level: " + std::to_string(lvl) + ".");
    }
    else if (call.is("observe"))
    {
      int id;
      if (auto v = call.get_if<int>(
        [](int id) { return g::snapshot.tanks.find(id) != g::snapshot.tanks.end(); }); v)
      {
        std::tie(id) = *v;
      }
      else goto invalid_args;
      g::tank_focus = id;
      msg::info(user_id, "Observing " + g::snapshot.tanks[id].info.name);
    }
    else if (call.is("kill"))
    {
      std::lock_guard<std::mutex> l(g::mainloop_mtx);
      if (call.args.empty())
      {
        for (auto &r: g::tanks)
        {
          if (r.second->is_alive()) r.second->kill();
        }
        game::clear_death();
        msg::info(user_id, "Killed all tanks.");
      }
      else if (auto v = call.get_if<int>([](int id) { return helper::is_valid_id(id); }); v)
      {
        auto [id] = *v;
        auto t = game::id_at(id);
        t->kill();
        game::clear_death();
        msg::info(user_id, t->get_name() + " was killed.");
      }
      else goto invalid_args;
    }
    else if (call.is("clear"))
    {
      std::lock_guard<std::mutex> l(g::mainloop_mtx);
      if (call.args.empty())
      {
        for (auto &r: g::bullets)
        {
          if (game::id_at(r->get_tank())->is_auto())
          {
            r->kill();
          }
        }
        for (auto &r: g::tanks)
        {
          if (r.second->is_auto())
          {
            r.second->kill();
          }
        }
        game::clear_death();
        for (auto it = g::tanks.begin(); it != g::tanks.end();)
        {
          if (!it->second->is_auto())
          {
            ++it;
          }
          else
          {
            delete it->second;
            it = g::tanks.erase(it);
          }
        }
        msg::info(user_id, "Cleared all tanks.");
      }
      else if (auto v = call.get_if<std::string>(
        [](std::string f) { return f == "death"; }); v)
      {
        for (auto &r: g::bullets)
        {
          auto t = game::id_at(r->get_tank());
          if (t->is_auto() && !t->is_alive())
          {
            r->kill();
          }
        }
        for (auto &r: g::tanks)
        {
          if (r.second->is_auto() && !r.second->is_alive())
          {
            r.second->kill();
          }
        }
        game::clear_death();
        for (auto it = g::tanks.begin(); it != g::tanks.end();)
        {
          if (!it->second->is_auto() || it->second->is_alive())
          {
            ++it;
          }
          else
          {
            delete it->second;
            it = g::tanks.erase(it);
          }
        }
        msg::info(user_id, "Cleared all died tanks.");
      }
      else if (auto v = call.get_if<int>([](int id) { return helper::is_valid_id(id) || id != 0; }); v)
      {
        auto [id] = *v;
        for (auto &r: g::bullets)
        {
          if (r->get_tank() == id)
          {
            r->kill();
          }
        }
        auto t = game::id_at(id);
        t->kill();
        delete t;
        g::tanks.erase(id);
        game::clear_death();
        msg::info(user_id, "ID: " + std::to_string(id) + " was cleared.");
      }
      else goto invalid_args;
    }
    else if (call.is("set"))
    {
      std::lock_guard<std::mutex> l(g::mainloop_mtx);
      if (auto v = call.get_if<int, std::string, int>(
        [](int id, std::string key, int value)
        {
          return helper::is_valid_id(id) &&
                 (key == "max_hp" || key == "hp" || key == "target");
        }); v)
      {
        auto [id, key, value] = *v;
        if (key == "max_hp")
        {
          game::id_at(id)->get_info().max_hp = value;
          msg::info(user_id, "The max_hp of " + game::id_at(id)->get_name()
                             + " was set to " + std::to_string(value) + ".");
          return;
        }
        else if (key == "hp")
        {
          if (!game::id_at(id)->is_alive()) game::revive(id);
          game::id_at(id)->get_hp() = value;
          msg::info(user_id, "The hp of " + game::id_at(id)->get_name()
                             + " was set to " + std::to_string(value) + ".");
          return;
        }
        else if (key == "target")
        {
          if (game::id_at(value) == nullptr || !game::id_at(value)->is_alive())
          {
            msg::error(user_id, "Invalid target.");
            return;
          }
          if (!game::id_at(id)->is_auto())
          {
            msg::error(user_id, "Invalid auto tank.");
            return;
          }
          auto atank = dynamic_cast<tank::AutoTank *>(game::id_at(id));
          atank->target(value, game::id_at(value)->get_pos());
          msg::info(user_id, "The target of " + atank->get_name() + " was set to " + std::to_string(value) + ".");
          return;
        }
      }
      else if (auto v = call.get_if<int, std::string, std::string>(
        [](int id, std::string key, std::string value) { return helper::is_valid_id(id) && key == "name"; }); v)
      {
        auto [id, key, value] = *v;
        if (key == "name")
        {
          std::string old_name = game::id_at(id)->get_name();
          game::id_at(id)->get_name() = value;
          msg::info(user_id, "The name of " + old_name + " was set to '" + game::id_at(id)->get_name() + "'.");
          return;
        }
      }
      else if (auto v = call.get_if<std::string, int>(
        [](std::string key, int arg)
        {
          return (key == "tick" && arg > 0)
                 || (key == "seed")
                 || (key == "msg_ttl" && arg > 0);
        }); v)
      {
        auto [option, arg] = *v;
        if (option == "tick")
        {
          g::tick = std::chrono::milliseconds(arg);
          msg::info(user_id, "Tick was set to " + std::to_string(arg) + ".");
        }
        else if (option == "seed")
        {
          g::seed = arg;
          g::output_inited = false;
          msg::info(user_id, "Seed was set to " + std::to_string(arg) + ".");
        }
        else if (option == "msg_ttl")
        {
          g::msg_ttl = std::chrono::milliseconds(arg);
          msg::info(user_id, "Msg_ttl was set to " + std::to_string(arg) + ".");
        }
      }
      else if (auto v = call.get_if<int, std::string, std::string, int>(
        [](int id, std::string f, std::string key, int value)
        {
          return helper::is_valid_id(id) && f == "bullet"
                 && (key == "hp" || key == "lethality" || key == "range");
        }); v)
      {
        auto [id, bulletstr, key, value] = *v;
        if (key == "hp")
        {
          game::id_at(id)->get_info().bullet.hp = value;
          msg::info(user_id,
                    "The bullet hp of " + game::id_at(id)->get_name() + " was set to " + std::to_string(value) + ".");
        }
        else if (key == "lethality")
        {
          game::id_at(id)->get_info().bullet.lethality = value;
          msg::info(user_id,
                    "The bullet lethality of " + game::id_at(id)->get_name() + " was set to " + std::to_string(value) +
                    ".");
        }
        else if (key == "range")
        {
          game::id_at(id)->get_info().bullet.range = value;
          msg::info(user_id, "The bullet range of " + game::id_at(id)->get_name()
                             + " was set to " + std::to_string(value) + ".");
        }
      }
      else goto invalid_args;
    }
    else if (call.is("server"))
    {
      std::lock_guard<std::mutex> l(g::mainloop_mtx);
      if (auto v = call.get_if<std::string, int>(
        [](std::string key, int port)
        {
          return g::game_mode == game::GameMode::NATIVE && key == "start" && helper::is_port(port);
        }); v)
      {
        auto [s, port] = *v;
        g::online_server.init();
        g::online_server.start(port);
        g::game_mode = game::GameMode::SERVER;
        msg::info(user_id, "Server started at " + std::to_string(port));
      }
      else if (auto v = call.get_if<std::string>(
        [](std::string key)
        {
          return g::game_mode == game::GameMode::SERVER && key == "stop";
        }); v)
      {
        g::online_server.stop();
        for (auto &r: g::userdata)
        {
          if (r.first == 0) continue;
          g::tanks[r.first]->kill();
          g::tanks[r.first]->clear();
          delete g::tanks[r.first];
          g::tanks.erase(r.first);
        }
        g::userdata = {{0, g::userdata[0]}};
        g::game_mode = game::GameMode::NATIVE;
        msg::info(user_id, "Server stopped");
      }
      else goto invalid_args;
    }
    else if (call.is("connect"))
    {
      std::lock_guard<std::mutex> l(g::mainloop_mtx);
      if (auto v = call.get_if<std::string, int>(
        [](std::string ip, int port)
        {
          return g::game_mode == game::GameMode::NATIVE && helper::is_ip(ip) && helper::is_port(port);
        }); v)
      {
        auto [ip, port] = *v;
        g::online_client.init();
        auto try_connect = g::online_client.connect(ip, port);
        if (try_connect.has_value())
        {
          g::game_mode = game::GameMode::CLIENT;
          g::user_id = *try_connect;
          g::tank_focus = g::user_id;
          g::userdata[g::user_id] = g::UserData{.user_id = g::user_id};
          g::output_inited = false;
          msg::info(user_id, "Connected to " + ip + ":" + std::to_string(port) + " as " + std::to_string(g::user_id));
        }
      }
      else goto invalid_args;
    }
    else if (call.is("disconnect"))
    {
      if (g::game_mode == game::GameMode::CLIENT && call.args.empty())
      {
        g::online_client.disconnect();
        g::game_mode = game::GameMode::NATIVE;
        g::user_id = 0;
        g::tank_focus = g::user_id;
        g::output_inited = false;
        msg::info(g::user_id, "Disconnected.");
      }
      else goto invalid_args;
    }
    else if (call.is("tell"))
    {
      int id = -1;
      std::string msg;
      if (auto v = call.get_if<int, std::string>(
        [](int id, std::string msg) { return helper::is_valid_id(id); }); v)
      {
        std::tie(id, msg) = *v;
      }
      else if (auto v = call.get_if<std::string>([](std::string msg) { return true; }); v)
      {
        std::tie(msg) = *v;
      }
      else goto invalid_args;
      int ret = msg::send_message(user_id, id, msg);
      if (ret == 0)
        msg::info(user_id, "Message sent.");
      else
        msg::info(user_id, "Failed sending message.");
    }
    else
    {
      msg::error(user_id, "Invalid command.");
      return;
    }

    return;
  invalid_args:
    msg::error(user_id, "Invalid arguments.");
    return;
  }
}
