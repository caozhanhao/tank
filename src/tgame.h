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
  enum class Event
  {
    NOTHING,
    PAUSE,
    CONTINUE,
    QUIT
  };
  
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
  
  class Game
  {
  private:
    std::vector<tank::Tank> tanks;
    std::vector<tank::AutoTank> auto_tanks;
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
      tanks.insert(tanks.cend(), n, tank::Tank(300, 30, map, changes, get_random_pos(), tanks.size()));
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
                                        [](const tank::AutoTank &at)
                                        { return at.is_alive(); });
      if (alive == 12)return *this;
      std::size_t id = 0;
      if (!auto_tanks.empty())
        id = auto_tanks[auto_tanks.size() - 1].get_id() + 1;
      
      for (int i = 0; i < n; i++)
        auto_tanks.emplace_back(
            tank::AutoTank((int) (11 - level) * 10, (int) (11 - level),
                           map, changes, get_random_pos(), level, id + i));
      
      return *this;
    }
    
    Game &add_auto_boss()
    {
      std::size_t alive = std::count_if(auto_tanks.begin(), auto_tanks.end(),
                                        [](const tank::AutoTank &at)
                                        { return at.is_alive(); });
      if (alive == 12) return *this;
      std::size_t id = 0;
      if (!auto_tanks.empty())
        id = auto_tanks[auto_tanks.size() - 1].get_id() + 1;
      auto_tanks.emplace_back(
          tank::AutoTank(map::random(300, 500), map::random(10, 20),
                         map, changes, get_random_pos(), map::random(0, 4), id));
      return *this;
    }
    
    Game &tank_react(std::size_t tankid, tank::TankEvent event)
    {
      if (!running || !check_tank_alive(tankid)) return *this;
      switch (event)
      {
        case tank::TankEvent::UP:
          tanks[tankid].up(map, changes);
          break;
        case tank::TankEvent::DOWN:
          tanks[tankid].down(map, changes);
          break;
        case tank::TankEvent::LEFT:
          tanks[tankid].left(map, changes);
          break;
        case tank::TankEvent::RIGHT:
          tanks[tankid].right(map, changes);
          break;
        case tank::TankEvent::FIRE:
          fire(tanks[tankid], 0, 2);
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
      for (auto it = auto_tanks.begin(); it < auto_tanks.end() && !all_over(); ++it)
      {
        if (!it->is_alive()) continue;
        //have not been found or target is not alive should target/retarget
        if (!it->get_found() || !get_target(*it).is_alive())
        {
          map::Pos target_pos;
          std::size_t target_id = 0;
          tank::TankType target_type = tank::TankType::TANK;
          auto alive = get_alive(it->get_id());
          
          do
          {
            auto t = alive[map::random(0, (int) alive.size())];
            it->target(map, t.first, t.second, get_pos(t));
          } while (!it->get_found());
        }
        
        //correct its way
        bool should_correct = false;
        if (it->has_arrived())
        {
          if (get_target(*it).get_delay() == 0)
          {
            get_target(*it).mark_blood();
            it->mark_blood();
          }
          ++get_target(*it).get_delay();
          
          int x = (int) it->get_pos().get_x() - (int) get_target(*it).get_pos().get_x();
          int y = (int) it->get_pos().get_y() - (int) get_target(*it).get_pos().get_y();
          
          if (!tank::is_in_firing_line(map, it->get_pos(), get_target(*it).get_pos())// not in firing line
              || it->has_been_attacked_since_marked()// be attacked
              || (x > 0 && it->get_direction() != map::Direction::LEFT)
              || (x < 0 && it->get_direction() != map::Direction::RIGHT)
              || (y > 0 && it->get_direction() != map::Direction::DOWN)
              || (y < 0 && it->get_direction() != map::Direction::UP))
            should_correct = true;
          else if (get_target(*it).get_delay() >= map::get_distance(it->get_pos(), get_target(*it).get_pos()) + 20)
          {
            if (!get_target(*it).has_been_attacked_since_marked())//not shot
              should_correct = true;
            get_target(*it).get_delay() = 0;
          }
        }
        else if (!tank::is_in_firing_line(map, it->get_target_pos(),
                                          get_target(*it).get_pos()))//target is not in firing line
          should_correct = true;
        
        if (should_correct)
        {
          it->target(map, get_target(*it).get_type(), get_target(*it).get_id(),
                     get_target(*it).get_pos());
          get_target(*it).get_delay() = 0;
        }
        switch (it->next())
        {
          case tank::AutoTankEvent::UP:
            it->up(map, changes);
            break;
          case tank::AutoTankEvent::DOWN:
            it->down(map, changes);
            break;
          case tank::AutoTankEvent::LEFT:
            it->left(map, changes);
            break;
          case tank::AutoTankEvent::RIGHT:
            it->right(map, changes);
            break;
          case tank::AutoTankEvent::FIRE:
            fire(*it);
            break;
          case tank::AutoTankEvent::NOTHING:
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
          CZH_NOTICE("Tank " + std::to_string(it - tanks.begin())
                     + " was attacked.Blood: " + std::to_string(it->get_blood()));
        }
        else
        {
          CZH_NOTICE("Tank " + std::to_string(it - tanks.begin()) + " was killed.");
        }
      }
      for (auto it = auto_tanks.begin(); it < auto_tanks.end(); ++it)
      {
        int lethality = it->get_pos().get_point(map.get_map()).get_lethality();
        if (lethality == 0) continue;
        it->attacked(lethality);
        if (it->is_alive())
        {
          CZH_NOTICE("Auto Tank " + std::to_string(it - auto_tanks.begin())
                     + " was attacked.Blood: " + std::to_string(it->get_blood()));
        }
        else
        {
          CZH_NOTICE("Auto Tank " + std::to_string(it - auto_tanks.begin()) + " was killed.");
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
        if (!it->is_alive() && !it->has_cleared())
        {
          it->get_pos().get_point(map.get_map()).remove_status(map::Status::TANK);
          changes.emplace_back(it->get_pos());
          it->clear();
        }
        if (it->get_revived())
        {
          it->get_pos() = get_random_pos();
          it->get_revived() = false;
        }
      }
      for (auto it = auto_tanks.begin(); it < auto_tanks.end(); ++it)
      {
        if (!it->is_alive() && !it->has_cleared())
        {
          it->get_pos().get_point(map.get_map()).remove_status(map::Status::TANK);
          changes.emplace_back(it->get_pos());
          it->clear();
        }
      }
      paint();
      return *this;
    }
    
    [[nodiscard]]bool is_running() const
    { return running; }
  
  private:
    [[nodiscard]]std::vector<std::pair<tank::TankType, std::size_t>> get_alive(std::size_t except) const
    {
      std::vector<std::pair<tank::TankType, std::size_t>> ret;
      for (std::size_t i = 0; i < tanks.size(); ++i)
      {
        if (tanks[i].is_alive())
          ret.emplace_back(std::make_pair(tank::TankType::TANK, i));
      }
      for (std::size_t i = 0; i < auto_tanks.size(); ++i)
      {
        if (auto_tanks[i].is_alive() && i != except)
          ret.emplace_back(std::make_pair(tank::TankType::AUTO, i));
      }
      return ret;
    }
    
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
    
    map::Pos get_pos(const std::pair<tank::TankType, std::size_t> &p)
    {
      if (p.first == tank::TankType::AUTO)
        return auto_tanks[p.second].get_pos();
      return tanks[p.second].get_pos();
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
        }
        else
        {
          w = (int) (it - tanks.begin());
          term::output(colorify_tank(w));
        }
      }
      else if (point.has(map::Status::BULLET))
      {
        auto w = find_bullet(pos.get_x(), pos.get_y());
        if (w->is_from_auto_tank())
          term::output(colorify_auto_tank_text(w->get_id(), w->get_text()));
        else
          term::output(colorify_tank_text(w->get_id(), w->get_text()));
      }
      else if (point.has(map::Status::WALL))
      {
        term::output(colorify_wall());
      }
      else
      {
        term::output(colorify_space());
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
            if (!tanks[i].is_alive())continue;
            std::string sout = colorify_tank_text(i, "Tank " + std::to_string(tanks[i].get_id()));
            term::mvoutput({cursor_x, cursor_y++}, sout);
            std::string blood = std::to_string(tanks[i].get_blood());
            std::string x = std::to_string(tanks[i].get_pos().get_x());
            std::string y = std::to_string(tanks[i].get_pos().get_y());
            sout.clear();
            sout.append("HP: ").append(blood)
                .append(" Pos: (").append(x).append(",").append(y).append(")");
            term::mvoutput({cursor_x, cursor_y++}, sout);
          }
          term::mvoutput({cursor_x, cursor_y++}, " ");
          for (int i = 0; i < auto_tanks.size(); ++i)
          {
            if (!auto_tanks[i].is_alive()) continue;
            std::string sout = colorify_auto_tank_text(i, "Auto Tank " + std::to_string(auto_tanks[i].get_id()));
            term::mvoutput({cursor_x, cursor_y++}, sout);
            
            std::string blood = std::to_string(auto_tanks[i].get_blood());
            std::string x = std::to_string(auto_tanks[i].get_pos().get_x());
            std::string y = std::to_string(auto_tanks[i].get_pos().get_y());
            std::string level = std::to_string(auto_tanks[i].get_level());
            level.insert(level.begin(), 2 - level.size(), '0');
            sout.clear();
            sout.append("HP: ").append(blood)
                .append(" Pos: (").append(x).append(",").append(y).append(")")
                .append(" Level: ").append(level);
            
            auto id = auto_tanks[i].get_target_id();
            if (auto_tanks[i].target_is_auto())
              sout += " Target: " + colorify_auto_tank_text(id, "Auto Tank " + std::to_string(id));
            else
              sout += " Target: " + colorify_tank_text(id, "Tank " + std::to_string(id));
            term::mvoutput({cursor_x, cursor_y++}, sout);
          }
          output_inited = true;
        }
      }
    }
    
    void fire(const tank::Tank &tank, int circle = 0, int blood = 1, int range = 1000)
    {
      auto &point = tank.get_pos().get_point(map.get_map());
      map::Pos pos = tank.get_pos();
      switch (tank.get_direction())
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
          bullet::Bullet(tank.is_auto(), tank.get_id(), pos, tank.get_direction(), tank.get_lethality(), circle, blood,
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