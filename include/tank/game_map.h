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
#ifndef TANK_GAME_MAP_H
#define TANK_GAME_MAP_H

#include "info.h"
#include <vector>
#include <list>
#include <set>
#include <algorithm>
#include <variant>
#include <random>
#include <map>
#include <optional>
#include <variant>

namespace czh::tank
{
  enum class NormalTankEvent
  {
    UP, DOWN, LEFT, RIGHT, FIRE
  };
  enum class AutoTankEvent
  {
    UP, DOWN, LEFT, RIGHT, FIRE, PASS
  };
  class Tank;
}
namespace czh::bullet
{
  class Bullet;
}
namespace czh::map {
    enum class Status {
        WALL, TANK, BULLET, END
    };
    enum class Direction {
        UP, DOWN, LEFT, RIGHT
    };

    class Pos {
    public:
        int x;
        int y;
    public:
        Pos() : x(0), y(0) {}

        Pos(int x_, int y_) : x(x_), y(y_) {}

        bool operator==(const Pos &pos) const;

        bool operator!=(const Pos &pos) const;
    };

    bool operator<(const Pos &pos1, const Pos &pos2);

    std::size_t get_distance(const map::Pos &from, const map::Pos &to);

    class Change {
    private:
        Pos pos;
    public:
        explicit Change(Pos pos_) : pos(pos_) {}

        const Pos &get_pos() const;

        Pos &get_pos();
    };

    bool operator<(const Change &c1, const Change &c2);

    struct BulletData {
        map::Pos pos;
        map::Direction direction;
        int from_tank_id;
        info::BulletInfo info;
    };

    struct NormalTankData {
    };
    struct AutoTankData {
        std::size_t target_id;
        map::Pos target_pos;
        map::Pos destination_pos;
        std::vector<tank::AutoTankEvent> way;
        std::size_t waypos;
        int gap_count;
    };

    struct TankData {
        info::TankInfo info;
        int hp;
        map::Pos pos;
        map::Direction direction;
        bool hascleared;

        std::variant<NormalTankData, AutoTankData> data;

        bool is_auto() const {
            return data.index() == 1;
        }
    };

    struct ActivePointData {
        tank::Tank *tank;
        std::vector<bullet::Bullet *> bullets;
    };

    struct InactivePointData {
        TankData tank;
        std::vector<BulletData> bullets;
    };

    class Map;

    class Point {
        friend class Map;

    private:
        bool generated;
        std::vector<Status> statuses;
        std::variant<ActivePointData, InactivePointData> data;
    public:
        Point() : generated(false) {}

        Point(std::string, std::vector<Status> s) : generated(true), statuses(std::move(s)) {}

        bool is_generated() const;

        bool is_active() const;

        tank::Tank *get_tank_instance() const;

        const std::vector<bullet::Bullet *> &get_bullets_instance() const;

        const TankData &get_tank_data() const;

        const std::vector<BulletData> &get_bullets_data() const;

        void activate(const ActivePointData &data);

        void deactivate(const InactivePointData &data);

        void add_status(const Status &status, void *);

        void remove_status(const Status &status);

        void remove_all_statuses();

        [[nodiscard]] bool has(const Status &status) const;

        [[nodiscard]]std::size_t count(const Status &status) const;
    };

    extern Point empty_point;
    extern Point wall_point;

    class Map {
    private:
        std::map<Pos, Point> map;
        std::set<Change> changes;
    public:
        Map();

        int tank_up(const Pos &pos);

        int tank_down(const Pos &pos);

        int tank_left(const Pos &pos);

        int tank_right(const Pos &pos);

        int bullet_up(bullet::Bullet *b, const Pos &pos);

        int bullet_down(bullet::Bullet *b, const Pos &pos);

        int bullet_left(bullet::Bullet *b, const Pos &pos);

        int bullet_right(bullet::Bullet *b, const Pos &pos);

        int add_tank(tank::Tank *, const Pos &pos);

        int add_bullet(bullet::Bullet *, const Pos &pos);

        void remove_status(const Status &status, const Pos &pos);

        bool has(const Status &status, const Pos &pos) const;

        int count(const Status &status, const Pos &pos) const;

        const std::set<Change> &get_changes() const;

        void clear_changes();

        int fill(const Pos &from, const Pos &to, const Status &status = Status::END);

        const Point &at(const Pos &i) const;

        const Point &at(int x, int y) const;

    private:
        int tank_move(const Pos &pos, int direction);

        int bullet_move(bullet::Bullet *, const Pos &pos, int direction);
    };
}
#endif