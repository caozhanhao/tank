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
namespace czh::game
{
  static void out(std::size_t x, std::size_t y, const std::string &str)
  {
    term::move_cursor({x, y});
    auto a = term::get_width() - str.size() - x;
    if (a <= 0)
      a = 1;
    term::output(str + std::string(a, ' '));
  }
  
  static void clear_tank_status(std::size_t x, std::size_t y)
  {
    term::move_cursor({x, y});
    std::size_t a = term::get_width() - x - 1;
    std::size_t b = term::get_height() - y;
    for (int i = 0; i < b; ++i)
    {
      term::output(" " + std::string(a, ' '));
      term::move_cursor({x, y++});
    }
  }
  
  std::string colorify_tank(std::size_t w)
  {
    w = w % 2;
    std::string ret = "\033[";
    ret += std::to_string(w + 46);
    ret += ";36m";
    ret += " \033[0m\033[?25l";
    return ret;
  }
  
  std::string colorify_tank_text(std::size_t w, const std::string &str)
  {
    w = w % 2;
    std::string ret = "\033[";
    ret += std::to_string(w + 36);
    ret += "m";
    ret += str;
    ret += "\033[0m\033[?25l";
    return ret;
  }
  
  std::string colorify_auto_tank(std::size_t w)
  {
    w = w % 4;
    std::string ret = "\033[";
    ret += std::to_string(w + 42);
    ret += ";31m";
    ret += " \033[0m\033[?25l";
    return ret;
  }
  
  std::string colorify_auto_tank_text(std::size_t w, const std::string &str)
  {
    w = w % 4;
    std::string ret = "\033[";
    ret += std::to_string(w + 32);
    ret += "m";
    ret += str;
    ret += "\033[0m\033[?25l";
    return ret;
  }
  
  std::string colorify_wall()
  {
    std::string ret = "\033[0;41;37m \033[0m\033[?25l";
    return ret;
  }
  
  std::string colorify_space()
  {
    return " ";
  }
  
  enum class Event
  {
    TANK_UP, TANK_DOWN, TANK_LEFT, TANK_RIGHT,
    TANK_FIRE,
    NOTHING,
    QUIT
  };
  
  class Game
  {
  private:
    map::Map map;
    std::vector<tank::Tank> tanks;
    std::vector<tank::AutoTank> auto_tanks;
    std::vector<bullet::Bullet> bullets;
    std::vector<map::Change> changes;
    bool inited;
    std::size_t screen_height;
    std::size_t screen_width;
    bool tank_status_changed;
    bool running;
  public:
    Game() : inited(false), tank_status_changed(true), running(true),
             screen_height(term::get_height()), screen_width(term::get_width()) {}
    
    Game &add_tank(std::size_t n = 1)
    {
      tanks.insert(tanks.cend(), n, tank::Tank(100, 30, map, changes, get_random_pos(), tanks.size()));
      tank_status_changed = true;
      return *this;
    }
    
    Game &revive(std::size_t id)
    {
      tanks[id].revive();
      return *this;
    }
    
    Game &add_auto_tank(std::size_t n = 1, std::size_t level = 1)
    {
      std::size_t alive = std::count_if(auto_tanks.begin(), auto_tanks.end(),
                                        [](const tank::AutoTank &at) { return at.is_alive(); });
      if (alive == 12)return *this;
      std::size_t id = 0;
      if (!auto_tanks.empty())
        id = auto_tanks[auto_tanks.size() - 1].get_id() + 1;
      
      for (int i = 0; i < n; i++)
        auto_tanks.emplace_back(
            tank::AutoTank((int) (11 - level) * 10, (int) (11 - level), map, changes, get_random_pos(), level, id + i));
      
      tank_status_changed = true;
      return *this;
    }
    
    Game &add_auto_boss()
    {
      std::size_t alive = std::count_if(auto_tanks.begin(), auto_tanks.end(),
                                        [](const tank::AutoTank &at) { return at.is_alive(); });
      if (alive == 12) return *this;
      std::size_t id = 0;
      if (!auto_tanks.empty())
        id = auto_tanks[auto_tanks.size() - 1].get_id() + 1;
      auto_tanks.emplace_back(
          tank::AutoTank(map::random(300, 500), map::random(10, 20), map, changes, get_random_pos(), map::random(1, 4),
                         id));
      tank_status_changed = true;
      return *this;
    }
    
    Game &react(std::size_t tankid, Event event)
    {
      //bullet move
      for (auto it = bullets.begin(); it < bullets.end(); ++it)
      {
        if (it->is_alive())
          it->move(map, changes);
      }
      //tank
      auto &tank = tanks[tankid];
      switch (event)
      {
        case Event::TANK_UP:
          if (!check_tank_alive(tankid)) break;
          tank.up(map, changes);
          tank_status_changed = true;
          break;
        case Event::TANK_DOWN:
          if (!check_tank_alive(tankid)) break;
          tank.down(map, changes);
          tank_status_changed = true;
          break;
        case Event::TANK_LEFT:
          if (!check_tank_alive(tankid)) break;
          tank.left(map, changes);
          tank_status_changed = true;
          break;
        case Event::TANK_RIGHT:
          if (!check_tank_alive(tankid)) break;
          tank.right(map, changes);
          tank_status_changed = true;
          break;
        case Event::TANK_FIRE:
          if (!check_tank_alive(tankid)) break;
          fire(tank, 0, 2);
          tank_status_changed = true;
          break;
        case Event::NOTHING:
          break;
        case Event::QUIT:
          term::move_cursor({0, map.get_height() + 1});
          CZH_NOTICE("Quitting.");
          running = false;
          break;
        default:
          break;
      }
      //auto tank
      for (auto it = auto_tanks.begin(); it < auto_tanks.end() && !all_over(); ++it)
      {
        if (!it->is_alive()) continue;
        //have not been found or target is not alive should target/retarget
        if (!it->get_found() || !get_target(*it).is_alive())
        {
          map::Pos target_pos;
          std::size_t target_id = 0;
          tank::TankType target_type = tank::TankType::TANK;
          do
          {
            target_id = map::random((int) 0, (int) tanks.size() + (int) auto_tanks.size());
            //target_id = map::random(0, tanks.size());
            if (target_id < tanks.size())
            {
              target_pos = tanks[target_id].get_pos();
              target_type = tank::TankType::TANK;
            } else if (target_id - tanks.size() < auto_tanks.size())
            {
              target_id -= tanks.size();
              target_pos = auto_tanks[target_id].get_pos();
              target_type = tank::TankType::AUTO;
            }
          } while (target_type == tank::TankType::AUTO && target_id == it->get_id());
          it->target(map, target_type, target_id, target_pos);
          tank_status_changed = true;
        }
        
        //correct its way
        bool should_correct = false;
        if (it->get_correct())// has arrived
        {
          if (get_target(*it).get_delay() == 0)
          {
            get_target(*it).mark_blood();
            it->mark_blood();
          }
          ++get_target(*it).get_delay();
          
          int x = (int) it->get_pos().get_x() - (int) get_target(*it).get_pos().get_x();
          int y = (int) it->get_pos().get_y() - (int) get_target(*it).get_pos().get_y();
          
          if (!(it->get_pos().get_x() == get_target(*it).get_pos().get_x()
                || it->get_pos().get_y() == get_target(*it).get_pos().get_y())// not in firing line
              || it->has_been_attacked_since_marked())// be attacked
            should_correct = true;
          else if (get_target(*it).get_delay() >= map::get_distance(it->get_pos(), get_target(*it).get_pos()) + 20)
          {
            if (!get_target(*it).has_been_attacked_since_marked())//not shot
              should_correct = true;
            get_target(*it).get_delay() = 0;
          } else if ((x > 0 && it->get_Direction() != map::Direction::LEFT)
                     || (x < 0 && it->get_Direction() != map::Direction::RIGHT)
                     || (y > 0 && it->get_Direction() != map::Direction::DOWN)
                     || (y < 0 && it->get_Direction() != map::Direction::UP))
          {
            should_correct = true;
          }
        } else if (!(it->get_around_target_pos().get_x() == get_target(*it).get_pos().get_x()
                     || it->get_around_target_pos().get_y() ==
                        get_target(*it).get_pos().get_y()))//around target is not in firing line
          should_correct = true;
        
        if (should_correct)
        {
          it->target(map, get_target(*it).get_type(), get_target(*it).get_id(), get_target(*it).get_pos());
          get_target(*it).get_delay() = 0;
          tank_status_changed = true;
        }
        switch (it->next())
        {
          case map::AutoTankEvent::UP:
            it->up(map, changes);
            tank_status_changed = true;
            break;
          case map::AutoTankEvent::DOWN:
            it->down(map, changes);
            tank_status_changed = true;
            break;
          case map::AutoTankEvent::LEFT:
            it->left(map, changes);
            tank_status_changed = true;
            break;
          case map::AutoTankEvent::RIGHT:
            it->right(map, changes);
            tank_status_changed = true;
            break;
          case map::AutoTankEvent::FIRE:
            fire(*it);
            tank_status_changed = true;
            break;
          case map::AutoTankEvent::NOTHING:
            break;
        }
        
      }
      //conflict
      for (auto it = bullets.begin(); it < bullets.end(); ++it)
      {
        if (it->get_pos().get_point(map.get_map()).count(map::Status::BULLET) > 1
            || (it->get_pos().get_point(map.get_map()).has(map::Status::TANK)))
        {
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
      for (auto it = tanks.begin(); it < tanks.end(); ++it)
      {
        int lethality = it->get_pos().get_point(map.get_map()).get_lethality();
        if (lethality == 0) continue;
        it->attacked(lethality);
        if (it->is_alive())
        {
          CZH_NOTICE("Tank " + std::to_string(it - tanks.begin()) + " was attacked.");
        } else
        {
          CZH_NOTICE("Tank " + std::to_string(it - tanks.begin()) + " was killed.");
        }
        tank_status_changed = true;
      }
      for (auto it = auto_tanks.begin(); it < auto_tanks.end(); ++it)
      {
        int lethality = it->get_pos().get_point(map.get_map()).get_lethality();
        if (lethality == 0) continue;
        it->attacked(lethality);
        if (it->is_alive())
        {
          CZH_NOTICE("Auto Tank " + std::to_string(it - auto_tanks.begin()) + " was attacked.");
        } else
        {
          CZH_NOTICE("Auto Tank " + std::to_string(it - auto_tanks.begin()) + " was killed.");
        }
        tank_status_changed = true;
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
      bool all_over = true;
      for (auto it = tanks.begin(); it < tanks.end(); ++it)
      {
        if (!it->is_alive())
        {
          if (!it->has_cleared())
          {
            it->get_pos().get_point(map.get_map()).remove_status(map::Status::TANK);
            changes.emplace_back(it->get_pos());
            it->clear();
          }
        } else
          all_over = false;
      }
      for (auto it = auto_tanks.begin(); it < auto_tanks.end(); ++it)
      {
        if (!it->is_alive())
        {
          if (!it->has_cleared())
          {
            it->get_pos().get_point(map.get_map()).remove_status(map::Status::TANK);
            changes.emplace_back(it->get_pos());
            it->clear();
          }
        }
      }
      paint();
      
      //if (all_over)
      //{
      //  logger::move_cursor(0, map.get_height() + 1);
      //  CZH_NOTICE("Game Over.");
      //}
      return *this;
    }
    
    [[nodiscard]]bool is_running() const { return running; }
  
  private:
    [[nodiscard]]bool all_over() const
    {
      std::size_t alive = 0;
      for (auto &t: tanks)
      {
        if (t.is_alive())
          ++alive;
      }
      for (auto &t: auto_tanks)
      {
        if (t.is_alive())
          ++alive;
      }
      return alive <= 1;
    }
    
    map::Pos get_random_pos()
    {
      map::Pos pos;
      do
      {
        pos = map::Pos(map::random(1, (int) map.get_width() - 1), map::random(1, (int) map.get_height() - 1));
      } while (find_tank(pos.get_x(), pos.get_y()) != tanks.end()
               || find_auto_tank(pos.get_x(), pos.get_y()) != auto_tanks.end()
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
        int w = 0;
        if (it == tanks.end())
        {
          auto ita = find_auto_tank(pos.get_x(), pos.get_y());
          w = (int) (ita - auto_tanks.begin());
          term::output(colorify_auto_tank(w));
        } else
        {
          w = (int) (it - tanks.begin());
          term::output(colorify_tank(w));
        }
      } else if (point.has(map::Status::BULLET))
      {
        auto w = find_bullet(pos.get_x(), pos.get_y());
        if (w->is_from_auto_tank())
          term::output(colorify_auto_tank_text(w->get_id(), w->get_text()));
        else
          term::output(colorify_tank_text(w->get_id(), w->get_text()));
      } else if (point.has(map::Status::WALL))
      {
        term::output(colorify_wall());
      } else
      {
        term::output(colorify_space());
      }
    }
    
    void paint()
    {
      if (screen_height != term::get_height() || screen_width != term::get_width())
      {
        term::clear();
        inited = false;
        screen_height = term::get_height();
        screen_width = term::get_width();
      }
      if (!inited)
      {
        term::clear();
        term::move_cursor({map.get_width() + 1, 0});
        term::output("Tank - by caozhanhao");
        term::move_cursor({0, 0});
        for (int j = map.get_height() - 1; j >= 0; --j)
        {
          for (int i = 0; i < map.get_width(); ++i)
          {
            update(map::Pos(i, j));
          }
          term::output("\n");
        }
        inited = true;
      } else
      {
        for (auto &p: changes)
          update(p.get_pos());
        changes.clear();
      }
      //tank status
      if (tank_status_changed)
      {
        std::size_t cursor_x = map.get_width();
        std::size_t cursor_y = 0;
        for (int i = 0; i < tanks.size(); ++i)
        {
          if (!tanks[i].is_alive())continue;
          std::string sout = colorify_tank_text(i, "Tank " + std::to_string(tanks[i].get_id()));
          out(cursor_x, ++cursor_y, sout);
          std::string blood = std::to_string(tanks[i].get_blood());
          blood.insert(blood.end(), 3 - blood.size(), ' ');
          std::string x = std::to_string(tanks[i].get_pos().get_x());
          x.insert(x.begin(), 2 - x.size(), '0');
          std::string y = std::to_string(tanks[i].get_pos().get_y());
          y.insert(y.begin(), 2 - y.size(), '0');
          sout.clear();
          sout.append("HP: ").append(blood);
          out(cursor_x, ++cursor_y, sout);
        }
        out(cursor_x, ++cursor_y, " ");
        for (int i = 0; i < auto_tanks.size(); ++i)
        {
          if (!auto_tanks[i].is_alive()) continue;
          std::string sout = colorify_auto_tank_text(i, "Auto Tank " + std::to_string(auto_tanks[i].get_id()));
          out(cursor_x, ++cursor_y, sout);
          
          std::string blood = std::to_string(auto_tanks[i].get_blood());
          blood.insert(blood.end(), 3 - blood.size(), ' ');
          std::string x = std::to_string(auto_tanks[i].get_pos().get_x());
          x.insert(x.begin(), 2 - x.size(), '0');
          std::string y = std::to_string(auto_tanks[i].get_pos().get_y());
          y.insert(y.begin(), 2 - y.size(), '0');
          std::string level = std::to_string(auto_tanks[i].get_level());
          level.insert(level.begin(), 2 - level.size(), '0');
          sout.clear();
          sout.append("HP: ").append(blood)
              .append(" Level: ").append(level);
          
          auto id = auto_tanks[i].get_target_id();
          if (auto_tanks[i].target_is_auto())
            sout += " Target: " + colorify_auto_tank_text(id, "Auto Tank " + std::to_string(id));
          else
            sout += " Target: " + colorify_tank_text(id, "Tank " + std::to_string(id));
          out(cursor_x, ++cursor_y, sout);
        }
        clear_tank_status(cursor_x, ++cursor_y);
        tank_status_changed = false;
      }
    }
    
    void fire(const tank::Tank &tank, int circle = 0, int blood = 1, int range = 1000)
    {
      auto &point = tank.get_pos().get_point(map.get_map());
      map::Pos pos = tank.get_pos();
      switch (tank.get_Direction())
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
          bullet::Bullet(tank.is_auto(), tank.get_id(), pos, tank.get_Direction(), tank.get_lethality(), circle, blood,
                         range));
    }
    
    bool check_tank_alive(const std::size_t tankid)
    {
      if (!tanks[tankid].is_alive())
      {
        CZH_NOTICE("Tank " + std::to_string(tankid) + " is not alive.");
        return false;
      }
      return true;
    }
    
    tank::Tank &get_target(tank::AutoTank &tank)
    {
      if (tank.target_is_auto())
        return auto_tanks[tank.get_target_id()];
      return tanks[tank.get_target_id()];
    }
    
    std::vector<bullet::Bullet>::iterator find_bullet(std::size_t i, std::size_t j)
    {
      return std::find_if(bullets.begin(), bullets.end(),
                          [i, j](const bullet::Bullet &b)
                          {
                            return (b.get_pos().get_x() == i && b.get_pos().get_y() == j);
                          });
    }
    
    std::vector<tank::Tank>::iterator find_tank(std::size_t i, std::size_t j)
    {
      return std::find_if(tanks.begin(), tanks.end(),
                          [i, j](const tank::Tank &b)
                          {
                            return (b.is_alive() && b.get_pos().get_x() == i && b.get_pos().get_y() == j);
                          });
    }
    
    std::vector<tank::AutoTank>::iterator find_auto_tank(std::size_t i, std::size_t j)
    {
      return std::find_if(auto_tanks.begin(), auto_tanks.end(),
                          [i, j](const tank::AutoTank &b)
                          {
                            return (b.is_alive() && b.get_pos().get_x() == i && b.get_pos().get_y() == j);
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