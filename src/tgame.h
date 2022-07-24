#pragma once
#include "ttank.h"
#include "tmap.h"
#include "tbullet.h"
#include "tlogger.h"
#include <vector>
#include <algorithm>
#include <functional>
#include <string>
#include <Windows.h>
namespace czh::game
{
  static void out(int x, int y, const std::string& str)
  {
    static HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    static CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(handle, &csbi);
    logger::move_cursor(x, y);
    int a = csbi.srWindow.Right - csbi.srWindow.Left + 1 - str.size() - x;
    if (a <= 0)
      a = 1;
    std::cout << str << std::string(a, ' ');
  }
  static void clear_tank_status(int x, int y)
  {
    static HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    static CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(handle, &csbi);
    logger::move_cursor(x, y);
    std::size_t a = csbi.srWindow.Right - csbi.srWindow.Left - x;
    std::size_t b = csbi.srWindow.Bottom - csbi.srWindow.Top + 1 - y;
    for (int i = 0; i < b; ++i)
    {
      std::cout << " " << std::string(a, ' ');
      logger::move_cursor(x, y++);
    }
  }
  std::string colorify_tank(int w)
  {
    w = w % 2;
    std::string ret = "\033[";
    ret += std::to_string(w + 46);
    ret += ";36m";
    ret += " \033[0m";
    return ret;
  }
  std::string colorify_tank_text(int w, const std::string& str)
  {
    w = w % 2;
    std::string ret = "\033[";
    ret += std::to_string(w + 36);
    ret += "m";
    ret += str;
    ret += "\033[0m";
    return ret;
  }
  std::string colorify_auto_tank(int w)
  {
    w = w % 5;
    std::string ret = "\033[";
    ret += std::to_string(w + 41);
    ret += ";31m";
    ret += " \033[0m";
    return ret;
  }
  std::string colorify_auto_tank_text(int w, const std::string& str)
  {
    w = w % 5;
    std::string ret = "\033[";
    ret += std::to_string(w + 31);
    ret += "m";
    ret += str;
    ret += "\033[0m";
    return ret;
  }
  std::string colorify_wall()
  {
    std::string ret = "\033[0;41;37m \033[0m";
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
    std::vector<map::Pos> changes;
    bool inited;
    bool tank_status_changed;
  public:
    Game():  inited(false), tank_status_changed(true){}
    Game& add_tank(std::size_t n = 1)
    {
      tanks.insert(tanks.cend(), n, tank::Tank(100, 20, map, changes, get_random_pos(), tanks.size()));
      tank_status_changed = true;
      return *this;
    }
    Game& revive(std::size_t id)
    {
      tanks[id].revive();
      return *this;
    }
    Game& add_auto_tank(std::size_t n = 1, std::size_t level = 1)
    {
      int alive = std::count_if(auto_tanks.begin(), auto_tanks.end(),
        [](const tank::AutoTank& at) {return at.is_alive(); });
      if (alive == 12)return *this;
      int id = 0;
      if (auto_tanks.size() != 0)
        id = auto_tanks[auto_tanks.size() - 1].get_id() + 1;

      for (int i = 0; i < n; i++)
        auto_tanks.emplace_back(tank::AutoTank((11 - level) * 10, level, map, changes, get_random_pos(), level, id + i));

      tank_status_changed = true;
      return *this;
    }
    Game& add_auto_boss()
    {
      int alive = std::count_if(auto_tanks.begin(), auto_tanks.end(),
        [](const tank::AutoTank& at) {return at.is_alive(); });
      if (alive == 12) return *this;
      int id = 0;
      if (auto_tanks.size() != 0)
        id = auto_tanks[auto_tanks.size() - 1].get_id() + 1;
      auto_tanks.emplace_back(tank::AutoTank(999, 10, map, changes, get_random_pos(), 10, id));
      tank_status_changed = true;
      return *this;
    }
    Game& react(std::size_t tankid, Event event)
    {
      //bullet move
      for (auto it = bullets.begin(); it < bullets.end(); ++it)
      {
        if (it->is_alive())
          it->move(map, changes);
      }
      //tank
      auto& tank = tanks[tankid];
      switch (event)
      {
      case Event::TANK_UP:
        if (!check_tank_alive(tankid)) break;
        tank.up(map, changes);
        CZH_NOTICE("Tank " + std::to_string(tankid) + " moved up.At:("
          + std::to_string(tank.get_pos().get_x()) + "," + std::to_string(tank.get_pos().get_y()) + ").");
        tank_status_changed = true;
        break;
      case Event::TANK_DOWN:
        if (!check_tank_alive(tankid)) break;
        tank.down(map, changes);
        CZH_NOTICE("Tank " + std::to_string(tankid) + " moved down.At:("
+std::to_string(tank.get_pos().get_x()) + "," + std::to_string(tank.get_pos().get_y()) + ").");
tank_status_changed = true;
break;
      case Event::TANK_LEFT:
        if (!check_tank_alive(tankid)) break;
        tank.left(map, changes);
        CZH_NOTICE("Tank " + std::to_string(tankid) + " moved left.At:("
          + std::to_string(tank.get_pos().get_x()) + "," + std::to_string(tank.get_pos().get_y()) + ").");
        tank_status_changed = true;
        break;
      case Event::TANK_RIGHT:
        if (!check_tank_alive(tankid)) break;
        tank.right(map, changes);
        CZH_NOTICE("Tank " + std::to_string(tankid) + " moved right.At:("
          + std::to_string(tank.get_pos().get_x()) + "," + std::to_string(tank.get_pos().get_y()) + ").");
        tank_status_changed = true;
        break;
      case Event::TANK_FIRE:
        if (!check_tank_alive(tankid)) break;
        CZH_NOTICE("Tank " + std::to_string(tankid) + " fired.At:("
          + std::to_string(tank.get_pos().get_x()) + "," + std::to_string(tank.get_pos().get_y()) + ").");
        fire(tank, 2, 2);
        tank_status_changed = true;
        break;
      case Event::NOTHING:
        break;
      case Event::QUIT:
        logger::move_cursor(0, map.get_height() + 1);
        CZH_NOTICE("Quitting.");
        exit(0);
        break;
      default:
        break;
      }
      //auto tank
      for (auto it = auto_tanks.begin(); it < auto_tanks.end(); ++it)
      {
        if (!it->is_alive()) continue;

        bool target_is_alive = true;
        if (it->target_is_auto())
          target_is_alive = auto_tanks[it->get_target_id()].is_alive();
        else
          target_is_alive = tanks[it->get_target_id()].is_alive();
        if (!it->get_found() || !target_is_alive)
          //have not been found or target is not alive should target/retarget
        {
          map::Pos target_pos;
          std::size_t target_id = 0;
          tank::TankType target_type = tank::TankType::AUTO;
          do
          {
            target_id = map::random(0, tanks.size() + auto_tanks.size());
            if (target_id < tanks.size())
            {
              target_pos = tanks[target_id].get_pos();
              target_type = tank::TankType::TANK;
            }
            else if (target_id - tanks.size() < auto_tanks.size())
            {
              target_id -= tanks.size();
              target_pos = auto_tanks[target_id].get_pos();
              target_type = tank::TankType::AUTO;
            }
          } while (target_type == tank::TankType::AUTO && target_id == it->get_id());
          it->target(map, target_type, target_id, target_pos);
          tank_status_changed = true;
        }

        //not in firing line or too close should correct its way
        tank::Tank* target = nullptr;
        if (it->target_is_auto())
          target = &auto_tanks[it->get_target_id()];
        else
          target = &tanks[it->get_target_id()];
        bool around_is_in_firing_line = it->get_around_target_pos().get_x() == target->get_pos().get_x()
          || it->get_around_target_pos().get_y() == target->get_pos().get_y();
        bool should_correct = false;
        if (it->get_correct())
        {
          if (!(it->get_pos().get_x() == target->get_pos().get_x()
            || it->get_pos().get_y() == target->get_pos().get_y()))// pos err
            should_correct = true;
        }
        else if (!(it->get_around_target_pos().get_x() == target->get_pos().get_x()
          || it->get_around_target_pos().get_y() == target->get_pos().get_y()))//around target pos err
          should_correct = true;
        else if (tank::get_distance(it->get_around_target_pos(), target->get_pos()) < 3)//distance too big
          should_correct = true;

        if (should_correct)
        {
          it->target(map, target->get_type(), target->get_id(), target->get_pos());
          tank_status_changed = true;
        }
        switch (it->next())
        {
        case map::AutoTankEvent::UP:
            it->up(map, changes);
            CZH_NOTICE("Auto-Tank " + std::to_string(it->get_id()) + " moved up.At:("
              + std::to_string(it->get_pos().get_x()) + "," + std::to_string(it->get_pos().get_y()) + ").");
            tank_status_changed = true;
            break;
        case map::AutoTankEvent::DOWN:
            it->down(map, changes);
            CZH_NOTICE("Auto-Tank " + std::to_string(it->get_id()) + " moved down.At:("
              + std::to_string(it->get_pos().get_x()) + "," + std::to_string(it->get_pos().get_y()) + ").");
            tank_status_changed = true;
            break;
        case map::AutoTankEvent::LEFT:
            it->left(map, changes);
            CZH_NOTICE("Auto-Tank " + std::to_string(it->get_id()) + " moved left.At:("
              + std::to_string(it->get_pos().get_x()) + "," + std::to_string(it->get_pos().get_y()) + ").");
            tank_status_changed = true;
            break;
        case map::AutoTankEvent::RIGHT:
            it->right(map, changes);
            CZH_NOTICE("Auto-Tank " + std::to_string(it->get_id()) + " moved right.At:("
              + std::to_string(it->get_pos().get_x()) + "," + std::to_string(it->get_pos().get_y()) + ").");
            tank_status_changed = true;
            break;
        case map::AutoTankEvent::STOP:
          CZH_NOTICE("Auto Tank " + std::to_string(it->get_id()) + " fired.At:("
            + std::to_string(it->get_pos().get_x()) + "," + std::to_string(it->get_pos().get_y()) + ").");
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
          for_all_bullets(it->get_pos().get_x(), it->get_pos().get_y(), [this](std::vector<bullet::Bullet>::iterator& it)
            {
              for (int i = it->get_pos().get_x() - it->get_circle(); i < it->get_pos().get_x() + it->get_circle(); ++i)
              {
                for (int j = it->get_pos().get_y() - it->get_circle(); j < it->get_pos().get_y() + it->get_circle(); ++j)
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
        }
        else
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
        }
        else
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
        [this](bullet::Bullet& bullet)
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
        }
        else
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
      if (all_over)
      {
        logger::move_cursor(0, map.get_height() + 1);
        CZH_NOTICE("Game Over.");
        //exit(0);
      }
      return *this;
    }
  private:
    map::Pos get_random_pos()
    {
      map::Pos pos;
      bool repeated = false;
      do
      {
        pos = map::Pos(map::random(1, map.get_width() - 1), map::random(1, map.get_height() - 1));
        for (auto& p : tanks)
        {
          if (!p.is_alive()) continue;
          if (p.get_pos() == pos)
          {
            repeated = true;
            break;
          }
        }
        for (auto& p : auto_tanks)
        {
          if (!p.is_alive()) continue;
          if (repeated) break;
          if (p.get_pos() == pos)
          {
            repeated = true;
            break;
          }
        }
      } while (repeated);
      return pos;
    }
    void update(const map::Pos& pos)
    {
      logger::move_cursor(pos.get_x(), map.get_height() - pos.get_y() - 1);
      auto& point = pos.get_point(map.get_map());
      if (point.has(map::Status::TANK))
      {
        auto it = find_tank(pos.get_x(), pos.get_y());
        int w = 0;
        if (it == tanks.end())
        {
          auto ita = find_auto_tank(pos.get_x(), pos.get_y());
          w = ita - auto_tanks.begin();
          std::cout << colorify_auto_tank(w);
        }
        else
        {
          w = it - tanks.begin();
          std::cout << colorify_tank(w);
        }
      }
      else if (point.has(map::Status::BULLET))
      {
        auto w = find_bullet(pos.get_x(), pos.get_y());
        if(w->is_from_auto_tank())
          std::cout << colorify_auto_tank_text(w->get_id(), w->get_text());
        else
          std::cout << colorify_tank_text(w->get_id(), w->get_text());
      }
      else if (point.has(map::Status::WALL))
      {
        std::cout << colorify_wall() << std::flush;
      }
      else
      {
        std::cout << colorify_space();
      }
    }
    void paint()
    {
      if (!inited)
      {
        logger::move_cursor(map.get_width() + 1, 0);
        std::cout << "   Tank - by caozhanhao";
        logger::move_cursor(0, 0);
        for (int j = map.get_height() - 1; j >= 0; --j)
        {
          for (int i = 0; i < map.get_width(); ++i)
          {
            update(map::Pos(i, j));
          }
          std::cout << "\n";
        }
        inited = true;
      }
      else
      {
        for (auto& p : changes)
         update(p);
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
          sout = "Blood: " + blood
            + " Pos: (" + x + "," + y  + ")";
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
          sout = "Blood: " + blood
            + " Pos: (" + x + "," + y + ")"
            + " Level: " + level;

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
      std::cout << std::flush;
    }
    void fire(const tank::Tank& tank, int circle = 1, int blood =  1, int range = 1000)
    {
      auto& point = tank.get_pos().get_point(map.get_map());
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
      auto& bullet_point = pos.get_point(map.get_map());
      if (bullet_point.has(map::Status::WALL)) return;
      bullet_point.add_status(map::Status::BULLET);
      bullets.emplace_back(bullet::Bullet(tank.is_auto(), tank.get_id(), pos, tank.get_Direction(), tank.get_lethality(), circle, blood, range));
    }
    bool check_tank_alive(const std::size_t tankid)
    {
      if (!tanks[tankid] .is_alive())
      {
        CZH_NOTICE("Tank " + std::to_string(tankid) + " is not alive.");
        return false;
      }
      return true;
    }
    std::vector<bullet::Bullet>::iterator find_bullet(int i, int j)
    {
      return std::find_if(bullets.begin(), bullets.end(),
        [i, j](const bullet::Bullet& b) {return (b.get_pos().get_x() == i && b.get_pos().get_y() == j); });
    }
    std::vector<tank::Tank>::iterator find_tank(int i, int j)
    {
      return std::find_if(tanks.begin(), tanks.end(),
        [i, j](const tank::Tank& b) {return (b.get_pos().get_x() == i && b.get_pos().get_y() == j); });
    }
    std::vector<tank::AutoTank>::iterator find_auto_tank(int i, int j)
    {
      return std::find_if(auto_tanks.begin(), auto_tanks.end(),
        [i, j](const tank::Tank& b) {return (b.get_pos().get_x() == i && b.get_pos().get_y() == j); });
    }
    void for_all_bullets(int i, int j, const std::function<void(std::vector<bullet::Bullet>::iterator&)>& func)
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