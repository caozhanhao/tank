#pragma once
#include "ttank.h"
#include "tterm.h"
#include "tmap.h"
#include "tbullet.h"
#include "tlogger.h"
#include <vector>
#include <algorithm>
#include <functional>
#include <string>
#include <memory>
namespace czh::game
{
  enum class Event
  {
    NOTHING,
    PAUSE,
    CONTINUE,
    QUIT
  };
  
  class Game
  {
  private:
    std::vector<std::shared_ptr<tank::Tank>> tanks;
    std::vector<bullet::Bullet> bullets;
    std::vector<map::Change> changes;
    bool output_inited;
    std::size_t screen_height;
    std::size_t screen_width;
    map::Map map;
    bool running;
  public:
    Game() : output_inited(false), running(true),
             screen_height(term::get_height()), screen_width(term::get_width()),
             map((screen_height - 1) % 2 == 0 ? screen_height - 2 : screen_height - 1,
                 screen_width % 2 == 0 ? screen_width - 1 : screen_width)
    {}
    
    Game &add_tank(std::size_t n = 1)
    {
      std::size_t id = 0;
      auto it = std::find_if(tanks.rbegin(), tanks.rend(), [](const std::shared_ptr<tank::Tank> &ptr)
      { return !ptr->is_auto(); });
      if (it != tanks.rend())
        id = (*it)->get_id() + 1;
      for (int i = 0; i < n; i++)
      tanks.insert(tanks.cend(), n, std::make_shared<tank::NormalTank>
      (300, 30, map, changes, get_random_pos(),  id + i));
      return *this;
    }
    
    Game &revive(std::size_t id)
    {
      std::dynamic_pointer_cast<tank::NormalTank>(tanks[id])->
          revive(map, changes, get_random_pos());
      return *this;
    }
    
    Game &add_auto_tank(std::size_t n = 1, std::size_t level = 1)
    {
      std::size_t alive = std::count_if(tanks.begin(), tanks.end(),
                                        [](const std::shared_ptr<tank::Tank> &ptr)
                                        { return ptr->is_auto() && ptr->is_alive(); });
      if (alive == 12)return *this;
      std::size_t id = 0;
      auto it = std::find_if(tanks.rbegin(), tanks.rend(), [](const std::shared_ptr<tank::Tank> &ptr)
      { return ptr->is_auto(); });
      if (it != tanks.rend())
        id = (*it)->get_id() + 1;
      for (int i = 0; i < n; i++)
        tanks.emplace_back(
            std::make_shared<tank::AutoTank>((int) (11 - level) * 10, (int) (11 - level),
                                             map, changes, get_random_pos(), level, id + i));
      return *this;
    }
    
    Game &add_auto_boss()
    {
      std::size_t alive = std::count_if(tanks.begin(), tanks.end(),
                                        [](const std::shared_ptr<tank::Tank> &ptr)
                                        { return ptr->is_auto() && ptr->is_alive(); });
      if (alive == 12)return *this;
      std::size_t id = 0;
      auto it = std::find_if(tanks.rbegin(), tanks.rend(), [](const std::shared_ptr<tank::Tank> &ptr)
      { return ptr->is_auto(); });
      if (it != tanks.rend())
        id = (*it)->get_id() + 1;
      tanks.emplace_back(
          std::make_shared<tank::AutoTank>(map::random(300, 500), map::random(10, 20),
                                           map, changes, get_random_pos(), map::random(0, 4), id));
      return *this;
    }
    
    Game &tank_react(std::size_t tankid, tank::NormalTankEvent event)
    {
      if (!running || !tanks[tankid]->is_alive()) return *this;
      auto tank = std::dynamic_pointer_cast<tank::NormalTank>(tanks[tankid]);
      switch (event)
      {
        case tank::NormalTankEvent::UP:
          tank->up(map, changes);
          break;
        case tank::NormalTankEvent::DOWN:
          tank->down(map, changes);
          break;
        case tank::NormalTankEvent::LEFT:
          tank->left(map, changes);
          break;
        case tank::NormalTankEvent::RIGHT:
          tank->right(map, changes);
          break;
        case tank::NormalTankEvent::FIRE:
          fire(tank, 0, 2);
          break;
      }
      react(Event::NOTHING);
      return *this;
    }
    
    Game &react(Event event)
    {
      switch (event)
      {
        case Event::NOTHING:
          break;
        case Event::PAUSE:
          running = false;
          output_inited = false;
          break;
        case Event::CONTINUE:
          running = true;
          output_inited = false;
          break;
        case Event::QUIT:
          term::move_cursor({0, map.get_height() + 1});
          term::output("\033[?25h");
          CZH_NOTICE("Quitting.");
          running = false;
          break;
      }
      if (!running)
      {
        paint();
        return *this;
      }
      //bullet move
      for (auto it = bullets.begin(); it < bullets.end(); ++it)
      {
        if (it->is_alive())
          it->move(map, changes);
      }
      //auto tank
      for (auto it = tanks.begin(); it < tanks.end() && !all_over(); ++it)
      {
        if (!(*it)->is_alive() || !(*it)->is_auto()) continue;
        auto tank = std::dynamic_pointer_cast<tank::AutoTank>(*it);
        auto target = tanks[tank->get_target_pos_in_vec()];
        //have not been found or target is not alive should target/retarget
        if (!tank->get_found() || !target->is_alive())
        {
          map::Pos target_pos;
          std::size_t target_id = 0;
          tank::TankType target_type = tank::TankType::NORMAL;
          auto alive = get_alive(it - tanks.begin());
          do
          {
            auto t = alive[map::random(0, (int) alive.size())];
            auto p = tanks[t];
            tank->target(map, t, p->get_pos());
          } while (!tank->get_found());
          target = tanks[tank->get_target_pos_in_vec()];
        }
        
        //correct its way
        bool should_correct = false;
        if (tank->has_arrived())
        {
          if (target->get_delay() == 0)
          {
            target->mark_blood();
            tank->mark_blood();
          }
          ++target->get_delay();
          
          int x = (int) tank->get_pos().get_x() - (int) target->get_pos().get_x();
          int y = (int) tank->get_pos().get_y() - (int) target->get_pos().get_y();
          
          if (!tank::is_in_firing_line(map, tank->get_pos(), target->get_pos())// not in firing line
              || tank->has_been_attacked_since_marked()// be attacked
              || (x > 0 && tank->get_direction() != map::Direction::LEFT)
              || (x < 0 && tank->get_direction() != map::Direction::RIGHT)
              || (y > 0 && tank->get_direction() != map::Direction::DOWN)
              || (y < 0 && tank->get_direction() != map::Direction::UP))
            should_correct = true;
          else if (target->get_delay() >= map::get_distance(tank->get_pos(), target->get_pos()) + 20)
          {
            if (!target->has_been_attacked_since_marked())//not shot
              should_correct = true;
            target->get_delay() = 0;
          }
        }
        else if (!tank::is_in_firing_line(map, tank->get_target_pos(),
                                          target->get_pos()))//target is not in firing line
          should_correct = true;
        
        if (should_correct)
        {
          tank->target(map, tank->get_target_pos_in_vec(),  target->get_pos());
          target->get_delay() = 0;
        }
        switch (tank->next())
        {
          case tank::AutoTankEvent::UP:
            tank->up(map, changes);
            break;
          case tank::AutoTankEvent::DOWN:
            tank->down(map, changes);
            break;
          case tank::AutoTankEvent::LEFT:
            tank->left(map, changes);
            break;
          case tank::AutoTankEvent::RIGHT:
            tank->right(map, changes);
            break;
          case tank::AutoTankEvent::FIRE:
            fire(tank);
            break;
          case tank::AutoTankEvent::NOTHING:
            break;
        }
      }
      //conflict
      bool conflict = false;
      for (auto it = bullets.begin(); it < bullets.end(); ++it)
      {
        if (it->get_pos().get_point(map.get_map()).count(map::Status::BULLET) > 1
            || (it->get_pos().get_point(map.get_map()).has(map::Status::TANK)))
        {
          conflict = true;
          for_all_bullets(it->get_pos().get_x(), it->get_pos().get_y(),
                          [this](std::vector<bullet::Bullet>::iterator &it)
                          {
                            for (std::size_t i = it->get_pos().get_x() - it->get_circle();
                                 i <= it->get_pos().get_x() + it->get_circle(); ++i)
                            {
                              for (std::size_t j = it->get_pos().get_y() - it->get_circle();
                                   j <= it->get_pos().get_y() + it->get_circle(); ++j)
                              {
                                map.get_map()[i][j].attacked(it->get_lethality());
                              }
                            }
                            it->kill();
                          });
        }
      }
      if(conflict)
      {
        for (auto it = tanks.begin(); it < tanks.end(); ++it)
        {
          auto tank = *it;
          int lethality = tank->get_pos().get_point(map.get_map()).get_lethality();
          if (lethality == 0) continue;
          tank->attacked(lethality);
          if (tank->is_alive())
          {
            CZH_NOTICE(tank->get_name() + " was attacked.Blood: " + std::to_string(tank->get_blood()));
          }
          else
          {
            CZH_NOTICE(tank->get_name() + " was killed.");
          }
        }
        for (auto it = bullets.begin(); it < bullets.end(); ++it)
        {
          int lethality = it->get_pos().get_point(map.get_map()).get_lethality();
          it->attacked(lethality);
        }
        for (int i = 0; i < map.get_width(); ++i)
        {
          for (int j = 0; j < map.get_height(); ++j)
            map.get_map()[i][j].remove_lethality();
        }
        //clear death
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                                     [this](bullet::Bullet &bullet)
                                     {
                                       if (!bullet.is_alive())
                                       {
                                         bullet.get_pos().get_point(map.get_map()).remove_status(map::Status::BULLET);
                                         changes.emplace_back(bullet.get_pos());
                                         return true;
                                       }
                                       return false;
                                     }), bullets.end());
        for (auto it = tanks.begin(); it < tanks.end(); ++it)
        {
          auto tank = *it;
          if (!tank->is_alive() && !tank->has_cleared())
          {
            tank->get_pos().get_point(map.get_map()).remove_status(map::Status::TANK);
            changes.emplace_back(tank->get_pos());
            tank->clear();
          }
        }
      }
      paint();
      return *this;
    }
    
    [[nodiscard]]bool is_running() const
    { return running; }
  
  private:
    [[nodiscard]]std::vector<std::size_t> get_alive(std::size_t except) const
    {
      std::vector<std::size_t> ret;
      for (std::size_t i = 0; i < tanks.size(); ++i)
      {
        if (tanks[i]->is_alive() && i != except)
          ret.emplace_back(i);
      }
      return ret;
    }
    
    [[nodiscard]]bool all_over() const
    {
      std::size_t alive = 0;
      for (auto &t: tanks)
      {
        if (t->is_alive())
          ++alive;
      }
      
      return alive <= 1;
    }
    
    auto find_tank(std::size_t i, std::size_t j)
    {
      return std::find_if(tanks.begin(), tanks.end(),
                          [i, j](const std::shared_ptr<tank::Tank> &b)
                          {
                            return (b->is_alive() && b->get_pos().get_x() == i && b->get_pos().get_y() == j);
                          });
    }
    
    map::Pos get_random_pos()
    {
      map::Pos pos;
      do
      {
        pos = map::Pos(map::random(1, (int) map.get_width() - 1), map::random(1, (int) map.get_height() - 1));
      } while (find_tank(pos.get_x(), pos.get_y()) != tanks.end()
               || pos.get_point(map.get_map()).has(map::Status::WALL));
      return pos;
    }
    
    void update(const map::Pos &pos)
    {
      term::move_cursor({pos.get_x(), map.get_height() - pos.get_y() - 1});
      auto &point = pos.get_point(map.get_map());
      if (point.has(map::Status::TANK))
      {
        auto it = find_tank(pos.get_x(), pos.get_y());
        term::output((*it)->colorify_tank());
      }
      else if (point.has(map::Status::BULLET))
      {
        auto w = find_bullet(pos.get_x(), pos.get_y());
        term::output(w->get_from()->colorify_text(w->get_text()));
      }
      else if (point.has(map::Status::WALL))
      {
        term::output("\033[0;41;37m \033[0m\033[?25l");
      }
      else
      {
        term::output(" ");
      }
    }
    
    void paint()
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
          for (int j = map.get_height() - 1; j >= 0; --j)
          {
            for (int i = 0; i < map.get_width(); ++i)
            {
              update(map::Pos(i, j));
            }
            term::output("\n");
          }
          output_inited = true;
        }
        else
        {
          for (auto &p: changes)
            update(p.get_pos());
          changes.clear();
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
          term::mvoutput({cursor_x, cursor_y++}, "Tank - by caozhanhao");
          for (int i = 0; i < tanks.size(); ++i)
          {
            auto tank = tanks[i];
            if (!tank->is_alive())continue;
            std::string sout = tank->colorify_text(tank->get_name());
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
              auto target = tanks[at->get_target_pos_in_vec()];
              std::string level = std::to_string(at->get_level());
              level.insert(level.begin(), 2 - level.size(), '0');
              sout.append(" Level: ").append(level).append(" Target: ")
                  .append(target->colorify_text(target->get_name()));
            }
            term::mvoutput({cursor_x, cursor_y++}, sout);
          }
          term::mvoutput({cursor_x, cursor_y++}, " ");
          output_inited = true;
        }
      }
    }
    
    void fire(std::shared_ptr<tank::Tank> tank, int circle = 0, int blood = 1, int range = 1000)
    {
      auto &point = tank->get_pos().get_point(map.get_map());
      map::Pos pos = tank->get_pos();
      switch (tank->get_direction())
      {
        case map::Direction::UP:
          pos.get_y()++;
          break;
        case map::Direction::DOWN:
          pos.get_y()--;
          break;
        case map::Direction::LEFT:
          pos.get_x()--;
          break;
        case map::Direction::RIGHT:
          pos.get_x()++;
          break;
      }
      auto &bullet_point = pos.get_point(map.get_map());
      if (bullet_point.has(map::Status::WALL)) return;
      bullet_point.add_status(map::Status::BULLET);
      bullets.emplace_back(
          bullet::Bullet(tank, pos, tank->get_direction(),
                         tank->get_lethality(), circle, blood, range));
    }
    
    
    std::vector<bullet::Bullet>::iterator find_bullet(std::size_t i, std::size_t j)
    {
      return std::find_if(bullets.begin(), bullets.end(),
                          [i, j](const bullet::Bullet &b)
                          {
                            return (b.get_pos().get_x() == i && b.get_pos().get_y() == j);
                          });
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
  };
}