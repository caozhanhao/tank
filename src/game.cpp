//   Copyright 2022-2023 tank - caozhanhao
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
#include "internal/utils.h"
#include "internal/cmd_parser.h"
#include "internal/info.h"
#include "internal/tank.h"
#include "internal/term.h"
#include "internal/game_map.h"
#include "internal/bullet.h"
#include "internal/logger.h"
#include "game.h"
#include <vector>
#include <functional>
#include <string>
#include <string_view>
#include <memory>

namespace czh::game
{
  void tank_assert(bool a, const std::string& err = "Assertion Failed.")
  {
    if(!a)
      throw std::runtime_error(err);
  }
  
  std::size_t Game::add_tank()
  {
    id_index[next_id] = tanks.size();
    tanks.insert(tanks.cend(), std::make_shared<tank::NormalTank>
        (info::TankInfo{
             .max_blood = 500,
             .name = "Tank " + std::to_string(next_id),
             .id = next_id,
             .type = info::TankType::NORMAL,
             .bullet =
             info::BulletInfo
                 {
                     .blood = 1,
                     .lethality = 120,
                     .range = 10000,
                 }},
         map, bullets, get_random_pos()));
    ++next_id;
    return next_id - 1;
  }
  
  std::size_t Game::add_auto_tank(std::size_t level)
  {
    id_index[next_id] = tanks.size();
    tanks.emplace_back(
        std::make_shared<tank::AutoTank>(
            info::TankInfo{
                .max_blood = static_cast<int>((11 - level) * 100),
                .name = "AutoTank " + std::to_string(next_id),
                .id = next_id,
                .level = level,
                .type = info::TankType::AUTO,
                .bullet =
                info::BulletInfo
                    {
                        .blood = 1,
                        .lethality = static_cast<int>((11 - level) * 10),
                        .range = 10000
                    }}, map, bullets, get_random_pos()));
    ++next_id;
    return next_id - 1;
  }
  
  Game &Game::revive(std::size_t id)
  {
    id_at(id)->revive(get_random_pos());
    return *this;
  }
  
  
  Game &Game::tank_react(std::size_t id, tank::NormalTankEvent event)
  {
    if (curr_page != Page::GAME || !id_at(id)->is_alive())
    {
      return *this;
    }
    normal_tank_events.emplace_back(id, event);
    return *this;
  }
  
  auto Game::find_tank_nocheck(std::size_t i, std::size_t j)
  {
    return std::find_if(tanks.begin(), tanks.end(),
                        [i, j](const std::shared_ptr<tank::Tank> &b)
                        {
                          return (b->is_alive() && b->get_pos().get_x() == i && b->get_pos().get_y() == j);
                        });
  }
  
  auto Game::find_tank(std::size_t i, std::size_t j)
  {
    auto it = find_tank_nocheck(i, j);
    tank_assert(it != tanks.end());
    return it;
  }
  
  Game &Game::react(Event event)
  {
    switch (event)
    {
      case Event::PASS:
        break;
      case Event::START:
        curr_page = Page::GAME;
        output_inited = false;
        break;
      case Event::PAUSE:
        curr_page = Page::TANK_STATUS;
        output_inited = false;
        break;
      case Event::COMMAND:
        curr_page = Page::COMMAND;
        output_inited = false;
        break;
      case Event::CONTINUE:
        curr_page = Page::GAME;
        output_inited = false;
        break;
    }
    
    switch(curr_page)
    {
      case Page::GAME:
        for (auto &ntevent: normal_tank_events)
        {
          auto tank = tanks[ntevent.first];
          switch (ntevent.second)
          {
            case tank::NormalTankEvent::UP:
              tank->up();
              break;
            case tank::NormalTankEvent::DOWN:
              tank->down();
              break;
            case tank::NormalTankEvent::LEFT:
              tank->left();
              break;
            case tank::NormalTankEvent::RIGHT:
              tank->right();
              break;
            case tank::NormalTankEvent::FIRE:
              tank->fire();
              break;
          }
        }
        normal_tank_events.clear();
        //auto tank
        for (auto it = tanks.begin(); it < tanks.end(); ++it)
        {
          if (!(*it)->is_alive() || !(*it)->is_auto()) continue;
          auto tank = std::dynamic_pointer_cast<tank::AutoTank>(*it);
          auto target = id_at(tank->get_target_id());
          //no target or target is dead/cleared should target/retarget
          if (target == nullptr || !tank->get_found() || !target->is_alive())
          {
            auto alive = get_alive(it - tanks.begin());
            if (alive.empty()) continue;
            do
            {
              auto t = alive[map::random(0, static_cast<int>(alive.size()))];
              auto p = tanks[t];
              tank->target(p->get_id(), p->get_pos());
            } while (!tank->get_found());
            target = id_at(tank->get_target_id());
          }
      
          //correct its way
          bool in_firing_line = tank::is_in_firing_line(map, tank->get_pos(), target->get_pos());
          if (
            // arrived but not in firing line
              (tank->has_arrived() && !in_firing_line)
              // in firing line but not arrived
              || (in_firing_line && !tank->has_arrived()))
          {
            tank->target(tank->get_target_id(), target->get_pos());
          }
          int ret = 0;
          switch (tank->next())
          {
            case tank::AutoTankEvent::UP:
              ret = tank->up();
              break;
            case tank::AutoTankEvent::DOWN:
              ret = tank->down();
              break;
            case tank::AutoTankEvent::LEFT:
              ret = tank->left();
              break;
            case tank::AutoTankEvent::RIGHT:
              ret = tank->right();
              break;
            case tank::AutoTankEvent::FIRE:
              tank->fire();
              break;
            case tank::AutoTankEvent::PASS:
              break;
          }
          if (ret != 0) tank->stuck();
          else tank->no_stuck();
        }
        // bullet move
        for (auto it = bullets->begin(); it < bullets->end(); ++it)
        {
          if ((map->count(map::Status::BULLET, it->get_pos()) > 1)
              || map->has(map::Status::TANK, it->get_pos())
              || (it->is_alive() && it->move() != 0)
              // bullet has moved
              || map->has(map::Status::TANK, it->get_pos()))
          {
            int lethality = 0;
            for_all_bullets(it->get_pos().get_x(), it->get_pos().get_y(),
                            [&lethality](const std::vector<bullet::Bullet>::iterator &it)
                            {
                              if(it->is_alive())
                                lethality += it->get_lethality();
                              it->kill();
                            });
            map->remove_status(map::Status::BULLET, it->get_pos());
            if (map->has(map::Status::TANK, it->get_pos()))
            {
              auto tank = find_tank(it->get_pos().get_x(), it->get_pos().get_y());
              (*tank)->attacked(lethality);
              if (!(*tank)->is_alive())
              {
                CZH_NOTICE((*tank)->get_name() + " was killed.");
                map->remove_status(map::Status::TANK, it->get_pos());
                (*tank)->clear();
              }
            }
          }
        }
        clear_death();
        break;
      case Page::MAIN:
        break;
      case Page::COMMAND:
        break;
      case Page::TANK_STATUS:
        break;
      case Page::HELP:
        break;
    }
    paint();
    return *this;
  }
  
  [[nodiscard]]Page Game::get_page() const {return curr_page;}
  
  
  [[nodiscard]]std::vector<std::size_t> Game::get_alive(std::size_t except) const
  {
    std::vector<std::size_t> ret;
    for (std::size_t i = 0; i < tanks.size(); ++i)
    {
      if (tanks[i]->is_alive() && i != except)
      {
        ret.emplace_back(i);
      }
    }
    return ret;
  }
  
  
  void Game::clear_death()
  {
    bullets->erase(std::remove_if(bullets->begin(), bullets->end(),
                                  [this](bullet::Bullet &bullet)
                                  {
                                    if (!bullet.is_alive())
                                    {
                                      map->remove_status(map::Status::BULLET, bullet.get_pos());
                                      return true;
                                    }
                                    return false;
                                  }), bullets->end());
    for (auto it = tanks.begin(); it < tanks.end(); ++it)
    {
      auto tank = *it;
      if (!tank->is_alive() && !tank->has_cleared())
      {
        map->remove_status(map::Status::TANK, tank->get_pos());
        tank->clear();
      }
    }
  }
  
  map::Pos Game::get_random_pos()
  {
    map::Pos pos;
    do
    {
      pos = map::Pos(map::random(1, static_cast<int>(map->get_width()) - 1),
                     map::random(1, static_cast<int>(map->get_height()) - 1));
    } while (find_tank_nocheck(pos.get_x(), pos.get_y()) != tanks.end()
             || map->has(map::Status::WALL, pos));
    return pos;
  }
  
  void Game::update(const map::Pos &pos)
  {
    term::move_cursor({pos.get_x(), map->get_height() - pos.get_y() - 1});
    if (map->has(map::Status::TANK, pos))
    {
      auto it = find_tank(pos.get_x(), pos.get_y());
      term::output((*it)->colorify_tank());
    }
    else if (map->has(map::Status::BULLET, pos))
    {
      auto w = find_bullet(pos.get_x(), pos.get_y());
      term::output(w->get_from()->colorify_text(w->get_text()));
    }
    else if (map->has(map::Status::WALL, pos))
    {
      term::output("\033[0;41;37m \033[0m\033[?25l");
    }
    else
    {
      term::output(" ");
    }
  }
  
  void Game::paint()
  {
    if (screen_height != term::get_height() || screen_width != term::get_width())
    {
      term::clear();
      output_inited = false;
      screen_height = term::get_height();
      screen_width = term::get_width();
    }
    switch (curr_page)
    {
      case Page::GAME:
        if (!output_inited)
        {
          term::clear();
          term::move_cursor({0, 0});
          for (int j = map->get_height() - 1; j >= 0; --j)
          {
            for (int i = 0; i < map->get_width(); ++i)
              update(map::Pos(i, j));
            term::output("\n");
          }
          output_inited = true;
        }
        else
        {
          for (auto &p: map->get_changes())
            update(p.get_pos());
          map->clear_changes();
        }
        break;
      case Page::TANK_STATUS:
        if (!output_inited)
        {
          term::clear();
          std::size_t cursor_y = 0;
          term::mvoutput({screen_width / 2 - 10, cursor_y++}, "Tank - by caozhanhao");
          size_t gap = 2;
          size_t id_x = gap;
          size_t name_x = id_x + gap + std::to_string((*std::max_element(tanks.begin(), tanks.end(),
                                                                         [](auto &&a, auto &&b)
                                                                         {
                                                                           return a->get_id() < b->get_id();
                                                                         }))->get_id()).size();
          auto pos_size = [](const map::Pos &p)
          {
            return std::to_string(p.get_x()).size() + std::to_string(p.get_y()).size() + 3;
          };
          size_t pos_x = name_x + gap + (*std::max_element(tanks.begin(), tanks.end(),
                                                           [](auto &&a, auto &&b)
                                                           {
                                                             return a->get_name().size() < b->get_name().size();
                                                           }))->get_name().size();
          size_t hp_x = pos_x + gap + pos_size((*std::max_element(tanks.begin(), tanks.end(),
                                                                  [&pos_size](auto &&a, auto &&b)
                                                                  {
                                                                    return pos_size(a->get_pos()) <
                                                                           pos_size(b->get_pos());
                                                                  }
          ))->get_pos());
        
          size_t lethality_x = hp_x + gap + std::to_string((*std::max_element(tanks.begin(), tanks.end(),
                                                                              [](auto &&a, auto &&b)
                                                                              {
                                                                                return a->get_blood() < b->get_blood();
                                                                              }))->get_blood()).size();
        
          size_t target_x = lethality_x + gap + +std::to_string((*std::max_element(tanks.begin(), tanks.end(),
                                                                                   [](auto &&a, auto &&b)
                                                                                   {
                                                                                     return
                                                                                         a->get_info().bullet.lethality
                                                                                         <
                                                                                         b->get_info().bullet.lethality;
                                                                                   }))->get_info().bullet.lethality).size();
        
          term::mvoutput({id_x, cursor_y}, "ID");
          term::mvoutput({name_x, cursor_y}, "Name");
          term::mvoutput({pos_x, cursor_y}, "Pos");
          term::mvoutput({hp_x, cursor_y}, "HP");
          term::mvoutput({lethality_x, cursor_y}, "ATK");
          term::mvoutput({target_x, cursor_y}, "Target");
        
          cursor_y++;
          for (int i = 0; i < tanks.size(); ++i)
          {
            auto tank = tanks[i];
            std::string x = std::to_string(tank->get_pos().get_x());
            std::string y = std::to_string(tank->get_pos().get_y());
            term::mvoutput({id_x, cursor_y}, std::to_string(tank->get_id()));
            term::mvoutput({name_x, cursor_y}, tank->colorify_text(tank->get_name()));
            term::mvoutput({pos_x, cursor_y}, "(" + x + "," + y + ")");
            term::mvoutput({hp_x, cursor_y}, std::to_string(tank->get_blood()));
            term::mvoutput({lethality_x, cursor_y}, std::to_string(tank->get_info().bullet.lethality));
            if (tank->is_auto())
            {
              auto at = std::dynamic_pointer_cast<tank::AutoTank>(tank);
              std::string target_name;
              auto target = id_at(at->get_target_id());
              if (target != nullptr)
                target_name = target->colorify_text(target->get_name());
              else
                target_name = "Cleared";
              term::mvoutput({target_x, cursor_y}, target_name);
            }
            cursor_y++;
            if (cursor_y == screen_height - 1)
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
          output_inited = true;
        }
        break;
      case Page::MAIN:
      {
        if (!output_inited)
        {
          constexpr std::string_view tank = R"(
 _____           _
|_   _|_ _ _ __ | | __
  | |/ _` | '_ \| |/ /
  | | (_| | | | |   <
  |_|\__,_|_| |_|_|\_\
)";
          static const auto s = utils::split<std::vector<std::string_view>>(tank, "\n");
          size_t x = screen_width / 2 - 12;
          size_t y = 2;
          term::clear();
          for (size_t i = 0; i < s.size(); ++i)
            term::mvoutput({x, y++}, std::string(s[i]));
          term::mvoutput({x + 5, y + 3}, ">>> Enter <<<");
          term::mvoutput({x + 1, y + 4}, "Use '/help' to get help.");
          output_inited = true;
        }
      }
        break;
      case Page::HELP:
        static const std::string help =
            R"(
Keys:
  Move: WASD
  Attack: space
  All tanks' status: 'o' or 'O'
  Command: '/'

Rules:
  User's Tank:
    Blood: 500, Lethality: 120
  Auto Tank:
    Blood: (11 - level) * 100, Lethality: (11 - level) * 10
    The higher level, the faster it moves and attack.
    
Command:
  help [page]
    - Get this help.
    - Use 'Enter' to return game.

  quit
    - Quit Tank.
  
  reshape [width, height]
    - Reshape the game map to the given size.
    - Default to reshape to the screen's size

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
    - Default to revive all tanks.
    
  summon [n] [level]
    - Summon n tanks with the given level.
    - e.g. summon 50 10
    
  kill [A id]
    - Kill A.
    - Default to kill all tanks.

  clear [A id]
    - Clear A.(auto tank only)
    - Default to clear all auto tanks.
  clear death
    - Clear all the died auto tanks
    Note:
       Clear is to delete rather than to kill.
       So can't clear the user's tank and cleared tanks can't be revived.
       And the bullets of the cleared tank will also be cleared.

  set [A id] [key] [value]
    - Set A's attribute below:
      - max_blood (int): Max blood of A. This will take effect when A is revived.
      - blood (int): Blood of A. This takes effect immediately but won't last when A is revived.
      - target (id, int): Auto Tank's target. Target should be alive.
      - name (string): Name of A.
  set [A id] bullet [key] [value]
      - blood (int): Blood of A's bullet.
      - Whe a bullet hits the wall, its blood decreases by one. That means it will bounce Blood times.
      - lethality (int): Lethality of A's bullet. This can be a negative number, in which case blood will be added.
      - range (int): Range of A's bullet.
      - e.g. set 0 max_blood 1000  |  set 0 bullet lethality 10
)";
        static const auto s = utils::split<std::vector<std::string_view>>(help, "\n");
        {
          size_t page_size = term::get_height() - 3;
          if (!output_inited)
          {
            std::size_t cursor_y = 0;
            term::mvoutput({screen_width / 2 - 10, cursor_y++}, "Tank - by caozhanhao");
            if((help_page - 1) * page_size > s.size()) help_page = 1;
            for(size_t i = (help_page - 1) * page_size; i < std::min(help_page * page_size, s.size()); ++i)
              term::mvoutput({0, cursor_y++}, std::string(s[i]));
            term::mvoutput({screen_width / 2 - 3, cursor_y}, "Page " + std::to_string(help_page));
            output_inited = true;
          }
        }
          break;
      case Page::COMMAND:
        if (!output_inited)
        {
          if (cmd_string[cmd_string_pos] == 1)
          {
            cmd_string.erase(cmd_string_pos , 1);
            --cmd_string_pos;
            cmd_string = history[history_pos];
            cmd_string_pos = cmd_string.size() - 1;
            if (history_pos != 0) --history_pos;
          }
          else if (cmd_string[cmd_string_pos] == 2)
          {
            cmd_string.erase(cmd_string_pos , 1);
            --cmd_string_pos;
            cmd_string = history[history_pos];
            cmd_string_pos = cmd_string.size() - 1;
            if (history_pos + 1 < history.size()) ++history_pos;
          }
          else if (cmd_string[cmd_string_pos] == 4)
          {
            cmd_string.erase(cmd_string_pos, 1);
            --cmd_string_pos;
            if(cmd_string_pos != 0)
              --cmd_string_pos;
          }
          else if (cmd_string[cmd_string_pos] == 5)
          {
            cmd_string.erase(cmd_string_pos, 1);
            --cmd_string_pos;
            if(cmd_string_pos + 1 < cmd_string.size())
              ++cmd_string_pos;
          }
          else if (cmd_string[cmd_string_pos] == 6)
          {
            cmd_string.erase(cmd_string_pos , 1);
            --cmd_string_pos;
            if(cmd_string_pos != 0)
            {
              cmd_string.erase(cmd_string_pos, 1);
              --cmd_string_pos;
            }
          }
          else if (cmd_string[cmd_string_pos] == 7)
          {
            cmd_string.erase(cmd_string_pos, 1);
            --cmd_string_pos;
            if (cmd_string_pos + 1 != cmd_string.size())
              cmd_string.erase(cmd_string_pos + 1, 1);
          }
          else if (cmd_string[cmd_string_pos] == 8)
          {
            cmd_string.erase(cmd_string_pos, 1);
            --cmd_string_pos;
            cmd_string_pos = 0;
          }
          else if (cmd_string[cmd_string_pos] == 9)
          {
            cmd_string.erase(cmd_string_pos, 1);
            --cmd_string_pos;
            cmd_string_pos = cmd_string.size() - 1;
          }
          czh::logger::output_at_bottom(cmd_string);
          output_inited = true;
        }
        break;
    }
  }
  
  std::vector<bullet::Bullet>::iterator Game::find_bullet(std::size_t i, std::size_t j)
  {
    auto it = std::find_if(bullets->begin(), bullets->end(),
                           [i, j](const bullet::Bullet &b)
                           {
                             return (b.get_pos().get_x() == i && b.get_pos().get_y() == j);
                           });
    tank_assert(it != bullets->end());
    return it;
  }
  
  void Game::for_all_bullets(std::size_t i, std::size_t j,
                             const std::function<void(std::vector<bullet::Bullet>::iterator &)> &func)
  {
    for (auto it = bullets->begin(); it < bullets->end(); ++it)
    {
      if (it->get_pos().get_x() == i && it->get_pos().get_y() == j)
      {
        func(it);
      }
    }
  }
  
  void Game::receive_char(char c)
  {
    if(c != 3)
    {
      cmd_string_pos++;
      cmd_string.insert(cmd_string.begin() + cmd_string_pos, c);
      output_inited = false;
    }
    else
    {
      run_command(cmd_string);
      history.emplace_back(cmd_string);
      cmd_string = "/";
      cmd_string_pos = 0;
      history_pos = history.size() - 1;
    }
  }
  
  void Game::run_command(const std::string &str)
  {
    curr_page = Page::GAME;
    auto[name, args_internal] = cmd::parse(str);
    try
    {
      if (name == "help")
      {
        term::clear();
        curr_page = Page::HELP;
        output_inited = false;
        if (args_internal.empty())
          help_page = 1;
        else
          help_page = std::get<0>(cmd::args_get<int>(args_internal));
        return;
      }
      else if (name == "quit")
      {
        if(!args_internal.empty())
        {
          CZH_NOTICE("Invalid range.");
          return;
        }
        term::move_cursor({0, map->get_height() + 1});
        term::output("\033[?25h");
        CZH_NOTICE("Quitting.");
        std::exit(0);
      }
      else if (name == "reshape")
      {
        if(args_internal.empty())
        {
          *map = map::Map((term::get_height() - 1) % 2 == 0 ? term::get_height() - 2 : term::get_height() - 1,
                          term::get_width() % 2 == 0 ? term::get_width() - 1 : term::get_width());
        }
        else
        {
          auto[width, height] = cmd::args_get<int, int>(args_internal);
          *map = map::Map((height - 1) % 2 == 0 ? height - 2 : height - 1,
                          width % 2 == 0 ? width - 1 : width);
        }
        bullets->clear();
        for(auto& r : tanks)
        {
          if(r->is_alive())
          {
            auto pos =  get_random_pos();
            r->get_pos() = pos;
            map->add_tank(pos);
          }
        }
        output_inited = false;
        paint();
        return;
      }
      else if (name == "clear_maze")
      {
        if(!args_internal.empty())
        {
          CZH_NOTICE("Invalid range.");
          return;
        }     map->clear_maze();
        return;
      }
      else if (name == "fill")
      {
        map::Pos from;
        map::Pos to;
        int is_wall = 0;
        if (cmd::args_is<int, int>(args_internal))
        {
          auto args = cmd::args_get<int, int, int>(args_internal);
          is_wall = std::get<0>(args);
          from.get_x() = std::get<1>(args);
          from.get_y() = std::get<2>(args);
          to = from;
        }
        else
        {
          auto args = cmd::args_get<int, int, int, int, int>(args_internal);
          is_wall = std::get<0>(args);
          from.get_x() = std::get<1>(args);
          from.get_y() = std::get<2>(args);
          to.get_x() = std::get<3>(args);
          to.get_y() = std::get<4>(args);
        }
        if (!map->check_pos(from) || !map->check_pos(from))
        {
          CZH_NOTICE("Invalid range.");
          return;
        }
  
        size_t bx = std::max(from.get_x(), to.get_x());
        size_t sx = std::min(from.get_x(), to.get_x());
        size_t by = std::max(from.get_y(), to.get_y());
        size_t sy = std::min(from.get_y(), to.get_y());
        
        for (size_t i = sx; i <= bx; ++i)
        {
          for (size_t j = sy; j <= by; ++j)
          {
            if (map->has(map::Status::TANK, {i, j}))
            {
              auto t = *find_tank(i, j);
              t->kill();
              map->remove_status(map::Status::TANK, {i, j});
              t->clear();
            }
            else if (map->has(map::Status::BULLET, {i, j}))
            {
              find_bullet(i, j)->kill();
              map->remove_status(map::Status::BULLET, {i, j});
            }
          }
        }
        if(is_wall)
          map->fill(from, to, map::Status::WALL);
        else
          map->fill(from, to);
        CZH_NOTICE("Filled from ("
                   + std::to_string(from.get_x()) + "," + std::to_string(from.get_y()) + ") to ("
                   + std::to_string(to.get_x()) + "," + std::to_string(to.get_y()) + ")."
        );
        return;
      }
      else if (name == "tp")
      {
        int id = -1;
        map::Pos to_pos;
        auto check = [this](const map::Pos &p)
        {
          return map->check_pos(p) && !map->has(map::Status::WALL, p) && !map->has(map::Status::TANK, p);
        };
      
        if (cmd::args_is<int, int>(args_internal))
        {
          auto args = cmd::args_get<int, int>(args_internal);
          id = std::get<0>(args);
          int to_id = std::get<1>(args);
          if (id_at(to_id) == nullptr || !id_at(to_id)->is_alive())
          {
            CZH_NOTICE("Invalid target tank.");
            return;
          }
          auto pos = id_at(to_id)->get_pos();
          map::Pos pos_up(pos.get_x(), pos.get_y() + 1);
          map::Pos pos_down(pos.get_x(), pos.get_y() - 1);
          map::Pos pos_left(pos.get_x() - 1, pos.get_y());
          map::Pos pos_right(pos.get_x() + 1, pos.get_y());
          if (check(pos_up)) to_pos = pos_up;
          else if (check(pos_down)) to_pos = pos_down;
          else if (check(pos_left)) to_pos = pos_left;
          else if (check(pos_right)) to_pos = pos_right;
          else
          {
            CZH_NOTICE("Target pos has no space.");
            return;
          }
        }
        else if (cmd::args_is<int, int, int>(args_internal))
        {
          auto args = cmd::args_get<int, int, int>(args_internal);
          id = std::get<0>(args);
          to_pos.get_x() = std::get<1>(args);
          to_pos.get_y() = std::get<2>(args);
          if (!check(to_pos))
          {
            CZH_NOTICE("Target pos has no space.");
            return;
          }
        }
        else
        {
          CZH_NOTICE("Invalid arguments");
          return;
        }
      
        if (id_at(id) == nullptr || !id_at(id)->is_alive())
        {
          CZH_NOTICE("Invalid tank");
          return;
        }
      
        map->remove_status(map::Status::TANK, id_at(id)->get_pos());
        map->add_tank(to_pos);
        id_at(id)->get_pos() = to_pos;
        CZH_NOTICE(id_at(id)->get_name() + " has been teleported to ("
                   + std::to_string(to_pos.get_x()) + "," + std::to_string(to_pos.get_y()) + ").");
        return;
      }
      else if (name == "revive")
      {
        if (args_internal.empty())
        {
          for (auto &r: tanks)
          {
            if (!r->is_alive()) revive(r->get_id());
          }
          CZH_NOTICE("Revived all tanks.");
          return;
        }
        else
        {
          auto[id] = cmd::args_get<int>(args_internal);
          if (id_at(id) == nullptr)
          {
            CZH_NOTICE("Invalid tank");
            return;
          }
          revive(id);
          CZH_NOTICE(id_at(id)->get_name() + " revived.");
          return;
        }
      }
      else if (name == "summon")
      {
        auto[num, lvl] = cmd::args_get<int, int>(args_internal);
        if (num <= 0 || lvl > 10 || lvl < 1)
        {
          CZH_NOTICE("Invalid num/lvl.");
          return;
        }
        for (size_t i = 0; i < num; ++i)
          add_auto_tank(lvl);
        CZH_NOTICE("Added " + std::to_string(num) + " AutoTanks, Level: " + std::to_string(lvl) + ".");
        return;
      }
      else if (name == "kill")
      {
        if (args_internal.empty())
        {
          for (auto &r: tanks)
          {
            if (r->is_alive()) r->kill();
          }
          clear_death();
          CZH_NOTICE("Killed all tanks.");
          return;
        }
        else
        {
          auto[id] = cmd::args_get<int>(args_internal);
          if (id_at(id) == nullptr)
          {
            CZH_NOTICE("Invalid tank.");
            return;
          }
          auto t = id_at(id);
          t->kill();
          map->remove_status(map::Status::TANK, t->get_pos());
          t->clear();
          CZH_NOTICE(t->get_name() + " has been killed.");
          return;
        }
      }
      else if (name == "clear")
      {
        if (args_internal.empty())
        {
          for (auto &r: *bullets)
          {
            if (r.get_from()->is_auto())
              r.kill();
          }
          for (auto &r: tanks)
          {
            if (r->is_auto())
            {
              r->kill();
            }
          }
          clear_death();
          tanks.erase(std::remove_if(tanks.begin(), tanks.end(), [](auto &&i) { return i->is_auto(); }), tanks.end());
          id_index.clear();
          CZH_NOTICE("Cleared all tanks.");
        }
        else if (cmd::args_is<std::string>(args_internal))
        {
          auto[d] = cmd::args_get<std::string>(args_internal);
          if (d == "death")
          {
            for (auto &r: *bullets)
            {
              if (r.get_from()->is_auto() && !r.get_from()->is_alive())
                r.kill();
            }
            for (auto &r: tanks)
            {
              if (r->is_auto() && !r->is_alive())
                r->kill();
            }
            clear_death();
            tanks.erase(
                std::remove_if(tanks.begin(), tanks.end(), [](auto &&i) { return i->is_auto() && !i->is_alive(); }),
                tanks.end());
            id_index.clear();
            CZH_NOTICE("Cleared all died tanks.");
          }
          else
          {
            CZH_NOTICE("Invalid arguments.");
            return;
          }
        }
        else
        {
          auto[id] = cmd::args_get<int>(args_internal);
          if (id_at(id) == nullptr || id == 0)
          {
            CZH_NOTICE("Invalid tank.");
            return;
          }
          for (auto &r: *bullets)
          {
            if (r.get_from()->get_id() == id)
            {
              r.kill();
            }
          }
          auto t = id_at(id);
          t->kill();
          map->remove_status(map::Status::TANK, t->get_pos());
          t->clear();
          tanks.erase(tanks.begin() + id);
          id_index.erase(id);
          CZH_NOTICE("ID: " + std::to_string(id) + " was cleared.");
        }
        // make id_index
        for (size_t i = 0; i < tanks.size(); ++i)
        {
          id_index[tanks[i]->get_id()] = i;
        }
      }
      else if (name == "set")
      {
        if (cmd::args_is<int, std::string, int>(args_internal))
        {
          auto[id, key, value] = cmd::args_get<int, std::string, int>(args_internal);
          if (id_at(id) == nullptr)
          {
            CZH_NOTICE("Invalid tank");
            return;
          }
          if (key == "max_blood")
          {
            id_at(id)->get_info().max_blood = value;
            CZH_NOTICE("The max_blood of " + id_at(id)->get_name()
                       + " has been set for " + std::to_string(value) + ".");
            return;
          }
          else if (key == "blood")
          {
            if (!id_at(id)->is_alive()) revive(id);
            id_at(id)->get_blood() = value;
            CZH_NOTICE("The blood of " + id_at(id)->get_name()
                       + " has been set for " + std::to_string(value) + ".");
            return;
          }
          else if (key == "target")
          {
            if(id_at(value) == nullptr || !id_at(value)->is_alive())
            {
              CZH_NOTICE("Invalid target.");
              return;
            }
            if(!id_at(id)->is_auto())
            {
              CZH_NOTICE("Invalid auto tank.");
              return;
            }
            auto atank = std::dynamic_pointer_cast<tank::AutoTank>(id_at(id));
            atank->target(value, id_at(value)->get_pos());
            CZH_NOTICE("The target of " + atank->get_name()
                       + " has been set for " + std::to_string(value) + ".");
            return;
          }
          else
          {
            CZH_NOTICE("Invalid option.");
            return;
          }
        }
        else if (cmd::args_is<int, std::string, std::string>(args_internal))
        {
          auto[id, key, value] = cmd::args_get<int, std::string, std::string>(args_internal);
          if (id_at(id) == nullptr)
          {
            CZH_NOTICE("Invalid tank");
            return;
          }
          if (key == "name")
          {
            std::string old_name = id_at(id)->get_name();
            id_at(id)->get_name() = value;
            CZH_NOTICE("The name of " + old_name
                       + " has been set for '" + id_at(id)->get_name() + "'.");
            return;
          }
          else
          {
            CZH_NOTICE("Invalid option.");
            return;
          }
        }
        else if (cmd::args_is<int, std::string, std::string, int>(args_internal))
        {
          auto[id, bullet, key, value] = cmd::args_get<int, std::string, std::string, int>(args_internal);
          if (id_at(id) == nullptr)
          {
            CZH_NOTICE("Invalid tank");
            return;
          }
          if (bullet != "bullet")
          {
            CZH_NOTICE("Invalid option.");
            return;
          }
          if (key == "blood")
          {
            id_at(id)->get_info().bullet.blood = value;
            CZH_NOTICE("The bullet blood of " + id_at(id)->get_name()
                       + " has been set for " + std::to_string(value) + ".");
            return;
          }
          else if (key == "lethality")
          {
            id_at(id)->get_info().bullet.lethality = value;
            CZH_NOTICE("The bullet lethality of " + id_at(id)->get_name()
                       + " has been set for " + std::to_string(value) + ".");
            return;
          }
          else if (key == "range")
          {
            id_at(id)->get_info().bullet.range = value;
            CZH_NOTICE("The bullet range of " + id_at(id)->get_name()
                       + " has been set for " + std::to_string(value) + ".");
            return;
          }
          else
          {
            CZH_NOTICE("Invalid bullet option.");
            return;
          }
        }
        else
        {
          CZH_NOTICE("Invalid arguments.");
          return;
        }
      }
      else
      {
        CZH_NOTICE("Invalid command.");
        return;
      }
    }
    // All the arguments whose types are wrong go here.
    catch (std::runtime_error &err)
    {
      if (std::string(err.what()) != "Get wrong type.") throw err;
      CZH_NOTICE("Invalid arguments.");
      return;
    }
  }
  
  std::shared_ptr<tank::Tank> Game::id_at(size_t id)
  {
    auto it = id_index.find(id);
    if(it == id_index.end()) return nullptr;
    return tanks[it->second];
  }
}