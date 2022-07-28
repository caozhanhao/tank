#pragma once
#include "tmap.h"
#include <map>
#include <algorithm>
namespace czh::tank
{
  enum class TankType
  {
    AUTO, NORMAL
  };
  enum class NormalTankEvent
  {
    UP, DOWN, LEFT, RIGHT, FIRE
  };
  enum class AutoTankEvent
  {
    UP, DOWN, LEFT, RIGHT, FIRE, NOTHING
  };
  class Tank
  {
  protected:
    int max_blood;
    int blood;
    int lethality;
    map::Pos pos;
    std::size_t id;
    map::Direction direction;
    bool hascleared;
    TankType type;
    std::string name;
    int last_blood;
    int delay;
  public:
    Tank(int blood_, int lethality_, map::Map &map, std::vector<map::Change> &changes,
         map::Pos pos_, std::size_t id_,TankType type_, const std::string& name_)
        : max_blood(blood_), blood(blood_), lethality(lethality_), direction(map::Direction::UP), pos(pos_),
          hascleared(false), id(id_), type(type_), last_blood(0), delay(0),
          name(name_)
    {
      pos.get_point(map.get_map()).add_status(map::Status::TANK);
      changes.emplace_back(map::Change(pos));
    }
    
    void up(map::Map &map, std::vector<map::Change> &changes)
    {
      int a = map.up(map::Status::TANK, pos);
      direction = map::Direction::UP;
      if (a == 0)
      {
        changes.emplace_back(map::Change(pos));
        changes.emplace_back(map::Change(map::Pos(pos.get_x(), pos.get_y() - 1)));
      }
    }
    
    void down(map::Map &map, std::vector<map::Change> &changes)
    {
      int a = map.down(map::Status::TANK, pos);
      direction = map::Direction::DOWN;
      if (a == 0)
      {
        changes.emplace_back(map::Change(pos));
        changes.emplace_back(map::Change(map::Pos(pos.get_x(), pos.get_y() + 1)));
      }
    }
    
    void left(map::Map &map, std::vector<map::Change> &changes)
    {
      int a = map.left(map::Status::TANK, pos);
      direction = map::Direction::LEFT;
      if (a == 0)
      {
        changes.emplace_back(map::Change(pos));
        changes.emplace_back(map::Change(map::Pos(pos.get_x() + 1, pos.get_y())));
      }
    }
    
    void right(map::Map &map, std::vector<map::Change> &changes)
    {
      int a = map.right(map::Status::TANK, pos);
      direction = map::Direction::RIGHT;
      if (a == 0)
      {
        changes.emplace_back(map::Change(pos));
        changes.emplace_back(map::Change(map::Pos(pos.get_x() - 1, pos.get_y())));
      }
    }
    
    [[nodiscard]]bool is_auto() const
    {
      return type == TankType::AUTO;
    }
    
    [[nodiscard]]std::size_t get_id() const
    {
      return id;
    }
    std::string& get_name()
    {
      return name;
    }
    const std::string& get_name() const
    {
      return name;
    }
    [[nodiscard]]int get_blood() const
    { return blood; }
    
    [[nodiscard]]int get_lethality() const
    { return lethality * map::random(5, 16) / 10; }
    
    [[nodiscard]]bool is_alive() const
    {
      return blood > 0;
    }
    
    [[nodiscard]]bool has_cleared() const
    {
      return hascleared;
    }
    
    void clear()
    { hascleared = true; }
    
    map::Pos &get_pos()
    {
      return pos;
    }
    
    void attacked(int lethality_)
    {
      blood -= lethality_;
      if (blood < 0) blood = 0;
      if (blood > max_blood) blood = max_blood;
    }
    
    [[nodiscard]]const map::Pos &get_pos() const
    {
      return pos;
    }
    
    [[nodiscard]]map::Direction& get_direction()
    {
      return direction;
    }
    [[nodiscard]]const map::Direction& get_direction() const
    {
      return direction;
    }
    [[nodiscard]]TankType get_type() const
    {
      return type;
    }
    
    void mark_blood()
    {
      last_blood = get_blood();
    }
    
    [[nodiscard]]bool has_been_attacked_since_marked() const
    {
      return last_blood != get_blood();
    }
    
    int &get_delay()
    {
      return delay;
    }
    virtual std::string colorify_text(const std::string& str) = 0;
    virtual std::string colorify_tank() = 0;
  };
  class NormalTank : public Tank
  {
  public:
    NormalTank(int blood_, int lethality_, map::Map &map, std::vector<map::Change> &changes,
               map::Pos pos_, std::size_t id_)
        :Tank(blood_, lethality_, map, changes, pos_, id_, TankType::NORMAL,
              "Tank " + std::to_string(id_)){}
    
    void revive(map::Map &map, std::vector<map::Change> &changes, const map::Pos& newpos)
    {
      if(is_alive() && !hascleared) return;
      blood = max_blood;
      hascleared = false;
      pos = newpos;
      pos.get_point(map.get_map()).add_status(map::Status::TANK);
      changes.emplace_back(map::Change(newpos));
    }
    std::string colorify_text(const std::string& str) override
    {
      int w = id % 2;
      std::string ret = "\033[";
      ret += std::to_string(w + 36);
      ret += "m";
      ret += str;
      ret += "\033[0m\033[?25l";
      return ret;
    }
    std::string colorify_tank() override
    {
      int w = id % 2;
      std::string ret = "\033[";
      ret += std::to_string(w + 46);
      ret += ";36m";
      ret += " \033[0m\033[?25l";
      return ret;
    }
  };
  
  AutoTankEvent get_pos_direction(const map::Pos &from, const map::Pos &to)
  {
    int x = (int) from.get_x() - (int) to.get_x();
    int y = (int) from.get_y() - (int) to.get_y();
    if (x > 0)
      return AutoTankEvent::LEFT;
    else if (x < 0)
      return AutoTankEvent::RIGHT;
    else if (y > 0)
      return AutoTankEvent::DOWN;
    return AutoTankEvent::UP;
  }
  
  class Node
  {
  private:
    map::Pos pos;
    map::Pos last;
    int G;
    bool root;
  public:
    Node() : G(0), root(false)
    {}
    
    Node(map::Pos pos_, int G_, const map::Pos &last_, bool root_ = false)
        : pos(pos_), G(G_), last(last_), root(root_)
    {}
    
    Node(const Node &node) = default;
    
    [[nodiscard]]int get_F(const map::Pos &dest) const
    {
      return G + (int) map::get_distance(dest, pos) * 10;
    }
    
    int &get_G()
    {
      return G;
    }
    
    map::Pos &get_last()
    {
      return last;
    }
    
    [[nodiscard]]const map::Pos &get_pos() const
    {
      return pos;
    }
    
    [[nodiscard]]bool is_root() const
    {
      return root;
    }
    
    std::vector<Node> get_neighbors(map::Map &map) const
    {
      std::vector<Node> ret;
      map::Pos pos_up(pos.get_x(), pos.get_y() + 1);
      map::Pos pos_down(pos.get_x(), pos.get_y() - 1);
      map::Pos pos_left(pos.get_x() - 1, pos.get_y());
      map::Pos pos_right(pos.get_x() + 1, pos.get_y());
      if (check(map, pos_up))
        ret.emplace_back(Node(pos_up, G + 10, pos));
      if (check(map, pos_down))
        ret.emplace_back(Node(pos_down, G + 10, pos));
      if (check(map, pos_left))
        ret.emplace_back(Node(pos_left, G + 10, pos));
      if (check(map, pos_right))
        ret.emplace_back(Node(pos_right, G + 10, pos));
      return ret;
    }
  
  private:
    static bool check(map::Map &map, map::Pos &pos)
    {
      return map.check_pos(pos)
             && !pos.get_point(map.get_map()).has(map::Status::WALL)
             && !pos.get_point(map.get_map()).has(map::Status::TANK);
    }
  };
  
  bool operator<(const Node &n1, const Node &n2)
  {
    return n1.get_pos() < n2.get_pos();
  }
  
  bool is_in_firing_line(map::Map &map, const map::Pos& pos, const map::Pos& target_pos)
  {
    int x = (int)target_pos.get_x() - (int)pos.get_x();
    int y = (int)target_pos.get_y() - (int)pos.get_y();
    if(x == 0 && std::abs(y) > 1)
    {
      std::size_t small =  y > 0 ? pos.get_y():target_pos.get_y();
      std::size_t big =  y < 0 ? pos.get_y():target_pos.get_y();
      for (std::size_t i = small + 1; i < big; ++i)
      {
        if(map.get_map()[pos.get_x()][i].has(map::Status::WALL)
           || map.get_map()[pos.get_x()][i].has(map::Status::TANK))
          return false;
      }
    }
    else if(y == 0 && std::abs(x) > 1)
    {
      std::size_t small =  x > 0 ? pos.get_x() : target_pos.get_x();
      std::size_t big = x < 0 ? pos.get_x():target_pos.get_x();
      for (std::size_t i = small + 1; i < big; ++i)
      {
        if(map.get_map()[i][pos.get_y()].has(map::Status::WALL)
           || map.get_map()[i][pos.get_y()].has(map::Status::TANK))
          return false;
      }
    }
    else if(std::abs(x) <=1 && std::abs(y) <= 1)
      return true;
    else
      return false;
    return true;
  }
  class AutoTank : public Tank
  {
  private:
    std::size_t target_pos_in_vec;
    map::Pos target_pos;
    std::vector<AutoTankEvent> way;
    std::size_t waypos;
    bool found;
    bool correct_direction;
    std::size_t level;
    std::size_t count;
  public:
    AutoTank(int blood_, int lethality_, map::Map &map, std::vector<map::Change> &changes, map::Pos pos_,
             std::size_t level_, std::size_t id_)
        : Tank(blood_, lethality_, map, changes, pos_, id_,
               TankType::AUTO, "Auto Tank " + std::to_string(id_)),
          found(false), correct_direction(false),
          waypos(0), target_pos_in_vec(0), level(level_), count(0)
    {}
    void target(map::Map &map, std::size_t target_pos_in_vec_, const map::Pos& target_pos_)
    {
      correct_direction = false;
      target_pos_in_vec = target_pos_in_vec_;
      target_pos = target_pos_;
      std::multimap<int, Node> open_list;
      std::map<map::Pos, Node> close_list;
      Node beg(get_pos(), 0, {0, 0}, true);
      open_list.insert({beg.get_F(target_pos), beg});
      while (!open_list.empty())
      {
        auto it = open_list.begin();
        auto curr = close_list.insert({it->second.get_pos(), it->second});
        open_list.erase(it);
        auto nodes = curr.first->second.get_neighbors(map);
        //open_list.clear();
        for (auto &node: nodes)
        {
          auto cit = close_list.find(node.get_pos());
          auto oit = std::find_if(open_list.begin(), open_list.end(),
                                  [&node](const std::pair<int, Node> &p)
                                  {
                                    return p.second.get_pos() == node.get_pos();
                                  });
          if (cit == close_list.end())
          {
            if (oit == open_list.end())
            {
              open_list.insert({node.get_F(target_pos), node});
            } else
            {
              if (oit->second.get_G() > node.get_G() + 10) //less G
              {
                oit->second.get_G() = node.get_G() + 10;
                oit->second.get_last() = node.get_pos();
                int F = oit->second.get_F(target_pos);
                auto n = open_list.extract(oit);
                n.key() = F;
                open_list.insert(std::move(n));
              }
            }
          }
        }
        auto itt = std::find_if(open_list.begin(), open_list.end(),
                                [this, &map](const std::pair<int, Node> &p)
                                {
                                  return tank::is_in_firing_line(map, p.second.get_pos(), target_pos);
                                });
        if (itt != open_list.end())//found
        {
          way.clear();
          waypos = 0;
          auto &np = itt->second;
          while (!np.is_root() && np.get_pos() != np.get_last())
          {
            way.insert(way.begin(), get_pos_direction(close_list[np.get_last()].get_pos(), np.get_pos()));
            np = close_list[np.get_last()];
          }
          found = true;
          return;
        }
      }
    }
    
    AutoTankEvent next()
    {
      if (!found)
        return AutoTankEvent::FIRE;
      if (++count < 10 - level)
        return AutoTankEvent::NOTHING;
      else
        count = 0;
      
      if (waypos < way.size())
      {
        auto ret = way[waypos];
        ++waypos;
        return ret;
      } else if (!correct_direction)
      {
        correct_direction = true;
        int x = (int) get_pos().get_x() - (int) target_pos.get_x();
        int y = (int) get_pos().get_y() - (int) target_pos.get_y();
        if (x > 0)
          get_direction() =  map::Direction::LEFT;
        else if (x < 0)
          get_direction() =  map::Direction::RIGHT;
        else if (y < 0)
          get_direction() = map::Direction::UP;
        else if (y > 0)
          get_direction() =  map::Direction::DOWN;
      }
      
      return AutoTankEvent::FIRE;
    }
    
    std::size_t &get_target_pos_in_vec()
    {
      return target_pos_in_vec;
    }
    
    map::Pos &get_target_pos()
    {
      return target_pos;
    }
    
    [[nodiscard]]bool get_found() const
    {
      return found;
    }
    
    bool &has_arrived()
    {
      return correct_direction;
    }
    
    [[nodiscard]]std::size_t get_level() const
    {
      return level;
    }
    
    std::string colorify_text(const std::string& str) override
    {
      int w = id % 4;
      std::string ret = "\033[";
      ret += std::to_string(w + 32);
      ret += "m";
      ret += str;
      ret += "\033[0m\033[?25l";
      return ret;
    }
    std::string colorify_tank() override
    {
      int w = id % 4;
      std::string ret = "\033[";
      ret += std::to_string(w + 42);
      ret += ";31m";
      ret += " \033[0m\033[?25l";
      return ret;
    }
  };
}