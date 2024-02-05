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
#include "tank/logger.h"
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
          args.emplace_back(std::stoi(temp));
        }
        else
        {
          args.emplace_back(temp);
        }
      }
    }
    return {name, args};
  }
  
  void run_command(const std::string &str)
  {
    auto invalid_arguments = []()
    {
      logger::error("Invalid arguments.");
    };
    g::curr_page = game::Page::GAME;
    auto [name, args] = parse(str);
    
      if (name == "help")
      {
        if(args_is<>(args))
        {
          g::help_page = 0;
        }
        else if(args_is<int>(args))
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
        term::flush();
        logger::info("Quitting.");
        for (auto it = g::tanks.begin(); it != g::tanks.end();)
        {
          delete it->second;
          it = g::tanks.erase(it);
        }
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
        logger::info("Filled from (", from_x, ",", from_y, ") to (", to_x, ",", to_y, ").");
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
            logger::error("Invalid tank.");
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
            logger::error("Target pos has no space.");
            return;
          }
        }
        else if (args_is<int, int, int>(args))
        {
          std::tie (id, to_pos.x, to_pos.y) = args_get<int, int, int>(args);
          if (game::id_at(id) == nullptr)
          {
            logger::error("Invalid tank.");
            return;
          }
          if (!check(to_pos))
          {
            logger::error("Target pos has no space.");
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
        logger::info(game::id_at(id)->get_name(), " has been teleported to (", to_pos.x, ",", to_pos.y, ").");
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
          logger::info("Revived all tanks.");
          return;
        }
        else if(args_is<int>(args))
        {
          std::tie(id) = args_get<int>(args);
          if (game::id_at(id) == nullptr)
          {
            logger::error("Invalid tank");
            return;
          }
        }
        else
        {
          invalid_arguments();
          return;
        }
        game::revive(id);
        logger::info(game::id_at(id)->get_name(), " revived.");
      }
      else if (name == "summon")
      {
        std::lock_guard<std::mutex> l(g::mainloop_mtx);
        int num, lvl;
        if(args_is<int, int>(args))
        {
          std::tie(num, lvl) = args_get<int, int>(args);
          if (num <= 0 || lvl > 10 || lvl < 1)
          {
            logger::error("Invalid num/lvl.");
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
        logger::info("Added ", num, " AutoTanks, Level: ", lvl, ".");
      }
      else if (name == "observe")
      {
        int id;
        if(args_is<int>(args))
        {
          std::tie(id) = args_get<int>(args);
          if (game::id_at(id) == nullptr)
          {
            logger::error("Invalid tank");
            return;
          }
        }
        else
        {
          invalid_arguments();
          return;
        }
        g::tank_focus = id;
        logger::info("Observing " + game::id_at(id)->get_name());
      }
      else if (name == "kill")
      {
        std::lock_guard<std::mutex> l(g::mainloop_mtx);
        if (args_is<>(args))
        {
          for (auto &r: g::tanks)
            if (r.second->is_alive()) r.second->kill();
          game::clear_death();
          logger::info("Killed all tanks.");
        }
        else if(args_is<int>(args))
        {
          auto [id] = args_get<int>(args);
          if (game::id_at(id) == nullptr)
          {
            logger::error("Invalid tank.");
            return;
          }
          auto t = game::id_at(id);
          t->kill();
          game::clear_death();
          logger::info(t->get_name(), " has been killed.");
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
          logger::info("Cleared all tanks.");
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
            logger::info("Cleared all died tanks.");
          }
          else
          {
            logger::error("Invalid arguments.");
            return;
          }
        }
        else if(args_is<int>(args))
        {
          auto [id] = args_get<int>(args);
          if (game::id_at(id) == nullptr || id == 0)
          {
            logger::error("Invalid tank.");
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
          logger::info("ID: ", id, " was cleared.");
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
            logger::error("Invalid tank");
            return;
          }
          if (key == "max_hp")
          {
            game::id_at(id)->get_info().max_hp = value;
            logger::info("The max_hp of ", game::id_at(id)->get_name(), " has been set for ", value, ".");
            return;
          }
          else if (key == "hp")
          {
            if (!game::id_at(id)->is_alive()) game::revive(id);
            game::id_at(id)->get_hp() = value;
            logger::info("The hp of ", game::id_at(id)->get_name(), " has been set for ", value, ".");
            return;
          }
          else if (key == "target")
          {
            if (game::id_at(value) == nullptr || !game::id_at(value)->is_alive())
            {
              logger::error("Invalid target.");
              return;
            }
            if (!game::id_at(id)->is_auto())
            {
              logger::error("Invalid auto tank.");
              return;
            }
            auto atank = dynamic_cast<tank::AutoTank *>(game::id_at(id));
            atank->target(value, game::id_at(value)->get_pos());
            logger::info("The target of ", atank->get_name(), " has been set for ", value, ".");
            return;
          }
          else
          {
            logger::error("Invalid option.");
            return;
          }
        }
        else if (args_is<int, std::string, std::string>(args))
        {
          auto [id, key, value] = args_get<int, std::string, std::string>(args);
          if (game::id_at(id) == nullptr)
          {
            logger::error("Invalid tank");
            return;
          }
          if (key == "name")
          {
            std::string old_name = game::id_at(id)->get_name();
            game::id_at(id)->get_name() = value;
            logger::info("The name of ", old_name, " has been set for '", game::id_at(id)->get_name(), "'.");
            return;
          }
          else
          {
            logger::error("Invalid option.");
            return;
          }
        }
        else if (args_is<std::string, int>(args))
        {
          auto [tickstr, time] = args_get<std::string, int>(args);
          if (tickstr != "tick")
          {
            logger::error("Invalid option.");
            return;
          }
          if (time > 0)
          {
            g::tick = std::chrono::milliseconds(time);
            logger::info("Tick has been set for ", time, ".");
            return;
          }
          else
          {
            invalid_arguments();
            return;
          }
        }
        else if (args_is<int, std::string, std::string, int>(args))
        {
          auto [id, bulletstr, key, value] = args_get<int, std::string, std::string, int>(args);
          if (game::id_at(id) == nullptr)
          {
            logger::error("Invalid tank");
            return;
          }
          if (bulletstr != "bullet")
          {
            logger::error("Invalid option.");
            return;
          }
          if (key == "hp")
          {
            game::id_at(id)->get_info().bullet.hp = value;
            logger::info("The bullet hp of ", game::id_at(id)->get_name(), " has been set for ", value, ".");
            return;
          }
          else if (key == "lethality")
          {
            game::id_at(id)->get_info().bullet.lethality = value;
            logger::info("The bullet lethality of ", game::id_at(id)->get_name(), " has been set for ", value, ".");
            return;
          }
          else if (key == "range")
          {
            game::id_at(id)->get_info().bullet.range = value;
            logger::info("The bullet range of ", game::id_at(id)->get_name(), " has been set for ", value, ".");
            return;
          }
          else
          {
            logger::error("Invalid bullet option.");
            return;
          }
        }
        else
        {
          invalid_arguments();
          return;
        }
      }
      else if(name == "server")
      {
        if(g::game_mode != game::GameMode::NATIVE)
        {
          logger::error("Invalid request.");
          return;
        }
        if(args_is<std::string, int>(args))
        {
          auto [startstr, port] = args_get<std::string, int>(args);
          if(startstr != "start")
          {
            logger::error("Invalid server option.");
            return;
          }
          else
          {
            g::online_server.start(port);
            g::game_mode = game::GameMode::SERVER;
            logger::info("Server started at ", port);
          }
        }
        else
        {
          invalid_arguments();
          return;
        }
      }
      else if(name == "connect")
      {
        if(g::game_mode != game::GameMode::NATIVE)
        {
          logger::error("Invalid request.");
          return;
        }
        if(args_is<std::string, int>(args))
        {
          auto [ip, port] = args_get<std::string, int>(args);
          g::game_mode = game::GameMode::CLIENT;
          g::user_id = g::online_client.connect(ip, port);
          g::tank_focus = g::user_id;
          g::userdata[g::user_id] = game::UserData{.user_id = g::user_id};
          g::output_inited = false;
          logger::info("Connected to ", ip, ":", port, " as ", g::user_id);
        }
        else
        {
          invalid_arguments();
          return;
        }
      }
      else
      {
        logger::error("Invalid command.");
        return;
      }
  }
}