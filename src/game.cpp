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
#include "tank/term.h"
#include "tank/game.h"
#include "tank/game_map.h"
#include "tank/command.h"
#include "tank/utils.h"
#include "tank/tank.h"
#include "tank/bullet.h"
#include "tank/logger.h"
#include <optional>
#include <mutex>

namespace czh::game
{
#if defined (CZH_TANK_KEYBOARD_MODE_1)
  int keyboard_mode = 1;
#else
  int keyboard_mode = 0;
#endif
  std::chrono::milliseconds tick(20);
  bool output_inited = false;
  bool map_size_changed = false;
  Zone rendered_zone {0,0,0,0};
  std::mutex render_mtx;
  std::mutex mainloop_mtx;
  std::size_t screen_height = term::get_height();
  std::size_t screen_width = term::get_width();
  map::Map game_map{INITIAL_MAP_WIDTH, INITIAL_MAP_HEIGHT};
  std::vector<tank::Tank *> tanks;
  std::vector<bullet::Bullet> bullets;
  std::vector<std::pair<std::size_t, tank::NormalTankEvent>> normal_tank_events;
  Page curr_page = Page::MAIN;
  size_t help_page = 0;
  size_t next_id = 0;
  std::map<std::size_t, std::size_t> id_index;
  std::vector<std::string> history;
  std::string cmd_string = "/";
  size_t history_pos = 0;
  size_t cmd_string_pos = 0;
  
  std::optional<map::Pos> get_available_pos()
  {
    size_t start_x = utils::randnum<size_t>(1, game_map.get_width() - 1);
    size_t start_y = utils::randnum<size_t>(1, game_map.get_height() - 1);
    for (size_t i = start_x; i < game_map.get_width(); ++i)
    {
      for (size_t j = start_y; j < game_map.get_height(); ++j)
      {
        if (!game_map.has(map::Status::WALL, {i, j}) && !game_map.has(map::Status::TANK, {i, j}))
          return map::Pos{i, j};
      }
    }
    for (int i = start_x; i >= 0; --i)
    {
      for (int j = start_y; j >= 0; --j)
      {
        if (!game_map.has(map::Status::WALL, {static_cast<size_t>(i), static_cast<size_t>(j)})
            && !game_map.has(map::Status::TANK, {static_cast<size_t>(i), static_cast<size_t>(j)}))
          return map::Pos{static_cast<size_t>(i), static_cast<size_t>(j)};
      }
    }
    return std::nullopt;
  }
  
  tank::Tank *id_at(size_t id)
  {
    auto it = id_index.find(id);
    if (it == id_index.end()) return nullptr;
    return tanks[it->second];
  }
  
  void tank_assert(bool a, const std::string &err)
  {
    if (!a)
      throw std::runtime_error(err);
  }
  
  std::size_t add_tank()
  {
    auto pos = get_available_pos();
    tank_assert(pos.has_value(), "No available space.");
    id_index[next_id] = tanks.size();
    tanks.insert(tanks.cend(), new tank::NormalTank
        (info::TankInfo{
            .max_hp = 10000,
            .name = "Tank " + std::to_string(next_id),
            .id = next_id,
            .type = info::TankType::NORMAL,
            .bullet = info::BulletInfo
                {
                    .hp = 1,
                    .lethality = 100,
                    .range = 10000,
                }
        }, *pos));
    ++next_id;
    return next_id - 1;
  }
  
  std::size_t add_auto_tank(std::size_t lvl)
  {
    auto pos = get_available_pos();
    if (!pos.has_value())
    {
      logger::error("No available space.");
      return 0;
    }
    id_index[next_id] = tanks.size();
    tanks.emplace_back(
        new tank::AutoTank(
            info::TankInfo{
                .max_hp = static_cast<int>(11 - lvl) * 150,
                .name = "AutoTank " + std::to_string(next_id),
                .id = next_id,
                .gap = static_cast<int>(10 - lvl),
                .type = info::TankType::AUTO,
                .bullet = info::BulletInfo
                    {
                        .hp = 1,
                        .lethality = static_cast<int>(11 - lvl) * 15,
                        .range = 10000
                    }}, *pos));
    ++next_id;
    return next_id - 1;
  }
  
  void revive(std::size_t id)
  {
    auto pos = get_available_pos();
    if (!pos.has_value())
    {
      logger::error("No available space");
      return;
    }
    id_at(id)->revive(*pos);
    return;
  }
  
  std::vector<tank::Tank *>::iterator find_tank_nocheck(std::size_t i, std::size_t j)
  {
    return std::find_if(tanks.begin(), tanks.end(),
                        [i, j](auto &&b)
                        {
                          return (b->is_alive() && b->get_pos().get_x() == i && b->get_pos().get_y() == j);
                        });
  }
  
  std::vector<tank::Tank *>::iterator find_tank(std::size_t i, std::size_t j)
  {
    auto it = find_tank_nocheck(i, j);
    tank_assert(it != tanks.end());
    return it;
  }
  
  std::vector<bullet::Bullet>::iterator find_bullet(std::size_t i, std::size_t j)
  {
    auto it = std::find_if(bullets.begin(), bullets.end(),
                           [i, j](const bullet::Bullet &b)
                           {
                             return (b.get_pos().get_x() == i && b.get_pos().get_y() == j);
                           });
    tank_assert(it != bullets.end());
    return it;
  }
  
  void for_all_bullets(std::size_t i, std::size_t j,
                       const std::function<void(std::vector<bullet::Bullet>::iterator &)> &func)
  {
    for (auto it = bullets.begin(); it < bullets.end(); ++it)
    {
      if (it->get_pos().get_x() == i && it->get_pos().get_y() == j)
      {
        func(it);
      }
    }
  }
  
  [[nodiscard]]std::vector<std::size_t> get_alive(std::size_t except)
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
  
  void clear_death()
  {
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                                 [](bullet::Bullet &bullet)
                                 {
                                   if (!bullet.is_alive())
                                   {
                                     game_map.remove_status(map::Status::BULLET, bullet.get_pos());
                                     return true;
                                   }
                                   return false;
                                 }), bullets.end());
    for (auto it = tanks.begin(); it < tanks.end(); ++it)
    {
      auto tank = *it;
      if (!tank->is_alive() && !tank->has_cleared())
      {
        game_map.remove_status(map::Status::TANK, tank->get_pos());
        tank->clear();
      }
    }
  }
  
  void tank_react(std::size_t id, tank::NormalTankEvent event)
  {
    if (curr_page != Page::GAME || !id_at(id)->is_alive())
      return;
    auto tank = id_at(id);
    switch (event)
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
    return;
  }
  
  void mainloop()
  {
    if (curr_page == Page::GAME)
    {
      std::lock_guard<std::mutex> l(game::mainloop_mtx);
      for (auto it = tanks.begin(); it < tanks.end(); ++it)
      {
        //auto tank
        if (!(*it)->is_alive() || !(*it)->is_auto()) continue;
        auto tank = dynamic_cast<tank::AutoTank *>(*it);
        auto target = id_at(tank->get_target_id());
        bool in_firing_line = tank::is_in_firing_line(tank->get_pos(), target->get_pos());
        if (!tank->is_in_retreat())
        {//no target or target is dead/cleared should target/retarget
          if (target == nullptr || !tank->get_found() || !target->is_alive())
          {
            auto alive = get_alive(it - tanks.begin());
            if (alive.empty()) continue;
            do
            {
              auto t = alive[utils::randnum<size_t>(0, alive.size())];
              auto p = tanks[t];
              tank->target(p->get_id(), p->get_pos());
            } while (!tank->get_found());
            target = id_at(tank->get_target_id());
          }
          //correct its way
          if (in_firing_line && !tank->has_arrived())
            // in firing line but not arrived
          {
            tank->clear_way();
          }
        }
        auto e = tank->next();
        if (e == tank::AutoTankEvent::FIRE)
        {
          if (!in_firing_line)
            // fired but not in firing line
          {
            tank->target(tank->get_target_id(), target->get_pos());
            e = tank->next();
          }
          else
            tank->correct_direction(target->get_pos());
        }
        int ret = 0;
        switch (e)
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
      }
      // bullet move
      for (auto it = bullets.begin(); it < bullets.end(); ++it)
      {
        if (it->is_alive())
          it->move();
        if ((game_map.count(map::Status::BULLET, it->get_pos()) > 1)
            || game_map.has(map::Status::TANK, it->get_pos()))
        {
          int lethality = 0;
          tank::Tank *attacker;
          for_all_bullets(it->get_pos().get_x(), it->get_pos().get_y(),
                          [&lethality, &attacker](const std::vector<bullet::Bullet>::iterator &it)
                          {
                            if (it->is_alive())
                              lethality += it->get_lethality();
                            it->kill();
                            attacker = it->get_from();
                          });
          game_map.remove_status(map::Status::BULLET, it->get_pos());
          if (game_map.has(map::Status::TANK, it->get_pos()))
          {
            auto tank = find_tank(it->get_pos().get_x(), it->get_pos().get_y());
            if ((*tank)->is_auto())
            {
              auto t = dynamic_cast<tank::AutoTank *>(*tank);
              if (attacker->get_id() != t->get_id())
                t->target(attacker->get_id(), attacker->get_pos());
            }
            (*tank)->attacked(lethality);
            if (!(*tank)->is_alive())
            {
              logger::info((*tank)->get_name() + " was killed.");
              game_map.remove_status(map::Status::TANK, it->get_pos());
              (*tank)->clear();
            }
          }
        }
      }
      for (auto it = tanks.begin(); it < tanks.end(); ++it)
      {
        if ((*it)->tanks_nearby() == 4)
        {
          (*it)->kill();
          logger::info((*it)->get_name() + " was killed by collision.");
          game_map.remove_status(map::Status::TANK, (*it)->get_pos());
          (*it)->clear();
        }
      }
      clear_death();
    }
    return;
  }
}
