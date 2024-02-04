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
#include <cassert>
#include <map>
#include <list>

namespace czh::g
{
  bool output_inited = false;
  std::size_t screen_height = term::get_height();
  std::size_t screen_width = term::get_width();
  size_t tank_focus = 0;
  map::Zone render_zone = {-128, 128, -128, 128};
  std::mutex render_mtx;
  std::set<map::Change> render_changes{};
}

namespace czh::renderer
{
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
    if (g::game_map.has(map::Status::TANK, pos))
    {
      term::output(colorify_tank(g::game_map.at(pos).get_tank()->get_id(), "  "));
    }
    else if (g::game_map.has(map::Status::BULLET, pos))
    {
      const auto &bullets = g::game_map.at(pos).get_bullets();
      assert(!bullets.empty());
      term::output(colorify_text(bullets[0]->get_tank(), std::string{bullets[0]->get_text()}));
    }
    else
    {
      if (g::game_map.has(map::Status::WALL, pos))
      {
        term::output("\033[0;41;37m  \033[0m");
      }
      else
      {
        term::output("  ");
      }
    }
    term::output("\033[?25l");
  }
  
  // [X min, X max)   [Y min, Y max)
  map::Zone get_visible_zone(size_t id)
  {
    auto pos = game::id_at(id)->get_pos();
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
    // When the render zone moves, every point in the screen doesn't move, but its corresponding pos changes.
    // so we need to do something to get the correct changes:
    // 1.  if there's no difference in the two point in the moving direction, ignore.
    // 2.  move the map's changes to its corresponding screen position.
    auto old_zone = g::render_zone;
    switch (move)
    {
      case map::Direction::UP:
        old_zone.y_max--;
        old_zone.y_min--;
        for (int i = g::render_zone.x_min; i < g::render_zone.x_max; i++)
        {
          for (int j = g::render_zone.y_min - 1; j < g::render_zone.y_max + 1; j++)
          {
            if (!g::game_map.at(i, j + 1).is_empty() || !g::game_map.at(i, j).is_empty())
            {
              g::render_changes.insert(map::Change{map::Pos(i, j)});
              g::render_changes.insert(map::Change{map::Pos(i, j + 1)});
            }
          }
        }
        for (auto &p: g::game_map.get_changes())
        {
          if (g::render_zone.contains(p.get_pos()) || old_zone.contains(p.get_pos()))
            g::render_changes.insert(map::Change{map::Pos{p.get_pos().x, p.get_pos().y + 1}});
        }
        break;
      case map::Direction::DOWN:
        old_zone.y_max++;
        old_zone.y_min++;
        for (int i = g::render_zone.x_min; i < g::render_zone.x_max; i++)
        {
          for (int j = g::render_zone.y_min - 1; j < g::render_zone.y_max + 1; j++)
          {
            if (!g::game_map.at(i, j - 1).is_empty() || !g::game_map.at(i, j).is_empty())
            {
              g::render_changes.insert(map::Change{map::Pos(i, j)});
              g::render_changes.insert(map::Change{map::Pos(i, j - 1)});
            }
          }
        }
        for (auto &p: g::game_map.get_changes())
        {
          if (g::render_zone.contains(p.get_pos()) || old_zone.contains(p.get_pos()))
            g::render_changes.insert(map::Change{map::Pos{p.get_pos().x, p.get_pos().y - 1}});
        }
        break;
      case map::Direction::LEFT:
        old_zone.x_max++;
        old_zone.x_min++;
        for (int i = g::render_zone.x_min - 1; i < g::render_zone.x_max + 1; i++)
        {
          for (int j = g::render_zone.y_min; j < g::render_zone.y_max; j++)
          {
            if (!g::game_map.at(i - 1, j).is_empty() || !g::game_map.at(i, j).is_empty())
            {
              g::render_changes.insert(map::Change{map::Pos(i, j)});
              g::render_changes.insert(map::Change{map::Pos(i - 1, j)});
            }
          }
        }
        for (auto &p: g::game_map.get_changes())
        {
          if (g::render_zone.contains(p.get_pos()) || old_zone.contains(p.get_pos()))
            g::render_changes.insert(map::Change{map::Pos{p.get_pos().x - 1, p.get_pos().y}});
        }
        break;
      case map::Direction::RIGHT:
        old_zone.x_max--;
        old_zone.x_min--;
        for (int i = g::render_zone.x_min - 1; i < g::render_zone.x_max + 1; i++)
        {
          for (int j = g::render_zone.y_min; j < g::render_zone.y_max; j++)
          {
            if (!g::game_map.at(i + 1, j).is_empty() || !g::game_map.at(i, j).is_empty())
            {
              g::render_changes.insert(map::Change{map::Pos(i, j)});
              g::render_changes.insert(map::Change{map::Pos(i + 1, j)});
            }
          }
        }
        for (auto &p: g::game_map.get_changes())
        {
          if (g::render_zone.contains(p.get_pos()) || old_zone.contains(p.get_pos()))
            g::render_changes.insert(map::Change{map::Pos{p.get_pos().x + 1, p.get_pos().y}});
        }
        break;
      case map::Direction::END:
        for (auto &p: g::game_map.get_changes())
        {
          if (g::render_zone.contains(p.get_pos()))
            g::render_changes.insert(map::Change{p.get_pos()});
        }
        break;
    }
    g::game_map.clear_changes();
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
    auto pos = game::id_at(id)->get_pos();
    return
        ((g::render_zone.x_min + 5 > pos.x)
         || (g::render_zone.x_max - 5 <= pos.x)
         || (g::render_zone.y_min + 5 > pos.y)
         || (g::render_zone.y_max - 5 <= pos.y));
  }
  
  bool completely_out_of_zone(size_t id)
  {
    auto pos = game::id_at(id)->get_pos();
    return
        ((g::render_zone.x_min > pos.x)
         || (g::render_zone.x_max <= pos.x)
         || (g::render_zone.y_min > pos.y)
         || (g::render_zone.y_max <= pos.y));
  }
  
  void render()
  {
    std::lock_guard<std::mutex> l1(g::render_mtx);
    std::lock_guard<std::mutex> l2(g::mainloop_mtx);
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
        size_t focus = g::tank_focus;
        if (auto t = game::id_at(focus); t == nullptr || !t->is_alive())
        {
          focus = 0;
          t = game::id_at(focus);
          while (t == nullptr || !t->is_alive())
          {
            ++focus;
            if (focus >= g::tanks.size())
            {
              focus = g::tank_focus;
              break;
            }
            t = game::id_at(focus);
          }
        }
        
        auto move = map::Direction::END;
        
        if(out_of_zone(focus))
        {
          if (completely_out_of_zone(focus))
            g::output_inited = false;
          else
          {
            move = game::id_at(focus)->get_direction();
            next_zone(move);
          }
        }
        
        if (!g::output_inited)
        {
          g::render_zone = get_visible_zone(focus);
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
          get_render_changes(move);
          for (auto &p: g::render_changes)
          {
            if (g::render_zone.contains(p.get_pos()))
              update_point(p.get_pos());
          }
        }
        g::render_changes.clear();
      }
        break;
      case game::Page::TANK_STATUS:
        if (!g::output_inited)
        {
          term::clear();
          std::size_t cursor_y = 0;
          term::mvoutput({g::screen_width / 2 - 10, cursor_y++}, "Tank - by caozhanhao");
          size_t gap = 2;
          size_t id_x = gap;
          size_t name_x = id_x + gap + std::to_string((*std::max_element(g::tanks.begin(), g::tanks.end(),
                                                                         [](auto &&a, auto &&b)
                                                                         {
                                                                           return a.second->get_id() <
                                                                                  b.second->get_id();
                                                                         })).second->get_id()).size();
          auto pos_size = [](const map::Pos &p)
          {
            return std::to_string(p.x).size() + std::to_string(p.y).size() + 3;
          };
          size_t pos_x = name_x + gap + (*std::max_element(g::tanks.begin(), g::tanks.end(),
                                                           [](auto &&a, auto &&b)
                                                           {
                                                             return a.second->get_name().size() <
                                                                    b.second->get_name().size();
                                                           })).second->get_name().size();
          size_t hp_x = pos_x + gap + pos_size((*std::max_element(g::tanks.begin(), g::tanks.end(),
                                                                  [&pos_size](auto &&a, auto &&b)
                                                                  {
                                                                    return pos_size(a.second->get_pos()) <
                                                                           pos_size(b.second->get_pos());
                                                                  }
          )).second->get_pos());
          
          size_t lethality_x = hp_x + gap + std::to_string((*std::max_element(g::tanks.begin(), g::tanks.end(),
                                                                              [](auto &&a, auto &&b)
                                                                              {
                                                                                return a.second->get_hp() <
                                                                                       b.second->get_hp();
                                                                              })).second->get_hp()).size();
          
          size_t auto_tank_gap_x =
              lethality_x + gap + std::to_string((*std::max_element(g::tanks.begin(), g::tanks.end(),
                                                                    [](auto &&a, auto &&b)
                                                                    {
                                                                      return
                                                                          a.second->get_info().bullet.lethality
                                                                          <
                                                                          b.second->get_info().bullet.lethality;
                                                                    })).second->get_info().bullet.lethality).size();
          
          size_t target_x = auto_tank_gap_x + gap + 2;
          
          term::mvoutput({id_x, cursor_y}, "ID");
          term::mvoutput({name_x, cursor_y}, "Name");
          term::mvoutput({pos_x, cursor_y}, "Pos");
          term::mvoutput({hp_x, cursor_y}, "HP");
          term::mvoutput({lethality_x, cursor_y}, "ATK");
          term::mvoutput({auto_tank_gap_x, cursor_y}, "Gap");
          term::mvoutput({target_x, cursor_y}, "Target");
          
          cursor_y++;
          for (auto it = g::tanks.begin(); it != g::tanks.end(); ++it)
          {
            auto tank = it->second;
            std::string x = std::to_string(tank->get_pos().x);
            std::string y = std::to_string(tank->get_pos().y);
            term::mvoutput({id_x, cursor_y}, std::to_string(tank->get_id()));
            term::mvoutput({name_x, cursor_y}, colorify_text(tank->get_id(), tank->get_name()));
            term::mvoutput({pos_x, cursor_y}, "(" + x + "," + y + ")");
            term::mvoutput({hp_x, cursor_y}, std::to_string(tank->get_hp()));
            term::mvoutput({lethality_x, cursor_y}, std::to_string(tank->get_info().bullet.lethality));
            if (tank->is_auto())
            {
              term::mvoutput({auto_tank_gap_x, cursor_y}, std::to_string(tank->get_info().gap));
              auto at = dynamic_cast<tank::AutoTank *>(tank);
              std::string target_name;
              auto target = game::id_at(at->get_target_id());
              if (target != nullptr)
              {
                target_name = colorify_text(target->get_id(), target->get_name());
              }
              else
              {
                target_name = "Cleared";
              }
              term::mvoutput({target_x, cursor_y}, target_name);
            }
            cursor_y++;
            if (cursor_y == g::screen_height - 1)
            {
              // pos_x = name_x + gap + name_size
              // target_size = name_size
              // then offset = target_x + gap + target_size
              size_t offset = target_x + pos_x - name_x;
              id_x += offset;
              name_x += offset;
              pos_x += offset;
              hp_x += offset;
              lethality_x += offset;
              target_x += offset;
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