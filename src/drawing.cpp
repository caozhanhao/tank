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
#include "tank/bullet.h"
#include "tank/utils.h"
#include "tank/term.h"
#include "tank/drawing.h"
#include "tank/globals.h"

#include <mutex>
#include <vector>
#include <string>
#include <list>

namespace czh::g
{
  bool output_inited = false;
  std::size_t screen_height = 0;
  std::size_t screen_width = 0;
  size_t tank_focus = 0;
  game::Page curr_page = game::Page::MAIN;
  size_t help_lineno = 1;
  std::vector<std::string> help_text;
  map::Zone visible_zone = {-128, 128, -128, 128};
  drawing::Snapshot snapshot{};
  int fps = 60;
  drawing::PointView empty_point_view{.status = map::Status::END, .tank_id = -1, .text = ""};
  drawing::PointView wall_point_view{.status = map::Status::WALL, .tank_id = -1, .text = ""};
  std::chrono::steady_clock::time_point last_drawing = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point last_message_displayed = std::chrono::steady_clock::now();
  drawing::Style style{.background = 15, .wall = 9, .tanks =
      {
          10, 3, 4, 5, 6, 11, 12, 13, 14, 57, 100, 214
      }};
  std::mutex drawing_mtx;
}

namespace czh::drawing
{
  const PointView &generate(const map::Pos &i, size_t seed)
  {
    if (map::generate(i, seed).has(map::Status::WALL))
    {
      return g::wall_point_view;
    }
    return g::empty_point_view;
  }
  
  const PointView &generate(int x, int y, size_t seed)
  {
    if (map::generate(x, y, seed).has(map::Status::WALL))
    {
      return g::wall_point_view;
    }
    return g::empty_point_view;
  }
  
  bool PointView::is_empty() const
  {
    return status == map::Status::END;
  }
  
  const PointView &MapView::at(int x, int y) const
  {
    return at(map::Pos(x, y));
  }
  
  const PointView &MapView::at(const map::Pos &i) const
  {
    if (view.find(i) != view.end())
    {
      return view.at(i);
    }
    return drawing::generate(i, seed);
  }
  
  bool MapView::is_empty() const
  {
    return view.empty();
  }
  
  PointView extract_point(const map::Pos &p)
  {
    if (g::game_map.has(map::Status::TANK, p))
    {
      return {
          .status = map::Status::TANK,
          .tank_id = static_cast<int>(g::game_map.at(p).get_tank()->get_id()),
          .text = ""
      };
    }
    else if (g::game_map.has(map::Status::BULLET, p))
    {
      return {
          .status = map::Status::BULLET,
          .tank_id = static_cast<int>(g::game_map.at(p).get_bullets()[0]->get_tank()),
          .text = g::game_map.at(p).get_bullets()[0]->get_text()
      };
    }
    else if (g::game_map.has(map::Status::WALL, p))
    {
      return {
          .status = map::Status::WALL,
          .tank_id = -1,
          .text = ""
      };
    }
    else
    {
      return {
          .status = map::Status::END,
          .tank_id = -1,
          .text = ""
      };
    }
    return {};
  }
  
  MapView extract_map(const map::Zone &zone)
  {
    MapView ret;
    ret.seed = g::seed;
    for (int i = zone.x_min; i < zone.x_max; ++i)
    {
      for (int j = zone.y_min; j < zone.y_max; ++j)
      {
        if (!g::game_map.at(i, j).is_generated())
        {
          ret.view.insert(std::make_pair(map::Pos{i, j}, extract_point({i, j})));
        }
      }
    }
    return ret;
  }
  
  std::map<size_t, TankView> extract_tanks()
  {
    std::map<size_t, TankView> view;
    for (auto &r: g::tanks)
    {
      view.insert(std::make_pair(r.second->get_id(),
                                 TankView{
                                     .info = r.second->get_info(),
                                     .hp = r.second->get_hp(),
                                     .pos = r.second->get_pos(),
                                     .direction = r.second->get_direction(),
                                     .is_auto = r.second->is_auto(),
                                     .is_alive = r.second->is_alive()
                                 }));
    }
    return view;
  }
  
  std::optional<TankView> view_id_at(size_t id)
  {
    auto it = g::snapshot.tanks.find(id);
    if (it == g::snapshot.tanks.end()) return std::nullopt;
    return it->second;
  }
  
  std::string colorify_text(size_t id, const std::string &str)
  {
    std::string ret = "\033[38;5;";
    int color;
    if (id == 0)
    {
      color = g::style.tanks[0];
    }
    else
    {
      color = g::style.tanks[id % g::style.tanks.size()];
    }
    return utils::color_256_fg(str, color);
  }
  
  std::string colorify_tank(size_t id, const std::string &str)
  {
    std::string ret = "\033[38;5;";
    int color;
    if (id == 0)
    {
      color = g::style.tanks[0];
    }
    else
    {
      color = g::style.tanks[id % g::style.tanks.size()];
    }
    return utils::color_256_bg(str, color);
  }
  
  void update_point(const map::Pos &pos)
  {
    term::move_cursor({static_cast<size_t>((pos.x - g::visible_zone.x_min) * 2),
                       static_cast<size_t>(g::visible_zone.y_max - pos.y - 1)});
    switch (g::snapshot.map.at(pos).status)
    {
      case map::Status::TANK:
        term::output(colorify_tank(g::snapshot.map.at(pos).tank_id, "  "));
        break;
      case map::Status::BULLET:
        term::output(utils::color_256_bg(colorify_text(g::snapshot.map.at(pos).tank_id,
                                                       g::snapshot.map.at(pos).text), g::style.background));
        break;
      case map::Status::WALL:
        term::output(utils::color_256_bg("  ", g::style.wall));
        break;
      case map::Status::END:
        term::output(utils::color_256_bg("  ", g::style.background));
        break;
    }
  }
  
  bool check_zone_size(const map::Zone &z)
  {
    size_t h = z.y_max - z.y_min;
    size_t w = z.x_max - z.x_min;
    if (h != g::screen_height - 2) return false;
    if (g::screen_width % 2 == 0)
    {
      if (g::screen_width != w * 2) return false;
    }
    else
    {
      if (g::screen_width - 1 != w * 2) return false;
    }
    return true;
  }
  
  // [X min, X max)   [Y min, Y max)
  map::Zone get_visible_zone(size_t w, size_t h, size_t id)
  {
    auto pos = view_id_at(id)->pos;
    map::Zone ret;
    int offset_x = (int) h / 4;
    ret.x_min = pos.x - offset_x;
    ret.x_max = (int) w / 2 + ret.x_min;
    
    int offset_y = (int) h / 2;
    ret.y_min = pos.y - offset_y;
    ret.y_max = (int) h - 2 + ret.y_min;
    return ret;
  }
  
  map::Zone get_visible_zone(size_t id)
  {
    auto ret = get_visible_zone(g::screen_width, g::screen_height, id);
    utils::tank_assert(check_zone_size(ret));
    return ret;
  }
  
  
  std::set<map::Pos> get_screen_changes(const map::Direction &move)
  {
    std::set<map::Pos> ret;
    // When the visible zone moves, every point in the screen doesn't move, but its corresponding pos changes.
    // so we need to do something to get the correct changes:
    // 1.  if there's no difference in the two point in the moving direction, ignore.
    // 2.  move the map's map_changes to its corresponding screen position.
    auto zone = g::visible_zone.bigger_zone(2);
    switch (move)
    {
      case map::Direction::UP:
        for (int i = g::visible_zone.x_min; i < g::visible_zone.x_max; i++)
        {
          for (int j = g::visible_zone.y_min - 1; j < g::visible_zone.y_max + 1; j++)
          {
            if (!g::snapshot.map.at(i, j + 1).is_empty() || !g::snapshot.map.at(i, j).is_empty())
            {
              ret.insert(map::Pos(i, j));
              ret.insert(map::Pos(i, j + 1));
            }
          }
        }
        for (auto &p: g::snapshot.changes)
        {
          if (zone.contains(p))
          {
            ret.insert(map::Pos{p.x, p.y + 1});
          }
        }
        break;
      case map::Direction::DOWN:
        for (int i = g::visible_zone.x_min; i < g::visible_zone.x_max; i++)
        {
          for (int j = g::visible_zone.y_min - 1; j < g::visible_zone.y_max + 1; j++)
          {
            if (!g::snapshot.map.at(i, j - 1).is_empty() || !g::snapshot.map.at(i, j).is_empty())
            {
              ret.insert(map::Pos(i, j));
              ret.insert(map::Pos(i, j - 1));
            }
          }
        }
        for (auto &p: g::snapshot.changes)
        {
          if (zone.contains(p))
          {
            ret.insert(map::Pos{p.x, p.y - 1});
          }
        }
        break;
      case map::Direction::LEFT:
        for (int i = g::visible_zone.x_min - 1; i < g::visible_zone.x_max + 1; i++)
        {
          for (int j = g::visible_zone.y_min; j < g::visible_zone.y_max; j++)
          {
            if (!g::snapshot.map.at(i - 1, j).is_empty() || !g::snapshot.map.at(i, j).is_empty())
            {
              ret.insert(map::Pos(i, j));
              ret.insert(map::Pos(i - 1, j));
            }
          }
        }
        for (auto &p: g::snapshot.changes)
        {
          if (zone.contains(p))
          {
            ret.insert(map::Pos{p.x - 1, p.y});
          }
        }
        break;
      case map::Direction::RIGHT:
        for (int i = g::visible_zone.x_min - 1; i < g::visible_zone.x_max + 1; i++)
        {
          for (int j = g::visible_zone.y_min; j < g::visible_zone.y_max; j++)
          {
            if (!g::snapshot.map.at(i + 1, j).is_empty() || !g::snapshot.map.at(i, j).is_empty())
            {
              ret.insert(map::Pos(i, j));
              ret.insert(map::Pos(i + 1, j));
            }
          }
        }
        for (auto &p: g::snapshot.changes)
        {
          if (zone.contains(p))
          {
            ret.insert(map::Pos{p.x + 1, p.y});
          }
        }
        break;
      case map::Direction::END:
        for (auto &p: g::snapshot.changes)
        {
          if (zone.contains(p))
          {
            ret.insert(p);
          }
        }
        break;
    }
    g::snapshot.changes.clear();
    return ret;
  }
  
  void next_zone(const map::Direction &direction)
  {
    switch (direction)
    {
      case map::Direction::UP:
        g::visible_zone.y_max++;
        g::visible_zone.y_min++;
        break;
      case map::Direction::DOWN:
        g::visible_zone.y_max--;
        g::visible_zone.y_min--;
        break;
      case map::Direction::LEFT:
        g::visible_zone.x_max--;
        g::visible_zone.x_min--;
        break;
      case map::Direction::RIGHT:
        g::visible_zone.x_max++;
        g::visible_zone.x_min++;
        break;
      default:
        break;
    }
  }
  
  bool out_of_zone(size_t id)
  {
    auto pos = view_id_at(id)->pos;
    return
        ((g::visible_zone.x_min + 5 > pos.x)
         || (g::visible_zone.x_max - 5 <= pos.x)
         || (g::visible_zone.y_min + 5 > pos.y)
         || (g::visible_zone.y_max - 5 <= pos.y));
  }
  
  bool completely_out_of_zone(size_t id)
  {
    auto pos = view_id_at(id)->pos;
    return
        ((g::visible_zone.x_min > pos.x)
         || (g::visible_zone.x_max <= pos.x)
         || (g::visible_zone.y_min > pos.y)
         || (g::visible_zone.y_max <= pos.y));
  }
  
  int update_snapshot()
  {
    if (g::game_mode == czh::game::GameMode::SERVER || g::game_mode == czh::game::GameMode::NATIVE)
    {
      g::snapshot.map = extract_map(g::visible_zone.bigger_zone(10));
      g::snapshot.tanks = extract_tanks();
      g::snapshot.changes = g::userdata[g::user_id].map_changes;
      g::userdata[g::user_id].map_changes.clear();
      return 0;
    }
    else
    {
      int ret = g::online_client.update();
      if (ret == 0)
      {
        g::client_failed_attempts = 0;
        return 0;
      }
      else
      {
        g::client_failed_attempts++;
        g::output_inited = false;
        return -1;
      }
    }
    return -1;
  }
  
  void draw()
  {
    term::hide_cursor();
    std::lock_guard<std::mutex> l1(g::mainloop_mtx);
    std::lock_guard<std::mutex> l2(g::drawing_mtx);
    if (g::screen_height != term::get_height() || g::screen_width != term::get_width())
    {
      term::clear();
      g::help_lineno = 1;
      g::output_inited = false;
      g::screen_height = term::get_height();
      g::screen_width = term::get_width();
      
      static const std::string help =
          R"(
Intro:
  In Tank, you will take control of a powerful tank in a maze, showcasing your strategic skills on the infinite map and overcome unpredictable obstacles. You can play solo or team up with friends.

Control:
  Move: WASD or direction keys
  Attack: space
  Tank Status: 'o' or 'O'
  Command: '/'

Tank:
  User's Tank:
    HP: 10000, Lethality: 100
  Auto Tank:
    HP: (11 - level) * 150, Lethality: (11 - level) * 15
    The higher level the tank is, the faster it moves.
    
Command:
  help [line]
    - Get this help.
    - Use 'Enter' to return game.

  quit
    - Quit Tank.
    
  pause
    - Pause.

  continue
    - Continue.

  fill [Status] [A x,y] [B x,y optional]
    - Status: [0] Empty [1] Wall
    - Fill the area from A to B as the given Status.
    - B defaults to the same as A
    - e.g.  fill 1 0 0 10 10   |   fill 1 0 0

  tp [A id] ([B id] or [B x,y])
    - Teleport A to B
    - A should be alive, and there should be space around B.
    - e.g.  tp 0 1   |  tp 0 1 1
    
  revive [A id optional]
    - Revive A.
    - Default to revive all tanks.
    
  summon [n] [level]
    - Summon n tanks with the given level.
    - e.g. summon 50 10
    
  kill [A id optional]
    - Kill A.
    - Default to kill all tanks.

  clear [A id optional]
    - Clear A.(only Auto Tank)
    - Default to clear all auto tanks.
  clear death
    - Clear all the died Auto Tanks
    Note:
       Clear is to delete rather than to kill, so the cleared tank can't revive.
       And the bullets of the cleared tank will also be cleared.
      
  set [A id] [key] [value]
    - Set A's attribute below:
      - max_hp (int): Max hp of A. This will take effect when A is revived.
      - hp (int): hp of A. This takes effect immediately but won't last when A is revived.
      - target (id, int): Auto Tank's target. Target should be alive.
      - name (string): Name of A.
  set [A id] bullet [key] [value]
      - hp (int): hp of A's bullet.
      - lethality (int): lethality of A's bullet. (negative to increase hp)
      - range (int): range of A's bullet.(default)
      - e.g. set 0 max_hp 1000  |  set 0 bullet lethality 10
      Note:
        When a bullet hits the wall, its hp decreases by one. That means it can bounce "hp - 1" times.
  set tick [tick]
      - tick (int, milliseconds): minimum time of the game's(or server's) mainloop.
  set msg_ttl [ttl]
      - ttl (int, milliseconds): a message's time to live.
  set seed [seed]
      - seed (int): the game map's seed.
  
  tell [A id optional] [msg]
    - Send a message to A.
    - id (int): defaults to be -1, in which case all the players will receive the message.
    - msg (string): the message's content.

  observe [A id]
    - Observe A.
    
  server start [port]
    - Start Tank Server.
    - port (int): the server's port.
  server stop
    - Stop Tank Server.

  connect [ip] [port]
    - Connect to Tank Server.
    - ip (string): the server's IP.
    - port (int): the server's port.
  
  disconnect
    - Disconnect from the Server.
)";
      static auto raw_lines = utils::split<std::vector<std::string_view>>(help, "\n");
      g::help_text.clear();
      for (auto &r: raw_lines)
      {
        if (r.size() > g::screen_width)
        {
          std::string indent, temp;
          for (auto &i: r)
          {
            if (std::isspace(i))
            {
              indent += i;
            }
            else
            {
              break;
            }
          }
          temp = indent;
          for (size_t i = 0; i < r.size(); ++i)
          {
            if (temp.size() == indent.size() && std::isspace(r[i])) continue;
            temp += r[i];
            if (temp.size() == g::screen_width)
            {
              if (i + 1 < r.size() && std::isalpha(r[i + 1]) && std::isalpha(r[i]))
              {
                int j = i;
                for (; j >= 0 && !std::isspace(r[j]); --j)
                {
                  temp.pop_back();
                }
                temp.insert(temp.end(), i - j, ' ');
                g::help_text.emplace_back(temp);
                temp = indent;
                i = j;
              }
              else
              {
                g::help_text.emplace_back(temp);
                temp = indent;
              }
            }
          }
          if (!temp.empty())
          {
            g::help_text.emplace_back(temp);
          }
        }
        else
        {
          g::help_text.emplace_back(r);
        }
      }
    }
    switch (g::curr_page)
    {
      case game::Page::GAME:
      {
        // check zone
        if (!check_zone_size(g::visible_zone))
        {
          g::visible_zone = get_visible_zone(g::tank_focus);
          g::output_inited = false;
          return;
        }
        
        auto move = map::Direction::END;
        
        if (out_of_zone(g::tank_focus))
        {
          if (completely_out_of_zone(g::tank_focus))
          {
            g::visible_zone = get_visible_zone(g::tank_focus);
            g::output_inited = false;
            int r = update_snapshot();
            if (r != 0) return;
          }
          else
          {
            move = view_id_at(g::tank_focus)->direction;
            next_zone(move);
          }
        }
        
        // output
        if (!g::output_inited)
        {
          term::move_cursor({0, 0});
          for (int j = g::visible_zone.y_max - 1; j >= g::visible_zone.y_min; j--)
          {
            for (int i = g::visible_zone.x_min; i < g::visible_zone.x_max; i++)
            {
              update_point(map::Pos(i, j));
            }
          }
          g::output_inited = true;
        }
        else
        {
          auto changes = get_screen_changes(move);
          for (auto &p: changes)
          {
            if (g::visible_zone.contains(p))
            {
              update_point(p);
            }
          }
        }
        
        auto now = std::chrono::steady_clock::now();
        auto delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - g::last_drawing);
        if (delta_time.count() != 0)
        {
          double curr_fps = 1.0 / (static_cast<double>(delta_time.count()) / 1000.0);
          g::fps = static_cast<int>((static_cast<double>(g::fps) + 0.01 * curr_fps) / 1.01);
        }
        g::last_drawing = now;
        
        // status bar
        term::move_cursor(term::TermPos(0, g::screen_height - 2));
        auto &focus_tank = g::snapshot.tanks[g::tank_focus];
        std::string left = colorify_text(focus_tank.info.id, focus_tank.info.name)
                           + " HP: " + std::to_string(focus_tank.hp) + "/" + std::to_string(focus_tank.info.max_hp)
                           + " Pos: (" + std::to_string(focus_tank.pos.x) + ", " + std::to_string(focus_tank.pos.y) +
                           ")";
        std::string right = std::to_string(g::fps) + "fps ";
        
        if (g::delay < 50)
        {
          right += utils::color_256_fg(std::to_string(g::delay) + " ms", 2);
        }
        else if (g::delay < 100)
        {
          right += utils::color_256_fg(std::to_string(g::delay) + " ms", 11);
        }
        else
        {
          right += utils::color_256_fg(std::to_string(g::delay) + " ms", 9);
        }
        
        int a = static_cast<int>(g::screen_width) - static_cast<int>(utils::escape_code_len(left, right));
        if (a > 0)
        {
          term::output(left, std::string(a, ' '), right);
        }
        else
        {
          term::output((left + right).substr(0, left.size() + right.size() + a));
        }
      }
        break;
      case game::Page::TANK_STATUS:
      {
        term::clear();
        std::size_t cursor_y = 0;
        term::mvoutput({g::screen_width / 2 - 10, cursor_y++}, "Tank");
        size_t gap = 2;
        size_t id_x = gap;
        size_t name_x =
            id_x + gap + std::to_string((*std::max_element(g::snapshot.tanks.begin(), g::snapshot.tanks.end(),
                                                           [](auto &&a, auto &&b)
                                                           {
                                                             return a.second.info.id <
                                                                    b.second.info.id;
                                                           })).second.info.id).size();
        size_t pos_x = name_x + gap + (*std::max_element(g::snapshot.tanks.begin(), g::snapshot.tanks.end(),
                                                         [](auto &&a, auto &&b)
                                                         {
                                                           return a.second.info.name.size() <
                                                                  b.second.info.name.size();
                                                         })).second.info.name.size();
        auto pos_size = [](const map::Pos &p)
        {
          // (x, y)
          return std::to_string(p.x).size() + std::to_string(p.y).size() + 4;
        };
        size_t hp_x = pos_x + gap + pos_size((*std::max_element(g::snapshot.tanks.begin(), g::snapshot.tanks.end(),
                                                                [&pos_size](auto &&a, auto &&b)
                                                                {
                                                                  return pos_size(a.second.pos) <
                                                                         pos_size(b.second.pos);
                                                                })).second.pos);
        
        size_t lethality_x =
            hp_x + gap + std::to_string((*std::max_element(g::snapshot.tanks.begin(), g::snapshot.tanks.end(),
                                                           [](auto &&a, auto &&b)
                                                           {
                                                             return a.second.hp <
                                                                    b.second.hp;
                                                           })).second.hp).size();
        
        size_t auto_tank_gap_x =
            lethality_x + gap + std::to_string((*std::max_element(g::snapshot.tanks.begin(), g::snapshot.tanks.end(),
                                                                  [](auto &&a, auto &&b)
                                                                  {
                                                                    return
                                                                        a.second.info.bullet.lethality <
                                                                        b.second.info.bullet.lethality;
                                                                  })).second.info.bullet.lethality).size();
  
        term::mvoutput({id_x, cursor_y}, "ID");
        term::mvoutput({name_x, cursor_y}, "Name");
        term::mvoutput({pos_x, cursor_y}, "Pos");
        term::mvoutput({hp_x, cursor_y}, "HP");
        term::mvoutput({lethality_x, cursor_y}, "ATK");
        term::mvoutput({auto_tank_gap_x, cursor_y}, "Gap");
        cursor_y++;
        for (auto it = g::snapshot.tanks.begin(); it != g::snapshot.tanks.end(); ++it)
        {
          auto tank = it->second;
          std::string x = std::to_string(tank.pos.x);
          std::string y = std::to_string(tank.pos.y);
          term::mvoutput({id_x, cursor_y}, tank.info.id);
          term::mvoutput({name_x, cursor_y}, colorify_text(tank.info.id, tank.info.name));
          term::mvoutput({pos_x, cursor_y}, "(", x, ",", y, ")");
          term::mvoutput({hp_x, cursor_y}, tank.hp);
          term::mvoutput({lethality_x, cursor_y}, tank.info.bullet.lethality);
          if (tank.is_auto)
          {
            term::mvoutput({auto_tank_gap_x, cursor_y}, tank.info.gap);
          }
          else
          {
            term::mvoutput({auto_tank_gap_x, cursor_y}, "-");
          }
          
          cursor_y++;
          if (cursor_y == g::screen_height - 2)
          {
            size_t offset = auto_tank_gap_x + 3 - id_x + gap;
            id_x += offset;
            name_x += offset;
            pos_x += offset;
            hp_x += offset;
            lethality_x += offset;
            auto_tank_gap_x += offset;
            cursor_y = 1;
            term::mvoutput({id_x, cursor_y}, "ID");
            term::mvoutput({name_x, cursor_y}, "Name");
            term::mvoutput({pos_x, cursor_y}, "Pos");
            term::mvoutput({hp_x, cursor_y}, "HP");
            term::mvoutput({lethality_x, cursor_y}, "ATK");
            term::mvoutput({auto_tank_gap_x, cursor_y}, "Gap");
            cursor_y = 2;
          }
        }
      }
        break;
      case game::Page::MAIN:
      {
        if (!g::output_inited)
        {
          constexpr std::string_view tank = R"(
 _____  _    _   _ _  __
|_   _|/ \  | \ | | |/ /
  | | / _ \ |  \| | ' /
  | |/ ___ \| |\  | . \
  |_/_/   \_\_| \_|_|\_\
)";
          size_t x = g::screen_width / 2 - 12;
          size_t y = 2;
          term::clear();
          if (g::screen_width > 24)
          {
            std::vector<std::string_view> splitted = utils::split<std::vector<std::string_view>>(tank, "\n");
            for (size_t i = 0; i < splitted.size(); ++i)
            {
              term::mvoutput({x, y++}, splitted[i]);
            }
          }
          else
          {
            x = g::screen_width / 2 - 2;
            term::mvoutput({x, y++}, "TANK");
          }
          
          term::mvoutput({x + 5, y + 3}, ">>> Enter <<<");
          term::mvoutput({x + 1, y + 4}, "Type '/help' to get help.");
          g::output_inited = true;
        }
      }
        break;
      case game::Page::HELP:
      {
        if (!g::output_inited)
        {
          term::clear();
          std::size_t cursor_y = 0;
          term::mvoutput({g::screen_width / 2 - 2, cursor_y++}, "Tank");
          if (g::help_lineno > g::help_text.size()) g::help_lineno = 1;
          for (size_t i = g::help_lineno - 1;
               i < (std::min)(g::help_lineno + g::screen_height - 4, g::help_text.size()); ++i)
          {
            term::mvoutput({0, cursor_y++}, g::help_text[i]);
          }
          term::mvoutput({g::screen_width / 2 - 3, g::screen_height - 2}, "Line ", g::help_lineno);
          g::output_inited = true;
        }
      }
        break;
    }
    // command
    if (g::typing_command)
    {
      term::move_cursor({g::cmd_pos + 1, g::screen_height - 1});
      term::show_cursor();
    }
    else
    {
      term::move_cursor(term::TermPos(0, g::screen_height - 1));
      auto now = std::chrono::steady_clock::now();
      auto d2 = std::chrono::duration_cast<std::chrono::milliseconds>(now - g::last_message_displayed);
      if (d2 > g::msg_ttl)
      {
        if (!g::userdata[g::user_id].messages.empty())
        {
          auto msg = g::userdata[g::user_id].messages.top();
          g::userdata[g::user_id].messages.pop();
          std::string str = ((msg.from == -1) ? "" : std::to_string(msg.from) + ": ") + msg.content;
          int a2 = static_cast<int>(g::screen_width) - static_cast<int>(utils::escape_code_len(str));
          if (a2 > 0)
          {
            term::output(str, std::string(a2, ' '));
          }
          else
          {
            term::output(str.substr(0, str.size() + a2));
          }
          g::last_message_displayed = now;
        }
        else
        {
          term::output(std::string(g::screen_width, ' '));
        }
      }
    }
    term::flush();
  }
}