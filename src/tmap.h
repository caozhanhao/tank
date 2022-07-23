#pragma once
#include <vector>
#include <algorithm>
#include <functional>
#include <random>
#include <ctime>
namespace czh::map
{
  int random(int a, int b)
  {
    static std::default_random_engine e(time(0));
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
  enum class AutoTankEvent
  {
    UP, DOWN, LEFT, RIGHT, STOP, NOTHING
  };
  class Point
  {
  private:
    std::vector<Status> statuses;
    int lethality;
  public:
    void add_status(const Status& status)
    {
      statuses.emplace_back(status);
    }
    void remove_status(const Status& status)
    {
      statuses.erase(std::remove(statuses.begin(), statuses.end(), status), statuses.end());
    }
    const std::vector<Status>& get_statuses() const
    {
      return statuses;
    }
    bool has(const Status& status) const
    {
      return (std::find(statuses.cbegin(), statuses.cend(), status) != statuses.cend());
    }
    void attacked(int lethality_)
    {
      lethality += lethality_;
    }
    int get_lethality() const
    {
      return lethality;
    }
    void remove_lethality()
    {
      lethality = 0;
    }
    std::size_t count(const Status& status) const
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
    Pos() : x(0), y(0){}
    Pos(std::size_t x_, std::size_t y_) : x(x_), y(y_){}
    Point& get_point(std::vector<std::vector<Point>>& map) const
    {
      return map[x][y];
    }
    std::size_t& get_x()
    {
      return x;
    }
    std::size_t& get_y()
    {
      return y;
    }
    const std::size_t& get_x() const
    {
      return x;
    }
    const std::size_t& get_y() const
    {
      return y;
    }
    bool operator==(const Pos& pos) const
    {
      return (x == pos.get_x() && y == pos.get_y());
    }
  };
  class Map
  {
  private:
    std::size_t height;
    std::size_t width;
    std::vector<std::vector<Point>> map;
  public:
    Map(): height(29), width(60), map(width)
    {
      for (auto& r : map)
      {
        r.resize(height);
      }
      for (int i = 0; i < width; ++i)
      {
        map[i][0].add_status(map::Status::WALL);
        map[i][height - 1].add_status(map::Status::WALL);
      }
      for (int i = 1; i < height - 1; ++i)
      {
        map[0][i].add_status(map::Status::WALL);
        map[width - 1][i].add_status(map::Status::WALL);
      }
    }
    auto get_width() const { return width; }
    auto get_height() const { return height; }
    int up(const Status& status, Pos& pos)
    {
      return move(status, pos, [](Pos& pos) {pos.get_y()++; });
    }
    int down(const Status& status, Pos& pos)
    {
      return move(status, pos, [](Pos& pos) {pos.get_y()--; });
    }
    int left(const Status& status, Pos& pos)
    {
      return move(status, pos, [](Pos& pos) {pos.get_x()--; });
    }
    int right(const Status& status, Pos& pos)
    {
      return move(status, pos, [](Pos& pos) {pos.get_x()++; });
    }
    std::vector<std::vector<Point>>& get_map()
    {
      return map;
    }
  private:
    int move(const Status& status, Pos& pos, const std::function<void(Pos& pos)>& func)
    {
      Pos posbak = pos;
      func(posbak);
      auto& pointbak = posbak.get_point(map);
      switch (status)
      {
      case Status::BULLET:
        if (pointbak.has(Status::WALL)) return -1;
        break;
      case Status::TANK:
        if (pointbak.has(Status::WALL) || pointbak.has(Status::TANK)) return -1;
        break;
      default:
        break;
      }
      auto& point = pos.get_point(map);
      point.remove_status(status);
      func(pos);
      auto& point_next = pos.get_point(map);
      point_next.add_status(status);
      return 0;
    }
  };
}