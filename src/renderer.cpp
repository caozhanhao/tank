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
#include "tank/game.h"
#include "tank/tank.h"
#include "tank/bullet.h"
#include "tank/utils.h"
#include "tank/term.h"
#include "tank/globals.h"
#include <mutex>
#include <vector>
#include <string>
#include <list>

namespace czh::g
{
  bool output_inited = false;
  std::size_t screen_height = term::get_height();
  std::size_t screen_width = term::get_width();
  size_t tank_focus = 0;
  map::Zone render_zone = {-128, 128, -128, 128};
  std::set<map::Pos> render_changes{};
  map::MapView map_view{};
  std::map<size_t, game::TankView> tanks_view{};
}

namespace czh::renderer
{
  std::optional<game::TankView> view_id_at(size_t id)
  {
    auto it = g::tanks_view.find(id);
    if(it == g::tanks_view.end()) return std::nullopt;
    return it->second;
  }
  std::string colorify_text(int id, std::string str)
  {
    std::string ret = "\033[0;";
    ret += std::to_string(id % 6 + 32);
    ret += "m";
    ret += str;
    ret += "\033[0m";
    return ret;
  }
  
  std::string colorify_tank(int id, std::string str)
  {
    std::string ret = "\033[0;";
    ret += std::to_string(id % 6 + 42);
    ret += ";36m";
    ret += str + "\033[0m";
    return ret;
  }
  
  void update_point(const map::Pos &pos)
  {
    term::move_cursor({static_cast<size_t>((pos.x - g::render_zone.x_min) * 2),
                       static_cast<size_t>(g::render_zone.y_max - pos.y - 1)});
    switch (g::map_view.at(pos).status)
    {
      case map::Status::TANK:
        term::output(colorify_tank(g::map_view.at(pos).tank_id, "  "));
        break;
      case map::Status::BULLET:
        term::output(colorify_text(g::map_view.at(pos).tank_id, g::map_view.at(pos).text));
        break;
      case map::Status::WALL:
        term::output("\033[0;41;37m  \033[0m");
        break;
      case map::Status::END:
        term::output("  ");
        break;
    }
    term::output("\033[?25l");
  }
  
  // [X min, X max)   [Y min, Y max)
  map::Zone get_visible_zone(size_t id)
  {
    auto pos = view_id_at(id)->pos;
    int offset_x = (int) g::screen_width / 4;
    int x_min = pos.x - offset_x;
    int x_max = (int) g::screen_width / 2 + x_min;
    
    int offset_y = (int) g::screen_height / 2;
    int y_min = pos.y - offset_y;
    int y_max = (int) g::screen_height - 1 + y_min;
    return {x_min, x_max, y_min, y_max};
  }
  
  void get_render_changes(const map::Direction& move)
  {
    // When the render zone moves, every point in the screen doesn't move, but its corresponding pos map_changes.
    // so we need to do something to get the correct map_changes:
    // 1.  if there's no difference in the two point in the moving direction, ignore.
    // 2.  move the map's map_changes to its corresponding screen position.
    auto zone = g::render_zone.bigger_zone(2);
    auto& map_changes = g::userdata[g::user_id].map_changes;
    zone.x_min--;
    zone.x_max++;
    zone.y_min--;
    zone.y_max++;
    switch (move)
    {
      case map::Direction::UP:
        for (int i = g::render_zone.x_min; i < g::render_zone.x_max; i++)
        {
          for (int j = g::render_zone.y_min - 1; j < g::render_zone.y_max + 1; j++)
          {
            if (!g::map_view.at(i, j + 1).is_empty() || !g::map_view.at(i, j).is_empty())
            {
              g::render_changes.insert(map::Pos(i, j));
              g::render_changes.insert(map::Pos(i, j + 1));
            }
          }
        }
        for (auto &p: map_changes)
        {
          if (zone.contains(p))
            g::render_changes.insert(map::Pos{p.x, p.y + 1});
        }
        break;
      case map::Direction::DOWN:
        for (int i = g::render_zone.x_min; i < g::render_zone.x_max; i++)
        {
          for (int j = g::render_zone.y_min - 1; j < g::render_zone.y_max + 1; j++)
          {
            if (!g::map_view.at(i, j - 1).is_empty() || !g::map_view.at(i, j).is_empty())
            {
              g::render_changes.insert(map::Pos(i, j));
              g::render_changes.insert(map::Pos(i, j - 1));
            }
          }
        }
        for (auto &p: map_changes)
        {
          if (zone.contains(p))
            g::render_changes.insert(map::Pos{p.x, p.y - 1});
        }
        break;
      case map::Direction::LEFT:
        for (int i = g::render_zone.x_min - 1; i < g::render_zone.x_max + 1; i++)
        {
          for (int j = g::render_zone.y_min; j < g::render_zone.y_max; j++)
          {
            if (!g::map_view.at(i - 1, j).is_empty() || !g::map_view.at(i, j).is_empty())
            {
              g::render_changes.insert(map::Pos(i, j));
              g::render_changes.insert(map::Pos(i - 1, j));
            }
          }
        }
        for (auto &p: map_changes)
        {
          if (zone.contains(p))
            g::render_changes.insert(map::Pos{p.x - 1, p.y});
        }
        break;
      case map::Direction::RIGHT:
        for (int i = g::render_zone.x_min - 1; i < g::render_zone.x_max + 1; i++)
        {
          for (int j = g::render_zone.y_min; j < g::render_zone.y_max; j++)
          {
            if (!g::map_view.at(i + 1, j).is_empty() || !g::map_view.at(i, j).is_empty())
            {
              g::render_changes.insert(map::Pos(i, j));
              g::render_changes.insert(map::Pos(i + 1, j));
            }
          }
        }
        for (auto &p: map_changes)
        {
          if (zone.contains(p))
            g::render_changes.insert(map::Pos{p.x + 1, p.y});
        }
        break;
      case map::Direction::END:
        for (auto &p: map_changes)
        {
          if (zone.contains(p))
            g::render_changes.insert(p);
        }
        break;
    }
    map_changes.clear();
  }
  
  void next_zone(const map::Direction& direction)
  {
    switch (direction)
    {
      case map::Direction::UP:
        g::render_zone.y_max++;
        g::render_zone.y_min++;
        break;
      case map::Direction::DOWN:
        g::render_zone.y_max--;
        g::render_zone.y_min--;
        break;
      case map::Direction::LEFT:
        g::render_zone.x_max--;
        g::render_zone.x_min--;
        break;
      case map::Direction::RIGHT:
        g::render_zone.x_max++;
        g::render_zone.x_min++;
        break;
    }
  }
  
  bool out_of_zone(size_t id)
  {
    auto pos = view_id_at(id)->pos;
    return
        ((g::render_zone.x_min + 5 > pos.x)
         || (g::render_zone.x_max - 5 <= pos.x)
         || (g::render_zone.y_min + 5 > pos.y)
         || (g::render_zone.y_max - 5 <= pos.y));
  }
  
  bool completely_out_of_zone(size_t id)
  {
    auto pos = view_id_at(id)->pos;
    return
        ((g::render_zone.x_min > pos.x)
         || (g::render_zone.x_max <= pos.x)
         || (g::render_zone.y_min > pos.y)
         || (g::render_zone.y_max <= pos.y));
  }
  
  void render()
  {
    std::lock_guard<std::mutex> l(g::mainloop_mtx);
    if (g::screen_height != term::get_height() || g::screen_width != term::get_width())
    {
      term::clear();
      g::help_page = 1;
      g::output_inited = false;
      g::screen_height = term::get_height();
      g::screen_width = term::get_width();
    }
    
    switch (g::curr_page)
    {
      case game::Page::GAME:
      {
        if (g::game_mode == czh::game::GameMode::SERVER || g::game_mode == czh::game::GameMode::NATIVE)
          g::tanks_view = game::extract_tanks();
        
        size_t focus = g::tank_focus;
        if (auto t = view_id_at(focus); t == std::nullopt || !t->is_alive)
        {
          focus = 0;
          t = view_id_at(focus);
          while (t == std::nullopt || !t->is_alive)
          {
            ++focus;
            if (focus >= g::tanks_view.size())
            {
              focus = g::tank_focus;
              break;
            }
            t = view_id_at(focus);
          }
        }
        
        auto move = map::Direction::END;
        
        if(out_of_zone(focus))
        {
          if (completely_out_of_zone(focus))
            g::output_inited = false;
          else
          {
            move = view_id_at(focus)->direction;
            next_zone(move);
          }
        }
        
        if (!g::output_inited)
        {
          g::render_zone = get_visible_zone(focus);
          if (g::game_mode == czh::game::GameMode::SERVER || g::game_mode == czh::game::GameMode::NATIVE)
            g::map_view = g::game_map.extract(g::render_zone.bigger_zone(2));
          else
            g::online_client.update();
          
          term::move_cursor({0, 0});
          for (int j = g::render_zone.y_max - 1; j >= g::render_zone.y_min; j--)
          {
            for (int i = g::render_zone.x_min; i < g::render_zone.x_max; i++)
            {
              update_point(map::Pos(i, j));
            }
            term::output("\n");
          }
          g::output_inited = true;
        }
        else
        {
          if (g::game_mode == czh::game::GameMode::SERVER || g::game_mode == czh::game::GameMode::NATIVE)
            g::map_view = g::game_map.extract(g::render_zone.bigger_zone(2));
          get_render_changes(move);
          for (auto &p: g::render_changes)
          {
            if (g::render_zone.contains(p))
              update_point(p);
          }
        }
        g::render_changes.clear();
        
        // msg
        if (!g::userdata[g::user_id].messages.empty())
        {
          term::move_cursor(term::TermPos(0, term::get_height() - 1));
          auto &msg = g::userdata[g::user_id].messages.back();
          std::string str = (msg.from == -1) ? msg.content : "From " + std::to_string(msg.from) + ": " + msg.content;
          int a = g::screen_width - str.size();
          if (a > 0)
          {
            term::output(str + std::string(a, ' '));
          }
          else
          {
            term::output(str.substr(0, g::screen_width));
          }
          g::userdata[g::user_id].messages.clear();
        }
      }
        break;
      case game::Page::TANK_STATUS:
        if (!g::output_inited)
        {
          term::clear();
          std::size_t cursor_y = 0;
          term::mvoutput({g::screen_width / 2 - 10, cursor_y++}, "Tank");
          size_t gap = 2;
          size_t id_x = gap;
          size_t name_x = id_x + gap + std::to_string((*std::max_element(g::tanks_view.begin(), g::tanks_view.end(),
                                                                         [](auto &&a, auto &&b)
                                                                         {
                                                                           return a.second.info.id <
                                                                                  b.second.info.id;
                                                                         })).second.info.id).size();
          auto pos_size = [](const map::Pos &p)
          {
            return std::to_string(p.x).size() + std::to_string(p.y).size() + 3;
          };
          size_t pos_x = name_x + gap + (*std::max_element(g::tanks_view.begin(), g::tanks_view.end(),
                                                           [](auto &&a, auto &&b)
                                                           {
                                                             return a.second.info.name.size() <
                                                                    b.second.info.name.size();
                                                           })).second.info.name.size();
          size_t hp_x = pos_x + gap + pos_size((*std::max_element(g::tanks_view.begin(), g::tanks_view.end(),
                                                                  [&pos_size](auto &&a, auto &&b)
                                                                  {
                                                                    return pos_size(a.second.pos) <
                                                                           pos_size(b.second.pos);
                                                                  }
          )).second.pos);
          
          size_t lethality_x =
              hp_x + gap + std::to_string((*std::max_element(g::tanks_view.begin(), g::tanks_view.end(),
                                                             [](auto &&a, auto &&b)
                                                             {
                                                               return a.second.hp <
                                                                      b.second.hp;
                                                             })).second.hp).size();
          
          size_t auto_tank_gap_x =
              lethality_x + gap + std::to_string((*std::max_element(g::tanks_view.begin(), g::tanks_view.end(),
                                                                    [](auto &&a, auto &&b)
                                                                    {
                                                                      return
                                                                          a.second.info.bullet.lethality
                                                                          <
                                                                          b.second.info.bullet.lethality;
                                                                    })).second.info.bullet.lethality).size();
          
          
          term::mvoutput({id_x, cursor_y}, "ID");
          term::mvoutput({name_x, cursor_y}, "Name");
          term::mvoutput({pos_x, cursor_y}, "Pos");
          term::mvoutput({hp_x, cursor_y}, "HP");
          term::mvoutput({lethality_x, cursor_y}, "ATK");
          term::mvoutput({auto_tank_gap_x, cursor_y}, "Gap");
          cursor_y++;
          for (auto it = g::tanks_view.begin(); it != g::tanks_view.end(); ++it)
          {
            auto tank = it->second;
            std::string x = std::to_string(tank.pos.x);
            std::string y = std::to_string(tank.pos.y);
            term::mvoutput({id_x, cursor_y}, std::to_string(tank.info.id));
            term::mvoutput({name_x, cursor_y}, colorify_text(tank.info.id, tank.info.name));
            term::mvoutput({pos_x, cursor_y}, "(" + x + "," + y + ")");
            term::mvoutput({hp_x, cursor_y}, std::to_string(tank.hp));
            term::mvoutput({lethality_x, cursor_y}, std::to_string(tank.info.bullet.lethality));
            if(tank.is_auto)
              term::mvoutput({auto_tank_gap_x, cursor_y}, std::to_string(tank.info.gap));
            else
              term::mvoutput({auto_tank_gap_x, cursor_y}, "-");
              
            cursor_y++;
            if (cursor_y == g::screen_height - 1)
            {
              size_t offset = pos_x - name_x;
              id_x += offset;
              name_x += offset;
              pos_x += offset;
              hp_x += offset;
              lethality_x += offset;
              cursor_y = 1;
            }
          }
          g::output_inited = true;
        }
        break;
      case game::Page::MAIN:
      {
        if (!g::output_inited)
        {
          constexpr std::string_view tank = R"(
 _____           _
|_   _|_ _ _ __ | | __
  | |/ _` | '_ \| |/ /
  | | (_| | | | |   <
  |_|\__,_|_| |_|_|\_\
)";
          static const auto splitted = utils::split<std::vector<std::string_view>>(tank, "\n");
          auto s = utils::fit_to_screen(splitted, g::screen_width);
          size_t x = g::screen_width / 2 - 12;
          size_t y = 2;
          term::clear();
          for (size_t i = 0; i < s.size(); ++i)
          {
            term::mvoutput({x, y++}, std::string(s[i]));
          }
          term::mvoutput({x + 5, y + 3}, ">>> Enter <<<");
          term::mvoutput({x + 1, y + 4}, "Use '/help' to get help.");
          g::output_inited = true;
        }
      }
        break;
      case game::Page::HELP:
        static const std::string help =
            R"(
Keys:
  Move: WASD
  Attack: space
  All g::tanks' status: 'o' or 'O'
  Command: '/'

Rules:
  User's Tank:
    HP: 10000, Lethality: 100
  Auto Tank:
    HP: (11 - level) * 150, Lethality: (11 - level) * 15
    The higher level the tank is, the faster it moves.
    
Command:
  help [page]
    - Get this help.
    - Use 'Enter' to return game.

  quit
    - Quit Tank.
  
  reshape [width, height]
    - Reshape the game map to the given size.

  clear_maze
    - Clear all the walls in the game map.

  fill [Status] [A x,y] [B x,y]
    - Status: [0] Empty [1] Wall
    - Fill the area from A to B as the given Status.
    - B defaults to the same as A
    - e.g.  fill 1 0 0 10 10

  tp [A id] [B id](or [B x,y])
    - Teleport A to B
    - A should be alive, and there should be space around B.
    - e.g.  tp 0 1   |  tp 0 1 1
    
  revive [A id]
    - Revive A.
    - Default to revive all g::tanks.
    
  summon [n] [level]
    - Summon n g::tanks with the given level.
    - e.g. summon 50 10
    
  kill [A id]
    - Kill A.
    - Default to kill all g::tanks.

  clear [A id]
    - Clear A.(auto tank only)
    - Default to clear all auto g::tanks.
  clear death
    - Clear all the died auto g::tanks
    Note:
       Clear is to delete rather than to kill.
       So can't clear the user's tank and cleared g::tanks can't be revived.
       And the g::bullets  of the cleared tank will also be cleared.

  set [A id] [key] [value]
    - Set A's attribute below:
      - max_hp (int): Max hp of A. This will take effect when A is revived.
      - hp (int): hp of A. This takes effect immediately but won't last when A is revived.
      - target (id, int): Auto Tank's target. Target should be alive.
      - name (string): Name of A.
  set [A id] bullet [key] [value]
      - hp (int): hp of A's bullet.
      - Whe a bullet hits the wall, its hp decreases by one. That means it will bounce hp times.
      - lethality (int): Lethality of A's bullet. This can be a negative number, in which case hp will be added.
      - range (int): Range of A's bullet.
      - e.g. set 0 max_hp 1000  |  set 0 bullet lethality 10
  set tick [value]
      - tick (int, milliseconds): time of the game's mainloop.
)";
        static auto splitted = utils::split<std::vector<std::string_view>>(help, "\n");
        {
          auto s = utils::fit_to_screen(splitted, g::screen_width);
          size_t page_size = term::get_height() - 3;
          if (!g::output_inited)
          {
            std::size_t cursor_y = 0;
            term::mvoutput({g::screen_width / 2 - 10, cursor_y++}, "Tank - by caozhanhao");
            if ((g::help_page - 1) * page_size > s.size()) g::help_page = 1;
            for (size_t i = (g::help_page - 1) * page_size; i < std::min(g::help_page * page_size, s.size()); ++i)
            {
              term::mvoutput({0, cursor_y++}, std::string(s[i]));
            }
            term::mvoutput({g::screen_width / 2 - 3, cursor_y}, "Page " + std::to_string(g::help_page));
            g::output_inited = true;
          }
        }
        break;
      case game::Page::COMMAND:
        if (!g::output_inited)
        {
          term::move_cursor({0, g::screen_height - 1});
          if (int a = g::screen_width - g::cmd_string.size(); a > 0)
          {
            term::output(g::cmd_string + std::string(a, ' ') + "\033[?25h");
          }
          else
          {
            term::output(g::cmd_string + "\033[?25h");
          }
          term::mvoutput({g::cmd_string_pos, g::screen_height - 1},
                         g::cmd_string.substr(g::cmd_string_pos, 1));
          g::output_inited = true;
        }
        break;
    }
    term::flush();
  }
}