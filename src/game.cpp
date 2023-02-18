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
#include <memory>
#include <cassert>

namespace czh::game
{
  void Game::enable_server()
  {
    as_server = true;
  }
  
  std::vector<map::Change> Game::get_changes()
  {
    running = false;
    return map->get_changes();
  }
  
  void Game::changes_applied()
  {
    curr_changes_apply++;
    if (curr_changes_apply == clients)
    {
      curr_changes_apply = 0;
      map->clear_changes();
      running = true;
    }
  }
  
  std::size_t Game::add_tank()
  {
    id_index[next_id] = tanks.size();
    tanks.insert(tanks.cend(), std::make_shared<tank::NormalTank>
        (info::TankInfo{
             .max_blood = 300,
             .name = "Tank " + std::to_string(next_id),
             .id = next_id,
             .type = info::TankType::NORMAL,
             .bullet =
             info::BulletInfo
                 {
                     .blood = 1,
                     .lethality = 60,
                     .circle = 0,
                     .range = 10000,
                 }},
         map, bullets, get_random_pos()));
    ++nalive_tank;
    ++clients;
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
                        .circle = 0,
                        .range = 10000
                    }}, map, bullets, get_random_pos()));
    ++nalive_tank;
    ++next_id;
    return next_id - 1;
  }
  
  Game &Game::revive(std::size_t id)
  {
    ++nalive_tank;
    id_at(id)->revive(get_random_pos());
    return *this;
  }
  
  
  Game &Game::tank_react(std::size_t id, tank::NormalTankEvent event)
  {
    if (!running || !id_at(id)->is_alive())
    {
      return *this;
    }
    normal_tank_events.emplace_back(id, event);
    return *this;
  }
  
  Game &Game::react(Event event)
  {
    bool command = false;
    switch (event)
    {
      case Event::PASS:
        break;
      case Event::PAUSE:
        running = false;
        output_inited = false;
        break;
      case Event::COMMAND:
        command = true;
        break;
      case Event::CONTINUE:
        running = true;
        output_inited = false;
        break;
    }
    if (command)
    {
      czh::logger::output_at_bottom("/");
      term::move_cursor(term::TermPos(1, term::get_height() - 1));
#if defined(__linux__)
      term::keyboard.deinit();
#endif
      std::string str;
      std::getline(std::cin, str);
#if defined(__linux__)
      term::keyboard.init();
#endif
      run_command(str);
      return *this;
    }
    if (!running)
    {
      paint();
      return *this;
    }
    //bullet move
    bool conflict = false;
    for (auto it = bullets->begin(); it < bullets->end(); ++it)
    {
      if (it->is_alive() && it->move() != 0)
      {
        conflict = true;
      }
    }
    //tank
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
    for (auto it = tanks.begin(); it < tanks.end() && !all_over(); ++it)
    {
      if (!(*it)->is_alive() || !(*it)->is_auto()) continue;
      auto tank = std::dynamic_pointer_cast<tank::AutoTank>(*it);
      auto target = id_at(tank->get_target_id());
      //hasn't found or target is not alive should target/retarget
      if (!tank->get_found() || !target->is_alive())
      {
        auto alive = get_alive(it - tanks.begin());
        if (alive.empty()) continue;
        do
        {
          auto t = alive[map::random(0, (int) alive.size())];
          auto p = tanks[t];
          tank->target(t, p->get_pos());
        } while (!tank->get_found());
        target = id_at(tank->get_target_id());
      }
      
      //correct its way
      bool should_correct = false;
      if (tank->has_arrived())
      {
        int x = (int) tank->get_pos().get_x() - (int) target->get_pos().get_x();
        int y = (int) tank->get_pos().get_y() - (int) target->get_pos().get_y();
        
        if (!tank::is_in_firing_line(map, tank->get_pos(), target->get_pos())// not in firing line
            || (x > 0 && tank->get_direction() != map::Direction::LEFT)
            || (x < 0 && tank->get_direction() != map::Direction::RIGHT)
            || (y > 0 && tank->get_direction() != map::Direction::DOWN)
            || (y < 0 && tank->get_direction() != map::Direction::UP))
        {
          should_correct = true;
        }
      }
      else if (!tank::is_in_firing_line(map, tank->get_target_pos(),
                                        target->get_pos()))
      {//target is not in firing line
        should_correct = true;
      }
      
      if (should_correct)
      {
        tank->target(tank->get_target_id(), target->get_pos());
      }
      switch (tank->next())
      {
        case tank::AutoTankEvent::UP:
          tank->up();
          break;
        case tank::AutoTankEvent::DOWN:
          tank->down();
          break;
        case tank::AutoTankEvent::LEFT:
          tank->left();
          break;
        case tank::AutoTankEvent::RIGHT:
          tank->right();
          break;
        case tank::AutoTankEvent::FIRE:
          tank->fire();
          break;
        case tank::AutoTankEvent::PASS:
          break;
      }
    }
    //conflict
    for (auto it = bullets->begin(); it < bullets->end(); ++it)
    {
      if (map->count(map::Status::BULLET, it->get_pos()) > 1
          || map->has(map::Status::TANK, it->get_pos()))
      {
        conflict = true;
        for_all_bullets(it->get_pos().get_x(), it->get_pos().get_y(),
                        [this](const std::vector<bullet::Bullet>::iterator &it)
                        {
                          for (std::size_t i = it->get_pos().get_x() - it->get_circle();
                               i <= it->get_pos().get_x() + it->get_circle(); ++i)
                          {
                            for (std::size_t j = it->get_pos().get_y() - it->get_circle();
                                 j <= it->get_pos().get_y() + it->get_circle(); ++j)
                            {
                              map->attacked(it->get_lethality(), {i, j});
                            }
                          }
                          it->kill();
                        });
      }
    }
    if (conflict)
    {
      for (auto it = tanks.begin(); it < tanks.end(); ++it)
      {
        auto tank = *it;
        int lethality = map->get_lethality(tank->get_pos());
        if (lethality == 0) continue;
        tank->attacked(lethality);
        if (tank->is_alive())
        {
          CZH_NOTICE(tank->get_name() + " was attacked.Blood: " + std::to_string(tank->get_blood()));
        }
        else
        {
          --nalive_tank;
          CZH_NOTICE(tank->get_name() + " was killed.");
        }
      }
      for (auto it = bullets->begin(); it < bullets->end(); ++it)
      {
        int lethality = map->get_lethality(it->get_pos());
        it->attacked(lethality);
        map->remove_lethality(it->get_pos());
      }
    }
    clear_death();
    paint();
    return *this;
  }
  
  [[nodiscard]]bool Game::is_running() const { return running; }
  
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
  
  [[nodiscard]]bool Game::all_over() const
  {
    return nalive_tank <= 1;
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
    assert(it != tanks.end());
    return it;
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
    if (running)
    {
      if (!output_inited)
      {
        term::clear();
        term::move_cursor({0, 0});
        for (int j = map->get_height() - 1; j >= 0; --j)
        {
          for (int i = 0; i < map->get_width(); ++i)
          {
            update(map::Pos(i, j));
          }
          term::output("\n");
        }
        output_inited = true;
      }
      else
      {
        for (auto &p: map->get_changes())
        {
          update(p.get_pos());
        }
        if (!as_server)
        {
          map->clear_changes();
        }
      }
    }
      //tank status
    else
    {
      if (!output_inited)
      {
        term::clear();
        std::size_t cursor_x = 0;
        std::size_t cursor_y = 0;
        term::mvoutput({screen_width / 2 - 10, cursor_y++}, "Tank - by caozhanhao");
        std::vector<std::size_t> max_x{0};
        std::size_t colpos = 0;
        for (int i = 0; i < tanks.size(); ++i)
        {
          if (cursor_y + 3 > screen_height)
          {
            colpos++;
            cursor_y = 1;
          }
          cursor_x = colpos == 0 ? 0 : max_x[colpos - 1];
          auto tank = tanks[i];
          //if (!tank->is_alive()) continue;
          std::string sout = "ID: " + std::to_string(tank->get_id()) + ", Name: "
                             + tank->colorify_text(tank->get_name());
          term::mvoutput({cursor_x, cursor_y++}, sout);
          std::string blood = std::to_string(tank->get_blood());
          std::string x = std::to_string(tank->get_pos().get_x());
          std::string y = std::to_string(tank->get_pos().get_y());
          sout.clear();
          sout.append("HP: ").append(blood)
              .append(" Pos: (").append(x).append(",").append(y).append(")");
          if (tank->is_auto())
          {
            auto at = std::dynamic_pointer_cast<tank::AutoTank>(tank);
            auto target = id_at(at->get_target_id());
            std::string level = std::to_string(at->get_level());
            level.insert(level.begin(), 2 - level.size(), '0');
            sout.append(" Level: ").append(level).append(" Target: ")
                .append(target->colorify_text(target->get_name()));
          }
          if (colpos == 0)
          {
            max_x[0] = std::max(max_x[0], sout.size());
          }
          else
          {
            max_x[colpos] = std::max(max_x[colpos], max_x[colpos - 1] + sout.size());
          }
          term::mvoutput({cursor_x, cursor_y++}, sout);
        }
        output_inited = true;
      }
    }
  }
  
  std::vector<bullet::Bullet>::iterator Game::find_bullet(std::size_t i, std::size_t j)
  {
    auto it = std::find_if(bullets->begin(), bullets->end(),
                           [i, j](const bullet::Bullet &b)
                           {
                             return (b.get_pos().get_x() == i && b.get_pos().get_y() == j);
                           });
    assert(it != bullets->end());
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
  
  
  void Game::run_command(const std::string &str)
  {
    std::vector<std::string> args{""};
    auto curr = args.begin();
    output_inited = false;
    paint();
    if (str.size() == 0) return;
    for (auto &r: str)
    {
      if (r == ' ')
      {
        curr = args.insert(args.end(), "");
        continue;
      }
      *curr += r;
    }
    bool failed = false;
    auto parse_int = [&args, &failed](size_t pos) -> int
    {
      if (pos >= args.size())
      {
        CZH_NOTICE("Needs at least " + std::to_string(pos) + " argument(s).");
        failed = true;
        return -1;
      }
      int ret;
      try
      {
        ret = std::stoi(args[pos]);
      }
      catch (...)
      {
        CZH_NOTICE("The " + std::to_string(pos) + "th arguments needs to be int.");
        failed = true;
        return -1;
      }
      return ret;
    };
    if (args[0] == "quit")
    {
      term::move_cursor({0, map->get_height() + 1});
      term::output("\033[?25h");
      CZH_NOTICE("Quitting.");
      std::exit(0);
    }
    else if (args[0] == "revive")
    {
      if (args.size() == 1)
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
        int id = parse_int(1);
        if (failed) return;
        if (id >= tanks.size())
        {
          CZH_NOTICE("Invalid arguments");
          return;
        }
        revive(id);
        CZH_NOTICE(id_at(id)->get_name() + " revived.");
        return;
      }
    }
    else if (args[0] == "kill")
    {
      if (args.size() == 1)
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
        int id = parse_int(1);
        if (failed) return;
        if (id >= tanks.size())
        {
          CZH_NOTICE("Invalid arguments.");
          return;
        }
        id_at(id)->kill();
        clear_death();
        CZH_NOTICE(id_at(id)->get_name() + " has been killed.");
        return;
      }
    }
    else if (args[0] == "clear")
    {
      if (args.size() == 1)
      {
        for (auto &r: *bullets)
        {
          if(r.get_from()->is_auto())
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
      else if(args.size() == 2 && args[1] == "death")
      {
        for (auto &r: *bullets)
        {
          if(r.get_from()->is_auto() && !r.get_from()->is_alive())
            r.kill();
        }
        for (auto &r: tanks)
        {
          if (r->is_auto() && !r->is_alive())
            r->kill();
        }
        clear_death();
        tanks.erase(std::remove_if(tanks.begin(), tanks.end(), [](auto &&i) { return i->is_auto() && !i->is_alive(); }), tanks.end());
        id_index.clear();
        CZH_NOTICE("Cleared all died tanks.");
      }
      else
      {
        int id = parse_int(1);
        if (failed) return;
        if (id >= tanks.size() || id == 0)
        {
          CZH_NOTICE("Invalid arguments");
          return;
        }
        for (auto &r: *bullets)
        {
          if (r.get_from()->get_id() == id)
          {
            r.kill();
          }
        }
        id_at(id)->kill();
        clear_death();
        tanks.erase(tanks.begin() + id);
        id_index.erase(id);
        CZH_NOTICE(id_at(id)->get_name() + " was cleared.");
      }
      // make index
      for (size_t i = 0; i < tanks.size(); ++i)
      {
        id_index[tanks[i]->get_id()] = i;
      }
    }
    else if (args[0] == "set")
    {
      if (args.size() < 4)
      {
        CZH_NOTICE("Needs at least 3 arguments.");
        return;
      }
      int id = parse_int(1);
      if (failed) return;
      if (id >= tanks.size())
      {
        CZH_NOTICE("Invalid arguments");
        return;
      }
      if (args[2] == "bullet")
      {
        if (args.size() != 5)
        {
          CZH_NOTICE("Needs key or value");
          return;
        }
        int value = parse_int(4);
        if (failed) return;
        if (args[3] == "blood")
        {
          id_at(id)->get_info().bullet.blood = value;
          CZH_NOTICE("The bullet blood of " + id_at(id)->get_name()
                     + " has been set for " + std::to_string(value) + ".");
          return;
        }
        else if (args[3] == "lethality")
        {
          id_at(id)->get_info().bullet.lethality = value;
          CZH_NOTICE("The bullet lethality of " + id_at(id)->get_name()
                     + " has been set for " + std::to_string(value) + ".");
          return;
        }
        else if (args[3] == "circle")
        {
          id_at(id)->get_info().bullet.circle = value;
          CZH_NOTICE("The bullet circle of " + id_at(id)->get_name()
                     + " has been set for " + std::to_string(value) + ".");
          return;
        }
        else if (args[3] == "range")
        {
          id_at(id)->get_info().bullet.range = value;
          CZH_NOTICE("The bullet range of " + id_at(id)->get_name()
                     + " has been set for " + std::to_string(value) + ".");
          return;
        }
        else
        {
          CZH_NOTICE("Invalid arguments.");
          return;
        }
      }
      else if (args[2] == "max_blood")
      {
        int value = parse_int(3);
        if (failed) return;
        id_at(id)->get_info().max_blood = value;
        CZH_NOTICE("The max_blood of " + id_at(id)->get_name()
                   + " has been set for " + std::to_string(value) + ".");
        return;
      }
      else if (args[2] == "blood")
      {
        int value = parse_int(3);
        if (failed) return;
        if(!id_at(id)->is_alive()) revive(id);
        id_at(id)->get_blood() = value;
        CZH_NOTICE("The blood of " + id_at(id)->get_name()
                   + " has been set for " + std::to_string(value) + ".");
        return;
      }
      else if (args[2] == "name")
      {
        std::string old_name = id_at(id)->get_name();
        id_at(id)->get_name() = args[3];
        CZH_NOTICE("The name of " + old_name
                   + " has been set for '" + id_at(id)->get_name() + "'.");
        return;
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
  
  std::shared_ptr<tank::Tank> Game::id_at(size_t id)
  {
    return tanks[id_index[id]];
  }
}