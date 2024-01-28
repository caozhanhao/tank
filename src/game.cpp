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
#include "tank/renderer.h"
#include <optional>
#include <mutex>
#include <cassert>
#include <variant>

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
  Zone rendered_zone {-10, 10, -10, 10};
  std::mutex render_mtx;
  std::mutex mainloop_mtx;
  std::size_t screen_height = term::get_height();
  std::size_t screen_width = term::get_width();
  map::Map game_map;
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
    std::vector<map::Pos> p;
    for (int i = game::rendered_zone.x_min; i < game::rendered_zone.x_max; ++i)
    {
      for (int j = game::rendered_zone.y_min; j < game::rendered_zone.y_max; ++j)
      {
        if (!game_map.has(map::Status::WALL, {i, j}) && !game_map.has(map::Status::TANK, {i, j}))
          p.emplace_back(map::Pos{i, j});
      }
    }
    if(p.empty())
      return std::nullopt;
    return p[utils::randnum<size_t>(0, p.size())];
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
                    .range = 30,
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
                        .range = 30
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
        bool in_firing_line = tank::is_in_firing_line(tank->get_info().bullet.range, tank->get_pos(),
                                                      target->get_pos());
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
          int attacker;
          auto bullets_instance = game_map.at(it->get_pos()).get_bullets_instance();
          for (auto it = bullets_instance.begin(); it < bullets_instance.end(); ++it)
          {
            if ((*it)->is_alive())
              lethality += (*it)->get_lethality();
            (*it)->kill();
            attacker = (*it)->get_tank();
          }
          if (game_map.has(map::Status::TANK, it->get_pos()))
          {
            if (auto tank = game_map.at(it->get_pos()).get_tank_instance(); tank != nullptr)
            {
              if (tank->is_auto())
              {
                auto t = dynamic_cast<tank::AutoTank *>(tank);
                if (attacker != t->get_id())
                {
                  if (auto tank_attacker = id_at(attacker); tank_attacker != nullptr)
                    t->target(attacker, tank_attacker->get_pos());
                }
              }
              tank->attacked(lethality);
            }
          }
        }
      }
      clear_death();
    }
    return;
  }
  
  void load_point(const map::Pos& pos)
  {
    auto point = game_map.at(pos);
    if(point.is_active()) return;
    map::ActivePointData apd;
    if(point.has(map::Status::TANK))
    {
      tanks.emplace_back(tank::build_tank(point.get_tank_data()));
      id_index[tanks.back()->get_id()] = tanks.size();
      apd.tank = tanks.back();
    }
    if(point.has(map::Status::BULLET))
    {
      for(auto& r : point.get_bullets_data())
      {
        bullets.emplace_back(bullet::build_bullet(r));
        apd.bullets.emplace_back(&bullets.back());
      }
    }
    point.activate(apd);
  }
  void load_point(int x, int y)
  {
    load_point(map::Pos(x, y));
  }
  void load_zone(const Zone& zone)
  {
    for(int x = zone.x_min; x < zone.x_max; ++x)
    {
      for(int y = zone.y_min; y < zone.y_max; ++y)
      {
        load_point(x, y);
      }
    }
  }
  
  void unload_point(const map::Pos& pos)
  {
    auto point = game_map.at(pos);
    if(!point.is_active()) return;
    map::InactivePointData iapd;
    if(point.has(map::Status::TANK))
    {
      auto tank = point.get_tank_instance();
      assert(tank != nullptr);
      iapd.tank = tank::get_tank_data(tank);
      auto it = id_index.find(tank->get_id());
      assert(it != id_index.end());
      delete tank;
      tanks.erase(tanks.begin() + it->second);
      id_index.erase(it);
    }
    if(point.has(map::Status::BULLET))
    {
      for(auto& r : point.get_bullets_instance())
      {
        assert(r != nullptr);
        iapd.bullets.emplace_back(bullet::get_bullet_data(*r));
        for(auto it = game::bullets.begin(); it != game::bullets.end();)
        {
          if(it->get_pos() == pos)
            it = game::bullets.erase(it);
          else
           ++it;
        }
      }
    }
    point.deactivate(iapd);
  }
  void unload_point(int x, int y)
  {
    unload_point(map::Pos(x, y));
  }
  void unload_zone(const Zone& zone)
  {
    for(int x = zone.x_min; x < zone.x_max; ++x)
    {
      for(int y = zone.y_min; y < zone.y_max; ++y)
      {
        unload_point(x, y);
      }
    }
  }
}
