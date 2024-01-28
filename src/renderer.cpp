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
#include <mutex>
#include <vector>
#include <string>
#include <cassert>
#include <map>
#include <list>

extern bool czh::game::output_inited;
extern czh::game::Zone czh::game::rendered_zone;
extern std::mutex czh::game::mainloop_mtx;
extern std::mutex czh::game::render_mtx;
extern czh::map::Map czh::game::game_map;
extern czh::game::Page czh::game::curr_page;
extern std::map<size_t, czh::tank::Tank *> czh::game::tanks;
extern std::list<czh::bullet::Bullet *> czh::game::bullets;
extern std::vector<std::string> czh::game::history;
extern std::string czh::game::cmd_string;
extern size_t czh::game::history_pos;
extern size_t czh::game::cmd_string_pos;
extern czh::game::Page czh::game::curr_page;
extern size_t czh::game::help_page;
extern std::size_t czh::game::screen_height;
extern std::size_t czh::game::screen_width;

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
    term::move_cursor({static_cast<size_t>((pos.x - game::rendered_zone.x_min) * 2),
                       static_cast<size_t>(game::rendered_zone.y_max - pos.y - 1)});
    if (game::game_map.has(map::Status::TANK, pos))
    {
      term::output(colorify_tank(game::game_map.at(pos).get_tank_instance()->get_id(), "  "));
    }
    else if (game::game_map.has(map::Status::BULLET, pos))
    {
      const auto &bullets = game::game_map.at(pos).get_bullets_instance();
      assert(!bullets.empty());
      term::output(colorify_text(bullets[0]->get_tank(), std::string{bullets[0]->get_text()}));
    }
    else
    {
      std::string s = "  ";
//      if(std::abs(pos.x) % 10 == 0)
//      {
//          s = "||";
//      }
//      if(std::abs(pos.y) % 10 == 0)
//      {
//        if (s == "||")
//          s = "<>";
//        else
//          s = "==";
//      }
      if (game::game_map.has(map::Status::WALL, pos))
      {
        term::output("\033[0;41;37m" + s + "\033[0m");
      }
      else
      {
        term::output(s);
      }
    }
    term::output("\033[?25l");
  }
  
  // [X min, X max)   [Y min, Y max)
  game::Zone get_visible_zone(size_t id)
  {
    auto pos = game::id_at(id)->get_pos();
    int offset_x = (int) game::screen_width / 4;
    int x_min = pos.x - offset_x;
    int x_max = (int) game::screen_width / 2 + x_min;
    
    int offset_y = (int) game::screen_height / 2;
    int y_min = pos.y - offset_y;
    int y_max = (int) game::screen_height - 1 + y_min;
    return {x_min, x_max, y_min, y_max};
  }
  
  game::Zone next_zone(size_t id)
  {
    auto ret = game::rendered_zone;
    auto direction = game::id_at(id)->get_direction();
    switch (direction)
    {
      case map::Direction::UP:
        ret.y_max++;
        ret.y_min++;
        break;
      case map::Direction::DOWN:
        ret.y_max--;
        ret.y_min--;
        break;
      case map::Direction::LEFT:
        ret.x_max--;
        ret.x_min--;
        break;
      case map::Direction::RIGHT:
        ret.x_max++;
        ret.x_min++;
        break;
      default:
        ret = get_visible_zone(id);
        break;
    }
    return ret;
  }
  
  bool out_of_zone(size_t id)
  {
    auto pos = game::id_at(id)->get_pos();
    return
        ((game::rendered_zone.x_min + 10 > pos.x)
         || (game::rendered_zone.x_max - 10 <= pos.x)
         || (game::rendered_zone.y_min + 10 > pos.y)
         || (game::rendered_zone.y_max - 10 <= pos.y));
  }
  
  bool completely_out_of_zone(size_t id)
  {
    auto pos = game::id_at(id)->get_pos();
    return
        ((game::rendered_zone.x_min > pos.x)
         || (game::rendered_zone.x_max <= pos.x)
         || (game::rendered_zone.y_min > pos.y)
         || (game::rendered_zone.y_max <= pos.y));
  }
  
  void render()
  {
    std::lock_guard<std::mutex> l1(game::render_mtx);
    std::lock_guard<std::mutex> l2(game::mainloop_mtx);
    if (game::screen_height != term::get_height() || game::screen_width != term::get_width() || game::map_size_changed)
    {
      term::clear();
      game::help_page = 1;
      game::output_inited = false;
      game::map_size_changed = false;
      game::screen_height = term::get_height();
      game::screen_width = term::get_width();
    }
    switch (game::curr_page)
    {
      case game::Page::GAME:
      {
        size_t focus = game::tank_focus;
        if (auto t = game::id_at(focus); t == nullptr || !t->is_alive())
        {
          focus = 0;
          t = game::id_at(focus);
          while (t == nullptr || !t->is_alive())
          {
            ++focus;
            if (focus >= game::tanks.size())
            {
              focus = game::tank_focus;
              break;
            }
            t = game::id_at(focus);
          }
        }
        if (!game::output_inited || out_of_zone(focus))
        {
          term::move_cursor({0, 0});
          if (completely_out_of_zone(focus))
          {
            game::rendered_zone = get_visible_zone(focus);
          }
          else
          {
            game::rendered_zone = next_zone(focus);
          }
          game::load_zone(game::rendered_zone);
          for (int j = game::rendered_zone.y_max - 1; j >= static_cast<int>(game::rendered_zone.y_min); j--)
          {
            for (int i = game::rendered_zone.x_min; i < static_cast<int>(game::rendered_zone.x_max); i++)
            {
              update_point(map::Pos(i, j));
            }
            term::output("\n");
          }
          game::output_inited = true;
        }
        else
        {
          for (auto &p: game::game_map.get_changes())
          {
            if (p.get_pos().x >= game::rendered_zone.x_min
                && p.get_pos().x < game::rendered_zone.x_max
                && p.get_pos().y >= game::rendered_zone.y_min
                && p.get_pos().y < game::rendered_zone.y_max)
            {
              update_point(p.get_pos());
            }
          }
          game::game_map.clear_changes();
        }
      }
        break;
      case game::Page::TANK_STATUS:
        if (!game::output_inited)
        {
          term::clear();
          std::size_t cursor_y = 0;
          term::mvoutput({game::screen_width / 2 - 10, cursor_y++}, "Tank - by caozhanhao");
          size_t gap = 2;
          size_t id_x = gap;
          size_t name_x = id_x + gap + std::to_string((*std::max_element(game::tanks.begin(), game::tanks.end(),
                                                                         [](auto &&a, auto &&b)
                                                                         {
                                                                           return a.second->get_id() <
                                                                                  b.second->get_id();
                                                                         })).second->get_id()).size();
          auto pos_size = [](const map::Pos &p)
          {
            return std::to_string(p.x).size() + std::to_string(p.y).size() + 3;
          };
          size_t pos_x = name_x + gap + (*std::max_element(game::tanks.begin(), game::tanks.end(),
                                                           [](auto &&a, auto &&b)
                                                           {
                                                             return a.second->get_name().size() <
                                                                    b.second->get_name().size();
                                                           })).second->get_name().size();
          size_t hp_x = pos_x + gap + pos_size((*std::max_element(game::tanks.begin(), game::tanks.end(),
                                                                  [&pos_size](auto &&a, auto &&b)
                                                                  {
                                                                    return pos_size(a.second->get_pos()) <
                                                                           pos_size(b.second->get_pos());
                                                                  }
          )).second->get_pos());
          
          size_t lethality_x = hp_x + gap + std::to_string((*std::max_element(game::tanks.begin(), game::tanks.end(),
                                                                              [](auto &&a, auto &&b)
                                                                              {
                                                                                return a.second->get_hp() <
                                                                                       b.second->get_hp();
                                                                              })).second->get_hp()).size();
          
          size_t auto_tank_gap_x =
              lethality_x + gap + std::to_string((*std::max_element(game::tanks.begin(), game::tanks.end(),
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
          for (auto it = game::tanks.begin(); it != game::tanks.end(); ++it)
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
            if (cursor_y == game::screen_height - 1)
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
          game::output_inited = true;
        }
        break;
      case game::Page::MAIN:
      {
        if (!game::output_inited)
        {
          constexpr std::string_view tank = R"(
 _____           _
|_   _|_ _ _ __ | | __
  | |/ _` | '_ \| |/ /
  | | (_| | | | |   <
  |_|\__,_|_| |_|_|\_\
)";
          static const auto splitted = utils::split<std::vector<std::string_view>>(tank, "\n");
          auto s = utils::fit_to_screen(splitted, game::screen_width);
          size_t x = game::screen_width / 2 - 12;
          size_t y = 2;
          term::clear();
          for (size_t i = 0; i < s.size(); ++i)
          {
            term::mvoutput({x, y++}, std::string(s[i]));
          }
          term::mvoutput({x + 5, y + 3}, ">>> Enter <<<");
          term::mvoutput({x + 1, y + 4}, "Use '/help' to get help.");
          game::output_inited = true;
        }
      }
        break;
      case game::Page::HELP:
        static const std::string help =
            R"(
Keys:
  Move: WASD
  Attack: space
  All game::tanks' status: 'o' or 'O'
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
    - Default to revive all game::tanks.
    
  summon [n] [level]
    - Summon n game::tanks with the given level.
    - e.g. summon 50 10
    
  kill [A id]
    - Kill A.
    - Default to kill all game::tanks.

  clear [A id]
    - Clear A.(auto tank only)
    - Default to clear all auto game::tanks.
  clear death
    - Clear all the died auto game::tanks
    Note:
       Clear is to delete rather than to kill.
       So can't clear the user's tank and cleared game::tanks can't be revived.
       And the game::bullets  of the cleared tank will also be cleared.

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
          auto s = utils::fit_to_screen(splitted, game::screen_width);
          size_t page_size = term::get_height() - 3;
          if (!game::output_inited)
          {
            std::size_t cursor_y = 0;
            term::mvoutput({game::screen_width / 2 - 10, cursor_y++}, "Tank - by caozhanhao");
            if ((game::help_page - 1) * page_size > s.size()) game::help_page = 1;
            for (size_t i = (game::help_page - 1) * page_size; i < std::min(game::help_page * page_size, s.size()); ++i)
            {
              term::mvoutput({0, cursor_y++}, std::string(s[i]));
            }
            term::mvoutput({game::screen_width / 2 - 3, cursor_y}, "Page " + std::to_string(game::help_page));
            game::output_inited = true;
          }
        }
        break;
      case game::Page::COMMAND:
        if (!game::output_inited)
        {
          term::move_cursor({0, game::screen_height - 1});
          if (int a = game::screen_width - game::cmd_string.size(); a > 0)
          {
            term::output(game::cmd_string + std::string(a, ' ') + "\033[?25h");
          }
          else
          {
            term::output(game::cmd_string + "\033[?25h");
          }
          term::mvoutput({game::cmd_string_pos, game::screen_height - 1},
                         game::cmd_string.substr(game::cmd_string_pos, 1));
          game::output_inited = true;
        }
        break;
    }
    term::flush();
  }
}