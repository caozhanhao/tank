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
#include "tank/renderer.h"
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
  game::Page curr_page = game::Page::MAIN;
  size_t help_page = 0;
  map::Zone render_zone = {-128, 128, -128, 128};
  renderer::Frame frame{};
  int fps = 60;
  renderer::PointView empty_point_view{.status = map::Status::END, .tank_id = -1, .text = ""};
  renderer::PointView wall_point_view{.status = map::Status::WALL, .tank_id = -1, .text = ""};
  std::chrono::steady_clock::time_point last_render = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point last_message_displayed = std::chrono::steady_clock::now();
  renderer::Style style{.background = 15, .wall = 9, .default_tank = 10};
  std::mutex render_mtx;
}

namespace czh::renderer
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
    return renderer::generate(i, seed);
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
    auto it = g::frame.tanks.find(id);
    if (it == g::frame.tanks.end()) return std::nullopt;
    return it->second;
  }
  
  const std::vector<int> &get_available_colors()
  {
    static std::vector<int> available_colors;
    if (available_colors.empty())
    {
      available_colors.emplace_back(g::style.default_tank);
      for (int i = 0; i < 16; ++i)
      {
        if (i == g::style.background
            || i == g::style.wall
            || i == g::style.default_tank
            || i == 0 // Black
            || i == 1 // Maroon
            )
        {
          continue;
        }
        available_colors.emplace_back(i);
      }
    }
    return available_colors;
  }
  
  std::string colorify_text(size_t id, const std::string &str)
  {
    const auto &avail = get_available_colors();
    std::string ret = "\033[38;5;";
    int color;
    if (id == 0)
    {
      color = avail[0];
    }
    else
    {
      color = avail[id % avail.size()];
    }
    return utils::color_256_fg(str, color);
  }
  
  std::string colorify_tank(size_t id, const std::string &str)
  {
    const auto &avail = get_available_colors();
    std::string ret = "\033[38;5;";
    int color;
    if (id == 0)
    {
      color = avail[0];
    }
    else
    {
      color = avail[id % avail.size()];
    }
    return utils::color_256_bg(str, color);
  }
  
  void update_point(const map::Pos &pos)
  {
    term::move_cursor({static_cast<size_t>((pos.x - g::render_zone.x_min) * 2),
                       static_cast<size_t>(g::render_zone.y_max - pos.y - 1)});
    switch (g::frame.map.at(pos).status)
    {
      case map::Status::TANK:
        term::output(colorify_tank(g::frame.map.at(pos).tank_id, "  "));
        break;
      case map::Status::BULLET:
        term::output(utils::color_256_bg(colorify_text(g::frame.map.at(pos).tank_id,
                                                       g::frame.map.at(pos).text), g::style.background));
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
  
  
  std::set<map::Pos> get_render_changes(const map::Direction &move)
  {
    std::set<map::Pos> ret;
    // When the render zone moves, every point in the screen doesn't move, but its corresponding pos map_changes.
    // so we need to do something to get the correct map_changes:
    // 1.  if there's no difference in the two point in the moving direction, ignore.
    // 2.  move the map's map_changes to its corresponding screen position.
    auto zone = g::render_zone.bigger_zone(2);
    switch (move)
    {
      case map::Direction::UP:
        for (int i = g::render_zone.x_min; i < g::render_zone.x_max; i++)
        {
          for (int j = g::render_zone.y_min - 1; j < g::render_zone.y_max + 1; j++)
          {
            if (!g::frame.map.at(i, j + 1).is_empty() || !g::frame.map.at(i, j).is_empty())
            {
              ret.insert(map::Pos(i, j));
              ret.insert(map::Pos(i, j + 1));
            }
          }
        }
        for (auto &p: g::frame.changes)
        {
          if (zone.contains(p))
          {
            ret.insert(map::Pos{p.x, p.y + 1});
          }
        }
        break;
      case map::Direction::DOWN:
        for (int i = g::render_zone.x_min; i < g::render_zone.x_max; i++)
        {
          for (int j = g::render_zone.y_min - 1; j < g::render_zone.y_max + 1; j++)
          {
            if (!g::frame.map.at(i, j - 1).is_empty() || !g::frame.map.at(i, j).is_empty())
            {
              ret.insert(map::Pos(i, j));
              ret.insert(map::Pos(i, j - 1));
            }
          }
        }
        for (auto &p: g::frame.changes)
        {
          if (zone.contains(p))
          {
            ret.insert(map::Pos{p.x, p.y - 1});
          }
        }
        break;
      case map::Direction::LEFT:
        for (int i = g::render_zone.x_min - 1; i < g::render_zone.x_max + 1; i++)
        {
          for (int j = g::render_zone.y_min; j < g::render_zone.y_max; j++)
          {
            if (!g::frame.map.at(i - 1, j).is_empty() || !g::frame.map.at(i, j).is_empty())
            {
              ret.insert(map::Pos(i, j));
              ret.insert(map::Pos(i - 1, j));
            }
          }
        }
        for (auto &p: g::frame.changes)
        {
          if (zone.contains(p))
          {
            ret.insert(map::Pos{p.x - 1, p.y});
          }
        }
        break;
      case map::Direction::RIGHT:
        for (int i = g::render_zone.x_min - 1; i < g::render_zone.x_max + 1; i++)
        {
          for (int j = g::render_zone.y_min; j < g::render_zone.y_max; j++)
          {
            if (!g::frame.map.at(i + 1, j).is_empty() || !g::frame.map.at(i, j).is_empty())
            {
              ret.insert(map::Pos(i, j));
              ret.insert(map::Pos(i + 1, j));
            }
          }
        }
        for (auto &p: g::frame.changes)
        {
          if (zone.contains(p))
          {
            ret.insert(map::Pos{p.x + 1, p.y});
          }
        }
        break;
      case map::Direction::END:
        for (auto &p: g::frame.changes)
        {
          if (zone.contains(p))
          {
            ret.insert(p);
          }
        }
        break;
    }
    g::frame.changes.clear();
    return ret;
  }
  
  void next_zone(const map::Direction &direction)
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
      default:
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
  
  int update_frame()
  {
    if (g::game_mode == czh::game::GameMode::SERVER || g::game_mode == czh::game::GameMode::NATIVE)
    {
      g::frame.map = extract_map(g::render_zone.bigger_zone(10));
      g::frame.tanks = extract_tanks();
      g::frame.changes = g::userdata[g::user_id].map_changes;
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
  
  void render()
  {
    term::hide_cursor();
    std::lock_guard<std::mutex> l1(g::mainloop_mtx);
    std::lock_guard<std::mutex> l2(g::render_mtx);
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
      case game::Page::COMMAND:
      {
        // check focus
        size_t focus = g::tank_focus;
        if (auto t = view_id_at(focus); t == std::nullopt || !t->is_alive)
        {
          focus = 0;
          t = view_id_at(focus);
          while (t == std::nullopt || !t->is_alive)
          {
            ++focus;
            if (focus >= g::frame.tanks.size())
            {
              focus = g::tank_focus;
              break;
            }
            t = view_id_at(focus);
          }
        }
        
        // check zone
        if (!check_zone_size(g::render_zone))
        {
          g::render_zone = get_visible_zone(g::tank_focus);
          g::output_inited = false;
          return;
        }
        
        auto move = map::Direction::END;
        
        
        if (out_of_zone(focus))
        {
          if (completely_out_of_zone(focus))
          {
            g::render_zone = get_visible_zone(focus);
            g::output_inited = false;
            int r = update_frame();
            if (r != 0) return;
          }
          else
          {
            move = view_id_at(focus)->direction;
            next_zone(move);
          }
        }
        
        // output
        if (!g::output_inited)
        {
          term::move_cursor({0, 0});
          for (int j = g::render_zone.y_max - 1; j >= g::render_zone.y_min; j--)
          {
            for (int i = g::render_zone.x_min; i < g::render_zone.x_max; i++)
            {
              update_point(map::Pos(i, j));
            }
          }
          g::output_inited = true;
        }
        else
        {
          auto changes = get_render_changes(move);
          for (auto &p: changes)
          {
            if (g::render_zone.contains(p))
            {
              update_point(p);
            }
          }
        }
        
        auto now = std::chrono::steady_clock::now();
        auto delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - g::last_render);
        if (delta_time.count() != 0)
        {
          double curr_fps = 1.0 / (static_cast<double>(delta_time.count()) / 1000.0);
          g::fps = static_cast<int>((static_cast<double>(g::fps) + 0.01 * curr_fps) / 1.01);
        }
        g::last_render = now;
        
        // status bar
        term::move_cursor(term::TermPos(0, g::screen_height - 2));
        auto &focus_tank = g::frame.tanks[focus];
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
        
        // command
        if(g::curr_page == game::Page::COMMAND)
        {
          term::move_cursor({g::cmd_pos + 1, g::screen_height - 1});
          term::show_cursor();
        }
        // msg
        else
        {
          term::move_cursor(term::TermPos(0, g::screen_height - 1));
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
                term::output(str.substr(0, str.size() + a));
              }
              g::last_message_displayed = now;
            }
            else
            {
              term::output(std::string(g::screen_width, ' '));
            }
          }
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
        size_t name_x = id_x + gap + std::to_string((*std::max_element(g::frame.tanks.begin(), g::frame.tanks.end(),
                                                                       [](auto &&a, auto &&b)
                                                                       {
                                                                         return a.second.info.id <
                                                                                b.second.info.id;
                                                                       })).second.info.id).size();
        auto pos_size = [](const map::Pos &p)
        {
          return std::to_string(p.x).size() + std::to_string(p.y).size() + 3;
        };
        size_t pos_x = name_x + gap + (*std::max_element(g::frame.tanks.begin(), g::frame.tanks.end(),
                                                         [](auto &&a, auto &&b)
                                                         {
                                                           return a.second.info.name.size() <
                                                                  b.second.info.name.size();
                                                         })).second.info.name.size();
        size_t hp_x = pos_x + gap + pos_size((*std::max_element(g::frame.tanks.begin(), g::frame.tanks.end(),
                                                                [&pos_size](auto &&a, auto &&b)
                                                                {
                                                                  return pos_size(a.second.pos) <
                                                                         pos_size(b.second.pos);
                                                                }
        )).second.pos);
        
        size_t lethality_x =
            hp_x + gap + std::to_string((*std::max_element(g::frame.tanks.begin(), g::frame.tanks.end(),
                                                           [](auto &&a, auto &&b)
                                                           {
                                                             return a.second.hp <
                                                                    b.second.hp;
                                                           })).second.hp).size();
        
        size_t auto_tank_gap_x =
            lethality_x + gap + std::to_string((*std::max_element(g::frame.tanks.begin(), g::frame.tanks.end(),
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
        for (auto it = g::frame.tanks.begin(); it != g::frame.tanks.end(); ++it)
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
          static const auto splitted = utils::split<std::vector<std::string_view>>(tank, "\n");
          auto s = utils::fit_to_screen(splitted, g::screen_width);
          size_t x = g::screen_width / 2 - 12;
          size_t y = 2;
          term::clear();
          for (size_t i = 0; i < s.size(); ++i)
          {
            term::mvoutput({x, y++}, s[i]);
          }
          term::mvoutput({x + 5, y + 3}, ">>> Enter <<<");
          term::mvoutput({x + 1, y + 4}, "Type '/help' to get help.");
          g::output_inited = true;
        }
      }
        break;
      case game::Page::HELP:
      {
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
  help [page]
    - Get this help.
    - Use 'Enter' to return game.

  quit
    - Quit Tank.

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
    - id defaults to be -1, in which case all the players will receive the message.
    - msg (string): the message's content.
    
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
        static auto splitted = utils::split<std::vector<std::string_view>>(help, "\n");
        auto s = utils::fit_to_screen(splitted, g::screen_width);
        size_t page_size = term::get_height() - 3;
        if (!g::output_inited)
        {
          std::size_t cursor_y = 0;
          term::mvoutput({g::screen_width / 2 - 2, cursor_y++}, "Tank");
          if ((g::help_page - 1) * page_size > s.size()) g::help_page = 1;
          for (size_t i = (g::help_page - 1) * page_size; i < (std::min)(g::help_page * page_size, s.size()); ++i)
          {
            term::mvoutput({0, cursor_y++}, s[i]);
          }
          term::mvoutput({g::screen_width / 2 - 3, cursor_y}, "Page ", g::help_page);
          g::output_inited = true;
        }
      }
        break;
    }
    term::flush();
  }
}