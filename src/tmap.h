#pragma once
#include <vector>
#include <algorithm>
#include <functional>
#include <random>
namespace czh::map
{
  int random(int a, int b)
  {
    std::random_device r;
    std::default_random_engine e(r());
    std::uniform_int_distribution<int> u(a, b - 1);
    return u(e);
  }
  
  enum class Status
  {
    WALL, TANK, BULLET
  };
  enum class Direction
  {
    UP, DOWN, LEFT, RIGHT
  };
  
  class Point
  {
  private:
    std::vector<Status> statuses;
    int lethality;
  public:
    void add_status(const Status &status)
    {
      statuses.emplace_back(status);
    }
    
    void remove_status(const Status &status)
    {
      statuses.erase(std::remove(statuses.begin(), statuses.end(), status), statuses.end());
    }
    
    [[nodiscard]] bool has(const Status &status) const
    {
      return (std::find(statuses.cbegin(), statuses.cend(), status) != statuses.cend());
    }
    
    void attacked(int lethality_)
    {
      lethality += lethality_;
    }
    
    [[nodiscard]]int get_lethality() const
    {
      return lethality;
    }
    
    void remove_lethality()
    {
      lethality = 0;
    }
    
    [[nodiscard]]std::size_t count(const Status &status) const
    {
      return std::count(statuses.cbegin(), statuses.cend(), status);
    }
  };
  
  class Pos
  {
  private:
    std::size_t x;
    std::size_t y;
  public:
    Pos() : x(0), y(0)
    {}
    
    Pos(std::size_t x_, std::size_t y_) : x(x_), y(y_)
    {}
    
    Point &get_point(std::vector<std::vector<Point>> &map) const
    {
      return map[x][y];
    }
    
    std::size_t &get_x()
    {
      return x;
    }
    
    std::size_t &get_y()
    {
      return y;
    }
    
    [[nodiscard]]const std::size_t &get_x() const
    {
      return x;
    }
    
    [[nodiscard]]const std::size_t &get_y() const
    {
      return y;
    }
    
    bool operator==(const Pos &pos) const
    {
      return (x == pos.get_x() && y == pos.get_y());
    }
    
    bool operator!=(const Pos &pos) const
    {
      return !(*this == pos);
    }
  };
  
  bool operator<(const Pos &pos1, const Pos &pos2)
  {
    if (pos1.get_x() == pos2.get_x())
      return pos1.get_y() < pos2.get_y();
    return pos1.get_x() < pos2.get_x();
  }
  
  std::size_t get_distance(const map::Pos &from, const map::Pos &to)
  {
    return std::abs(int(from.get_x() - to.get_x())) + std::abs(int(from.get_y() - to.get_y()));
  }
  
  class Change
  {
  private:
    Pos pos;
  public:
    explicit Change(Pos pos_) : pos(pos_)
    {}
    
    Pos &get_pos()
    { return pos; }
  };
  
  class Map
  {
  private:
    std::size_t height;
    std::size_t width;
    std::vector<std::vector<Point>> map;
  public:
    Map(std::size_t height_, std::size_t width_)
        : height(height_), width(width_), map(width)
    {
      for (auto &r: map)
      {
        r.resize(height);
      }
      //for (int i = 0; i < width; ++i)
      //{
      //  map[i][0].add_status(map::Status::WALL);
      //  map[i][height - 1].add_status(map::Status::WALL);
      //}
      //for (int i = 1; i < height - 1; ++i)
      //{
      //  map[0][i].add_status(map::Status::WALL);
      //  map[width - 1][i].add_status(map::Status::WALL);
      //}
      
      make_maze();
    }
    
    [[nodiscard]]auto get_width() const
    { return width; }
    
    [[nodiscard]] auto get_height() const
    { return height; }
    
    int up(const Status &status, Pos &pos)
    {
      return move(status, pos, [](Pos &pos)
      { pos.get_y()++; });
    }
    
    int down(const Status &status, Pos &pos)
    {
      return move(status, pos, [](Pos &pos)
      { pos.get_y()--; });
    }
    
    int left(const Status &status, Pos &pos)
    {
      return move(status, pos, [](Pos &pos)
      { pos.get_x()--; });
    }
    
    int right(const Status &status, Pos &pos)
    {
      return move(status, pos, [](Pos &pos)
      { pos.get_x()++; });
    }
    
    std::vector<std::vector<Point>> &get_map()
    {
      return map;
    }
    
    [[nodiscard]]bool check_pos(const Pos &pos) const
    {
      return pos.get_x() < width && pos.get_y() < height;
    }
  
  private:
    void make_maze()
    {
      for (int i = 0; i < width; ++i)
      {
        for (int j = 0; j < height; ++j)
        {
          if (i % 2 == 0 || j % 2 == 0)
            map[i][j].add_status(map::Status::WALL);
        }
      }
      std::vector<Pos> way{Pos((std::size_t) random(0, (int) width / 2) * 2 - 1,
                               (std::size_t) random(0, (int) height / 2) * 2 - 1)};
      auto it = way.begin();
      auto is_available = [this, &way](const Pos &pos)
      {
        return check_pos(pos)
               && !pos.get_point(map).has(Status::WALL)
               && std::find(way.begin(), way.end(), pos) == way.end();
      };
      auto next = [&is_available, &way, &it, this]() -> bool
      {
        std::vector<Pos> avail;
        auto &pos = *it;
        Pos up(pos.get_x(), pos.get_y() + 2);
        Pos down(pos.get_x(), pos.get_y() - 2);
        Pos left(pos.get_x() - 2, pos.get_y());
        Pos right(pos.get_x() + 2, pos.get_y());
        if (is_available(up)) avail.emplace_back(up);
        if (is_available(down)) avail.emplace_back(down);
        if (is_available(left)) avail.emplace_back(left);
        if (is_available(right)) avail.emplace_back(right);
        if (avail.empty()) return false;
        
        auto &result = avail[(std::size_t) random(0, (int) avail.size())];
        Pos midpos((result.get_x() + pos.get_x()) / 2, (result.get_y() + pos.get_y()) / 2);
        midpos.get_point(map).remove_status(Status::WALL);
        way.emplace_back(midpos);
        way.emplace_back(result);
        it = way.end() - 1;
        return true;
      };
      while (true)
      {
        if (!next())
        {
          if (it != way.begin())
            --it;
          else
            break;
        }
      }
      add_random_space();
    }
    
    void add_random_space()
    {
      auto add = [this](std::vector<Pos> &avail, std::size_t n, std::size_t space_height, std::size_t space_width)
      {
        for (std::size_t i = 0; i < n; ++i)
        {
          if (avail.empty()) return;
          Pos left_bottom(avail[random(0, (int) avail.size())]);
          avail.erase(std::remove_if(avail.begin(), avail.end(),
                                     [&left_bottom, &space_width, &space_height](const Pos &pos)
                                     {
                                       return std::abs((int) left_bottom.get_x() - (int) pos.get_x()) < space_width + 1
                                              && std::abs((int) left_bottom.get_y() - (int) pos.get_y()) <
                                                 space_height + 1;
                                     }
          ), avail.end());
          for (std::size_t j = left_bottom.get_x(); j < left_bottom.get_x() + space_width; ++j)
          {
            for (std::size_t k = left_bottom.get_y(); k < left_bottom.get_y() + space_height; ++k)
            {
              map[j][k].remove_status(Status::WALL);
            }
          }
        }
      };
      std::vector<Pos> large;
      std::vector<Pos> small;
      std::size_t large_height = height / 3;
      std::size_t large_width = width / 3;
      std::size_t small_height = height / 6;
      std::size_t small_width = width / 6;
      std::size_t nlarge = 2;
      std::size_t nsmall = 2;
      
      std::vector<Pos> avail;
      for (std::size_t i = 1; i < width - large_width - 1; ++i)
      {
        for (std::size_t j = 1; j < height - large_height - 1; ++j)
        {
          avail.emplace_back(Pos(i, j));
        }
      }
      add(avail, nlarge, large_height, large_width);
      add(avail, nsmall, small_height, small_width);
    }
    
    int move(const Status &status, Pos &pos, const std::function<void(Pos &pos)> &func)
    {
      if (!check_pos(pos))
        return -1;
      Pos posbak = pos;
      func(posbak);
      if (!check_pos(posbak))
        return -1;
      auto &pointbak = posbak.get_point(map);
      switch (status)
      {
        case Status::BULLET:if (pointbak.has(Status::WALL)) return -1;
          break;
        case Status::TANK:if (pointbak.has(Status::WALL) || pointbak.has(Status::TANK)) return -1;
          break;
        default:break;
      }
      auto &point = pos.get_point(map);
      point.remove_status(status);
      func(pos);
      auto &point_next = pos.get_point(map);
      point_next.add_status(status);
      return 0;
    }
  };
}