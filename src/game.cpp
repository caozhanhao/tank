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
#include "tank/game_map.h"
#include "tank/command.h"
#include "tank/utils.h"
#include "tank/tank.h"
#include "tank/bullet.h"
#include "tank/globals.h"
#include <optional>
#include <mutex>
#include <vector>
#include <list>

namespace czh::g
{
  game::GameMode game_mode = game::GameMode::NATIVE;
  size_t user_id = 0;
  std::map<size_t, UserData> userdata{{0, UserData{.user_id = 0}}};
  std::chrono::milliseconds tick(16);
  std::chrono::milliseconds msg_ttl(2000);
  std::mutex mainloop_mtx;
  std::mutex tank_reacting_mtx;
  std::map<std::size_t, tank::Tank *> tanks;
  std::list<bullet::Bullet *> bullets;
  std::vector<std::pair<std::size_t, tank::NormalTankEvent>> normal_tank_events;
  size_t next_id = 0;
}

namespace czh::game
{
  std::optional<map::Pos> get_available_pos()
  {
    std::vector<map::Pos> p;
    for (int i = g::visible_zone.x_min; i < g::visible_zone.x_max; ++i)
    {
      for (int j = g::visible_zone.y_min; j < g::visible_zone.y_max; ++j)
      {
        if (!g::game_map.has(map::Status::WALL, {i, j}) && !g::game_map.has(map::Status::TANK, {i, j}))
        {
          p.emplace_back(map::Pos{i, j});
        }
      }
    }
    if (p.empty())
    {
      return std::nullopt;
    }
    return p[utils::randnum<size_t>(0, p.size())];
  }
  
  tank::Tank *id_at(size_t id)
  {
    auto it = g::tanks.find(id);
    if (it == g::tanks.end()) return nullptr;
    return it->second;
  }
  
  std::size_t add_tank(const map::Pos &pos)
  {
    g::tanks.insert({g::next_id, new tank::NormalTank
        (info::TankInfo{
            .max_hp = 10000,
            .name = "Tank " + std::to_string(g::next_id),
            .id = g::next_id,
            .type = info::TankType::NORMAL,
            .bullet = info::BulletInfo
                {
                    .hp = 1,
                    .lethality = 100,
                    .range = 30,
                }
        }, pos)});
    ++g::next_id;
    return g::next_id - 1;
  }
  
  std::size_t add_tank()
  {
    auto pos = get_available_pos();
    utils::tank_assert(pos.has_value(), "No available space.");
    return add_tank(*pos);
  }
  
  std::size_t add_auto_tank(std::size_t lvl, const map::Pos &pos)
  {
    g::tanks.insert({g::next_id,
                     new tank::AutoTank(
                         info::TankInfo{
                             .max_hp = static_cast<int>(11 - lvl) * 150,
                             .name = "AutoTank " + std::to_string(g::next_id),
                             .id = g::next_id,
                             .gap = static_cast<int>(10 - lvl),
                             .type = info::TankType::AUTO,
                             .bullet = info::BulletInfo
                                 {
                                     .hp = 1,
                                     .lethality = static_cast<int>(11 - lvl) * 15,
                                     .range = 30
                                 }}, pos)});
    ++g::next_id;
    return g::next_id - 1;
  }
  
  std::size_t add_auto_tank(std::size_t lvl)
  {
    auto pos = get_available_pos();
    if (!pos.has_value())
    {
      msg::error(g::user_id, "No available space.");
      return 0;
    }
    return add_auto_tank(lvl, *pos);
  }
  
  void revive(std::size_t id)
  {
    auto pos = get_available_pos();
    if (!pos.has_value())
    {
      msg::error(g::user_id, "No available space");
      return;
    }
    id_at(id)->revive(*pos);
    if (id == 0)
    {
      g::tank_focus = 0;
    }
  }
  
  [[nodiscard]]std::vector<std::size_t> get_alive()
  {
    std::vector<std::size_t> ret;
    for (std::size_t i = 0; i < g::tanks.size(); ++i)
    {
      if (g::tanks[i]->is_alive())
      {
        ret.emplace_back(i);
      }
    }
    return ret;
  }
  
  void clear_death()
  {
    for (auto it = g::bullets.begin(); it != g::bullets.end();)
    {
      if (!(*it)->is_alive())
      {
        g::game_map.remove_status(map::Status::BULLET, (*it)->get_pos());
        delete *it;
        it = g::bullets.erase(it);
      }
      else
      {
        ++it;
      }
    }
    
    for (auto it = g::tanks.begin(); it != g::tanks.end(); ++it)
    {
      auto tank = *it;
      if (!tank.second->is_alive() && !tank.second->has_cleared())
      {
        tank.second->clear();
      }
    }
  }
  
  void tank_react(std::size_t id, tank::NormalTankEvent event)
  {
    std::lock_guard<std::mutex> l(g::tank_reacting_mtx);
    if (id_at(id)->is_alive())
    {
      g::normal_tank_events.emplace_back(std::make_pair(id, event));
    }
  }
  
  void mainloop()
  {
    std::lock_guard<std::mutex> l(g::mainloop_mtx);
    //normal tank
    for (auto &r: g::normal_tank_events)
    {
      auto tank = id_at(r.first);
      switch (r.second)
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
    g::normal_tank_events.clear();
    
    //auto tank
    for (auto it = g::tanks.begin(); it != g::tanks.end(); ++it)
    {
      utils::tank_assert(it->second != nullptr);
      if (it->second->is_alive() && it->second->is_auto())
      {
        dynamic_cast<tank::AutoTank *>(it->second)->react();
      }
    }
    // bullet move
    for (auto it = g::bullets.begin(); it != g::bullets.end(); ++it)
    {
      if ((*it)->is_alive())
      {
        (*it)->react();
      }
    }
    
    for (auto it = g::bullets.begin(); it != g::bullets.end(); ++it)
    {
      if (!(*it)->is_alive()) continue;
      
      if ((g::game_map.count(map::Status::BULLET, (*it)->get_pos()) > 1)
          || g::game_map.has(map::Status::TANK, (*it)->get_pos()))
      {
        int lethality = 0;
        int attacker = -1;
        auto bullets_instance = g::game_map.at((*it)->get_pos()).get_bullets();
        utils::tank_assert(!bullets_instance.empty());
        for (auto it1 = bullets_instance.begin(); it1 != bullets_instance.end(); ++it1)
        {
          if ((*it1)->is_alive())
          {
            lethality += (*it1)->get_lethality();
          }
          (*it1)->kill();
          attacker = (*it1)->get_tank();
        }
        
        if (g::game_map.has(map::Status::TANK, (*it)->get_pos()))
        {
          if (auto tank = g::game_map.at((*it)->get_pos()).get_tank(); tank != nullptr)
          {
            auto tank_attacker = id_at(attacker);
            utils::tank_assert(tank_attacker != nullptr);
            if (tank->is_auto())
            {
              auto t = dynamic_cast<tank::AutoTank *>(tank);
              if (attacker != t->get_id()
                  && map::get_distance(tank_attacker->get_pos(), tank->get_pos()) < 30)
              {
                t->target(attacker, tank_attacker->get_pos());
              }
            }
            tank->attacked(lethality);
            if (!tank->is_alive())
            {
              msg::info(-1, tank->get_name() + " was killed by " + tank_attacker->get_name());
            }
          }
        }
      }
    }
    clear_death();
  }
  
  void quit()
  {
    for (auto it = g::tanks.begin(); it != g::tanks.end();)
    {
      delete it->second;
      it = g::tanks.erase(it);
    }
    if (g::game_mode == game::GameMode::CLIENT)
    {
      g::online_client.disconnect();
    }
    else if (g::game_mode == game::GameMode::SERVER)
    {
      g::online_server.stop();
    }
  }
}
