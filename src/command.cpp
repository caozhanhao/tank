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
#include <list>
#include <map>

namespace czh::cmd
{
  std::tuple<std::string, std::vector<details::Arg>>
  parse(const std::string &cmd)
  {
    if (cmd.empty()) return {};
    auto it = cmd.begin() + 1; // skip '/'
    auto skip_space = [&it, &cmd] { while (std::isspace(*it) && it < cmd.end()) ++it; };
    skip_space();
    
    std::string name;
    while (!std::isspace(*it) && it < cmd.end())
    {
      name += *it++;
    }
    
    std::vector<details::Arg> args;
    while (it < cmd.end())
    {
      skip_space();
      std::string temp;
      bool is_int = true;
      while (!std::isspace(*it) && it < cmd.end())
      {
        if (!std::isdigit(*it) && *it != '+' && *it != '-') is_int = false;
        temp += *it++;
      }
      if (!temp.empty())
      {
        if (is_int)
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
          if(stoi_success)
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
    return {name, args};
  }
  
  void run_command(size_t user_id, const std::string &str)
  {
    auto invalid_arguments = [user_id]()
    {
      msg::error(user_id, "Invalid arguments.");
    };
    g::curr_page = game::Page::GAME;
    auto [name, args] = parse(str);
    if (g::game_mode == game::GameMode::CLIENT)
    {
      if (name == "fill" || name == "tp" || name == "kill" || name == "clear" || name == "summon"
          || name == "revive" || name == "set")
      {
        g::online_client.run_command(str);
        return;
      }
    }
    
    if (name == "help")
    {
      if (args_is<>(args))
      {
        g::help_page = 0;
      }
      else if (args_is<int>(args))
      {
        g::help_page = std::get<0>(args_get<int>(args));
      }
      else
      {
        invalid_arguments();
        return;
      }
      term::clear();
      g::curr_page = game::Page::HELP;
      g::output_inited = false;
    }
    else if (name == "quit")
    {
      term::move_cursor({0, g::screen_height + 1});
      term::output("\033[?25h");
      msg::info(user_id, "Quitting.");
      term::flush();
      for (auto it = g::tanks.begin(); it != g::tanks.end();)
      {
        delete it->second;
        it = g::tanks.erase(it);
      }
      if (g::game_mode == game::GameMode::CLIENT)
        g::online_client.disconnect();
      else if(g::game_mode == game::GameMode::SERVER)
        g::online_server.stop();
      std::exit(0);
    }
    else if (name == "fill")
    {
      std::lock_guard<std::mutex> l(g::mainloop_mtx);
      int from_x;
      int from_y;
      int to_x;
      int to_y;
      int is_wall = 0;
      if (args_is<int, int, int>(args))
      {
        std::tie(is_wall, from_x, from_y) = args_get<int, int, int>(args);
        to_x = from_x;
        to_y = from_y;
      }
      else if (args_is<int, int, int, int, int>(args))
      {
        std::tie(is_wall, from_x, from_y, to_x, to_y) = args_get<int, int, int, int, int>(args);
      }
      else
      {
        invalid_arguments();
        return;
      }
      
      map::Zone zone = {std::min(from_x, to_x), std::max(from_x, to_x) + 1,
                        std::min(from_y, to_y), std::max(from_y, to_y) + 1};
      
      for (int i = zone.x_min; i < zone.x_max; ++i)
      {
        for (int j = zone.y_min; j < zone.y_max; ++j)
        {
          if (g::game_map.has(map::Status::TANK, {i, j}))
          {
            if (auto t = g::game_map.at(i, j).get_tank(); t != nullptr)
              t->kill();
          }
          else if (g::game_map.has(map::Status::BULLET, {i, j}))
          {
            auto bullets = g::game_map.at(i, j).get_bullets();
            for (auto &r: bullets)
              r->kill();
          }
          game::clear_death();
        }
      }
      if (is_wall)
        g::game_map.fill(zone, map::Status::WALL);
      else
        g::game_map.fill(zone);
      msg::info(user_id, "Filled from (" + std::to_string(from_x) + ","
                         + std::to_string(from_y) + ") to (" + std::to_string(to_x) + "," + std::to_string(to_y) +
                         ").");
    }
    else if (name == "tp")
    {
      std::lock_guard<std::mutex> l(g::mainloop_mtx);
      int id = -1;
      map::Pos to_pos;
      auto check = [](const map::Pos &p)
      {
        return !g::game_map.has(map::Status::WALL, p) && !g::game_map.has(map::Status::TANK, p);
      };
      
      if (args_is<int, int>(args))
      {
        int to_id;
        std::tie(id, to_id) = args_get<int, int>(args);
        
        if (game::id_at(id) == nullptr || game::id_at(to_id) == nullptr || !game::id_at(to_id)->is_alive())
        {
          msg::error(user_id, "Invalid tank.");
          return;
        }
        auto pos = game::id_at(to_id)->get_pos();
        map::Pos pos_up(pos.x, pos.y + 1);
        map::Pos pos_down(pos.x, pos.y - 1);
        map::Pos pos_left(pos.x - 1, pos.y);
        map::Pos pos_right(pos.x + 1, pos.y);
        if (check(pos_up)) { to_pos = pos_up; }
        else if (check(pos_down)) { to_pos = pos_down; }
        else if (check(pos_left)) { to_pos = pos_left; }
        else if (check(pos_right)) { to_pos = pos_right; }
        else
        {
          msg::error(user_id, "Target pos has no space.");
          return;
        }
      }
      else if (args_is<int, int, int>(args))
      {
        std::tie(id, to_pos.x, to_pos.y) = args_get<int, int, int>(args);
        if (game::id_at(id) == nullptr)
        {
          msg::error(user_id, "Invalid tank.");
          return;
        }
        if (!check(to_pos))
        {
          msg::error(user_id, "Target pos has no space.");
          return;
        }
      }
      else
      {
        invalid_arguments();
        return;
      }
      
      g::game_map.remove_status(map::Status::TANK, game::id_at(id)->get_pos());
      g::game_map.add_tank(game::id_at(id), to_pos);
      game::id_at(id)->get_pos() = to_pos;
      msg::info(user_id, game::id_at(id)->get_name() + " was teleported to ("
                         + std::to_string(to_pos.x) + "," + std::to_string(to_pos.y) + ").");
    }
    else if (name == "revive")
    {
      std::lock_guard<std::mutex> l(g::mainloop_mtx);
      int id;
      if (args_is<>(args))
      {
        for (auto &r: g::tanks)
        {
          if (!r.second->is_alive()) game::revive(r.second->get_id());
        }
        msg::info(user_id, "Revived all tanks.");
        return;
      }
      else if (args_is<int>(args))
      {
        std::tie(id) = args_get<int>(args);
        if (game::id_at(id) == nullptr)
        {
          msg::error(user_id, "Invalid tank");
          return;
        }
      }
      else
      {
        invalid_arguments();
        return;
      }
      game::revive(id);
      msg::info(user_id, game::id_at(id)->get_name() + " revived.");
    }
    else if (name == "summon")
    {
      std::lock_guard<std::mutex> l(g::mainloop_mtx);
      int num, lvl;
      if (args_is<int, int>(args))
      {
        std::tie(num, lvl) = args_get<int, int>(args);
        if (num <= 0 || lvl > 10 || lvl < 1)
        {
          msg::error(user_id, "Invalid num/lvl.");
          return;
        }
      }
      else
      {
        invalid_arguments();
        return;
      }
      for (size_t i = 0; i < num; ++i)
      {
        game::add_auto_tank(lvl);
      }
      msg::info(user_id, "Added " + std::to_string(num) + " AutoTanks, Level: " + std::to_string(lvl) + ".");
    }
    else if (name == "observe")
    {
      int id;
      if (args_is<int>(args))
      {
        std::tie(id) = args_get<int>(args);
        if (game::id_at(id) == nullptr)
        {
          msg::error(user_id, "Invalid tank");
          return;
        }
      }
      else
      {
        invalid_arguments();
        return;
      }
      g::tank_focus = id;
      msg::info(user_id, "Observing " + game::id_at(id)->get_name());
    }
    else if (name == "kill")
    {
      std::lock_guard<std::mutex> l(g::mainloop_mtx);
      if (args_is<>(args))
      {
        for (auto &r: g::tanks)
          if (r.second->is_alive()) r.second->kill();
        game::clear_death();
        msg::info(user_id, "Killed all tanks.");
      }
      else if (args_is<int>(args))
      {
        auto [id] = args_get<int>(args);
        if (game::id_at(id) == nullptr)
        {
          msg::error(user_id, "Invalid tank.");
          return;
        }
        auto t = game::id_at(id);
        t->kill();
        game::clear_death();
        msg::info(user_id, t->get_name() + " was killed.");
      }
      else
      {
        invalid_arguments();
        return;
      }
    }
    else if (name == "clear")
    {
      std::lock_guard<std::mutex> l(g::mainloop_mtx);
      if (args_is<>(args))
      {
        for (auto &r: g::bullets)
        {
          if (game::id_at(r->get_tank())->is_auto())
            r->kill();
        }
        for (auto &r: g::tanks)
        {
          if (r.second->is_auto())
            r.second->kill();
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
      else if (args_is<std::string>(args))
      {
        auto [d] = args_get<std::string>(args);
        if (d == "death")
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
        else
        {
          msg::error(user_id, "Invalid arguments.");
          return;
        }
      }
      else if (args_is<int>(args))
      {
        auto [id] = args_get<int>(args);
        if (game::id_at(id) == nullptr || id == 0)
        {
          msg::error(user_id, "Invalid tank.");
          return;
        }
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
      else
      {
        invalid_arguments();
        return;
      }
    }
    else if (name == "set")
    {
      std::lock_guard<std::mutex> l(g::mainloop_mtx);
      if (args_is<int, std::string, int>(args))
      {
        auto [id, key, value] = args_get<int, std::string, int>(args);
        if (game::id_at(id) == nullptr)
        {
          msg::error(user_id, "Invalid tank");
          return;
        }
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
        else
        {
          msg::error(user_id, "Invalid option.");
          return;
        }
      }
      else if (args_is<int, std::string, std::string>(args))
      {
        auto [id, key, value] = args_get<int, std::string, std::string>(args);
        if (game::id_at(id) == nullptr)
        {
          msg::error(user_id, "Invalid tank");
          return;
        }
        if (key == "name")
        {
          std::string old_name = game::id_at(id)->get_name();
          game::id_at(id)->get_name() = value;
          msg::info(user_id, "The name of " + old_name + " was set to '" + game::id_at(id)->get_name() + "'.");
          return;
        }
        else
        {
          msg::error(user_id, "Invalid option.");
          return;
        }
      }
      else if (args_is<std::string, int>(args))
      {
        auto [option, arg] = args_get<std::string, int>(args);
        if (option == "tick")
        {
          if (arg > 0)
          {
            g::tick = std::chrono::milliseconds(arg);
            msg::info(user_id, "Tick was set to " + std::to_string(arg) + ".");
            return;
          }
          else
          {
            invalid_arguments();
            return;
          }
        }
        else if(option == "seed")
        {
          g::seed = arg;
          g::output_inited = false;
          msg::info(user_id, "Seed was set to " + std::to_string(arg) + ".");
        }
        else
        {
          msg::error(user_id, "Invalid option.");
          return;
        }
      }
      else if (args_is<int, std::string, std::string, int>(args))
      {
        auto [id, bulletstr, key, value] = args_get<int, std::string, std::string, int>(args);
        if (game::id_at(id) == nullptr)
        {
          msg::error(user_id, "Invalid tank");
          return;
        }
        if (bulletstr != "bullet")
        {
          msg::error(user_id, "Invalid option.");
          return;
        }
        if (key == "hp")
        {
          game::id_at(id)->get_info().bullet.hp = value;
          msg::info(user_id,
                    "The bullet hp of " + game::id_at(id)->get_name() + " was set to " + std::to_string(value) + ".");
          return;
        }
        else if (key == "lethality")
        {
          game::id_at(id)->get_info().bullet.lethality = value;
          msg::info(user_id,
                    "The bullet lethality of " + game::id_at(id)->get_name() + " was set to " + std::to_string(value) +
                    ".");
          return;
        }
        else if (key == "range")
        {
          game::id_at(id)->get_info().bullet.range = value;
          msg::info(user_id, "The bullet range of " + game::id_at(id)->get_name()
                             + " was set to " + std::to_string(value) + ".");
          return;
        }
        else
        {
          msg::error(user_id, "Invalid bullet option.");
          return;
        }
      }
      else
      {
        invalid_arguments();
        return;
      }
    }
    else if (name == "server")
    {
      std::lock_guard<std::mutex> l(g::mainloop_mtx);
      if (args_is<std::string, int>(args))
      {
        if (g::game_mode != game::GameMode::NATIVE)
        {
          msg::error(user_id, "Invalid request.");
          return;
        }
        auto [s, port] = args_get<std::string, int>(args);
        if (s != "start")
        {
          msg::error(user_id, "Invalid server option.");
          return;
        }
        else
        {
          g::online_server.start(port);
          g::game_mode = game::GameMode::SERVER;
          msg::info(user_id, "Server started at " + std::to_string(port));
        }
      }
      else if(args_is<std::string>(args))
      {
        if (g::game_mode != game::GameMode::SERVER)
        {
          msg::error(user_id, "Invalid request.");
          return;
        }
        auto [s] = args_get<std::string>(args);
        if (s != "stop")
        {
          msg::error(user_id, "Invalid server option.");
          return;
        }
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
      else
      {
        invalid_arguments();
        return;
      }
    }
    else if (name == "connect")
    {
      std::lock_guard<std::mutex> l(g::mainloop_mtx);
      if (g::game_mode != game::GameMode::NATIVE)
      {
        msg::error(user_id, "Invalid request.");
        return;
      }
      if (args_is<std::string, int>(args))
      {
        auto [ip, port] = args_get<std::string, int>(args);
        auto try_connect = g::online_client.connect(ip, port);
        if(try_connect.has_value())
        {
          g::game_mode = game::GameMode::CLIENT;
          g::user_id = *try_connect;
          g::tank_focus = g::user_id;
          g::userdata[g::user_id] = game::UserData{.user_id = g::user_id};
          g::output_inited = false;
          msg::info(user_id, "Connected to " + ip + ":" + std::to_string(port) + " as " + std::to_string(g::user_id));
        }
      }
      else
      {
        invalid_arguments();
        return;
      }
    }
    else if (name == "disconnect")
    {
      if (g::game_mode != game::GameMode::CLIENT)
      {
        msg::error(user_id, "Invalid request.");
        return;
      }
      if (args_is<>(args))
      {
        g::online_client.disconnect();
        g::game_mode = game::GameMode::NATIVE;
        g::user_id = 0;
        g::tank_focus = g::user_id;
        g::output_inited = false;
        msg::info(g::user_id, "Disconnected.");
      }
      else
      {
        invalid_arguments();
        return;
      }
    }
    else
    {
      msg::error(user_id, "Invalid command.");
      return;
    }
  }
}