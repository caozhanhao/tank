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
#include <cassert>
#include <variant>
#include <vector>
#include <list>

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
  size_t tank_focus = 0;
  Zone rendered_zone {-10, 10, -10, 10};
  std::mutex render_mtx;
  std::mutex mainloop_mtx;
  std::size_t screen_height = term::get_height();
  std::size_t screen_width = term::get_width();
  map::Map game_map;
  std::map<std::size_t, tank::Tank*> tanks;
  std::list<bullet::Bullet*> bullets;
  std::vector<std::pair<std::size_t, tank::NormalTankEvent>> normal_tank_events;
  Page curr_page = Page::MAIN;
  size_t help_page = 0;
  size_t next_id = 0;
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
    auto it = tanks.find(id);
    if (it == tanks.end()) return nullptr;
    return it->second;
  }

  void tank_assert(bool a, const std::string &err)
  {
    if (!a)
      throw std::runtime_error(err);
  }
  
  std::size_t add_tank() {
      auto pos = get_available_pos();
      tank_assert(pos.has_value(), "No available space.");
      tanks.insert({next_id,  new tank::NormalTank
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
              }, *pos)});
      ++next_id;
      return next_id - 1;
  }
  
  std::size_t add_auto_tank(std::size_t lvl) {
      auto pos = get_available_pos();
      if (!pos.has_value()) {
          logger::error("No available space.");
          return 0;
      }
      tanks.insert({next_id,
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
                                            }}, *pos)});
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
    if(id == 0)
        tank_focus = 0;
    return;
  }
  
  [[nodiscard]]std::vector<std::size_t> get_alive()
  {
    std::vector<std::size_t> ret;
    for (std::size_t i = 0; i < tanks.size(); ++i)
    {
      if (tanks[i]->is_alive())
      {
        ret.emplace_back(i);
      }
    }
    return ret;
  }
  
  void clear_death() {
      for (auto it = game::bullets.begin(); it != game::bullets.end();) {
          if (!(*it)->is_alive()) {
              game_map.remove_status(map::Status::BULLET, (*it)->get_pos());
              delete *it;
              it = bullets.erase(it);
          } else
              ++it;
      }

      for (auto it = tanks.begin(); it != tanks.end(); ++it) {
          auto tank = *it;
          if (!tank.second->is_alive() && !tank.second->has_cleared()) {
              game_map.remove_status(map::Status::TANK, tank.second->get_pos());
              tank.second->clear();
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
  
  void mainloop() {
      if (curr_page == Page::GAME) {
          std::lock_guard<std::mutex> l(game::mainloop_mtx);
          for (auto it = tanks.begin(); it != tanks.end(); ++it) {
              //auto tank
              assert(it->second != nullptr);
              if (!it->second->is_alive() || !it->second->is_auto()) continue;
              auto tank = dynamic_cast<tank::AutoTank *>(it->second);
                  if (tank->has_arrived()) {
                      for (int i = tank->get_pos().x - 15; i < tank->get_pos().x + 15; ++i) {
                          for (int j = tank->get_pos().y - 15; j < tank->get_pos().y + 15; ++j) {
                              if (i == tank->get_pos().x && j == tank->get_pos().y)
                                  continue;

                              if (game_map.at(i, j).has(map::Status::TANK)) {
                                  auto t = game_map.at(i, j).get_tank_instance();
                                  assert(t != nullptr);
                                  if (t->is_alive()) {
                                      tank->target(t->get_id(), t->get_pos());
                                      break;
                                  }
                              }
                          }
                      }
                  }
              int ret = 0;
              auto e = tank->next();
              switch (e) {
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
          }
          // bullet move
          for (auto it = bullets.begin(); it != bullets.end(); ++it) {
              if ((*it)->is_alive())
                  (*it)->move();
          }

          for (auto it = bullets.begin();  it != bullets.end(); ++it) {
              if (!(*it)->is_alive()) continue;

              if ((game_map.count(map::Status::BULLET, (*it)->get_pos()) > 1)
                  || game_map.has(map::Status::TANK, (*it)->get_pos())) {
                   int lethality = 0;
                  int attacker = -1;
                  auto bullets_instance = game_map.at((*it)->get_pos()).get_bullets_instance();
                  assert(!bullets_instance.empty());
                  for (auto it = bullets_instance.begin(); it != bullets_instance.end(); ++it) {
                      if ((*it)->is_alive())
                          lethality += (*it)->get_lethality();
                      (*it)->kill();
                      attacker = (*it)->get_tank();
                  }

                  if (game_map.has(map::Status::TANK, (*it)->get_pos())) {
                      if (auto tank = game_map.at((*it)->get_pos()).get_tank_instance(); tank != nullptr) {
                          auto tank_attacker = id_at(attacker);
                          assert(tank_attacker != nullptr);
                          if (tank->is_auto()) {
                              auto t = dynamic_cast<tank::AutoTank *>(tank);
                              if (attacker != t->get_id()
                                  && map::get_distance(tank_attacker->get_pos(), tank->get_pos()) < 30) {
                                  t->target(attacker, tank_attacker->get_pos());
                              }
                          }
                          tank->attacked(lethality);
                          if(!tank->is_alive())
                              logger::info(tank->get_name() + " was killed by " + tank_attacker->get_name());
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
    if(point.is_generated() || point.is_active()) return;
    map::ActivePointData apd;
    if(point.has(map::Status::TANK)) {
        auto data = point.get_tank_data();
        auto it = tanks.insert({data.info.id, tank::build_tank(data)});
        assert(it.second);
        apd.tank = it.first->second;
    }
    if(point.has(map::Status::BULLET))
    {
      for(auto& r : point.get_bullets_data())
      {
        bullets.emplace_back(bullet::build_bullet(r));
        apd.bullets.emplace_back(bullets.back());
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
      auto it = tanks.find(tank->get_id());
      assert(it != tanks.end());
      delete tank;
      tanks.erase(it);
    }
    if(point.has(map::Status::BULLET))
    {
      for(auto& r : point.get_bullets_instance())
      {
        assert(r != nullptr);
        iapd.bullets.emplace_back(bullet::get_bullet_data(r));
        for(auto it = game::bullets.begin(); it != game::bullets.end();)
        {
          if((*it)->get_pos() == pos) {
              delete *it;
              it = game::bullets.erase(it);
          }
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
