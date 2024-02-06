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

#include "tank/online.h"
#include "tank/message.h"
#include "tank/command.h"
#include "tank/game_map.h"
#include "tank/game.h"
#include "tank/globals.h"
#include "tank/renderer.h"
#include "tank/utils.h"

#include <string>
#include <chrono>
#include <utility>
#include <vector>

namespace czh::g
{
  online::TankServer online_server{};
  online::TankClient online_client{};
  int client_failed_attempts = 0;
  int delay = -1;
  std::map<uint32_t, std::tuple<online::MsgHeader, std::string>> buffer;
}
namespace czh::online
{
  Thpool::Thpool(std::size_t size) : run(true) { add_thread(size); }
  
  Thpool::~Thpool()
  {
    run = false;
    cond.notify_all();
    for (auto &th: pool)
    {
      if (th.joinable()) th.join();
    }
  }
  
  void Thpool::add_thread(std::size_t num)
  {
    for (std::size_t i = 0; i < num; i++)
    {
      pool.emplace_back(
          [this]
          {
            while (this->run)
            {
              Task task;
              {
                std::unique_lock<std::mutex> lock(this->th_mutex);
                this->cond.wait(lock, [this] { return !this->run || !this->tasks.empty(); });
                if (!this->run && this->tasks.empty()) return;
                task = std::move(this->tasks.front());
                this->tasks.pop();
              }
              task();
            }
          }
      );
    }
  }
  
  Addr::Addr() : len(sizeof(addr))
  {
    std::memset(&addr, 0, sizeof(addr));
  }
  
  Addr::Addr(struct sockaddr_in addr_, decltype(len) len_)
      : addr(addr_), len(len_) {}
  
  Addr::Addr(const std::string &ip, int port)
  {
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_port = htons(port);
    len = sizeof(addr);
  }
  
  Addr::Addr(int port)
  {
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    len = sizeof(addr);
  }
  
  std::string Addr::to_string() const
  {
    std::string str(16, '\0');
#ifdef _WIN32
    str = inet_ntoa(addr.sin_addr);
#else
    inet_ntop(AF_INET, &addr.sin_addr, str.data(), len);
#endif
    str += ":" + std::to_string(ntohs(addr.sin_port));
    return str;
  }
  
  SocketHandle::SocketHandle() : fd(-1)
  {
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    utils::tank_assert(fd != -1, "socket initialize failed.");
    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&on), sizeof(on));
    utils::tank_assert(fd != -1, "socket initialize failed.");
  }
  
  SocketHandle::SocketHandle(Socket_t fd_) : fd(fd_) {}
  
  SocketHandle::SocketHandle(SocketHandle && soc) noexcept: fd(soc.fd)
  {
    soc.fd = -1;
  }
  
  SocketHandle::~SocketHandle()
  {
    if (fd != -1)
    {
      ::close(fd);
      fd = -1;
    }
  }
  
  std::tuple<SocketHandle, Addr> SocketHandle::accept() const
  {
    Addr addr;
#ifdef _WIN32
    return {std::move(SocketHandle{::accept(fd, reinterpret_cast<sockaddr *>(&addr.addr),
                                          reinterpret_cast<int *> (&addr.len))}), addr};
#else
    return {std::move(SocketHandle{::accept(fd, reinterpret_cast<sockaddr *>(&addr.addr), reinterpret_cast<socklen_t *>(&addr.len))}), addr};
#endif
  }
  
  Socket_t SocketHandle::get_fd() const { return fd; }
  
  bool check(bool a)
  {
    if (!a)
    {
      msg::error(g::user_id, strerror(errno));
    }
    return a;
  }
  
  int send_all_data(Socket_t sock, const char* buf, int size)
  {
    while (size > 0)
    {
      int s = ::send(sock, buf, size, MSG_NOSIGNAL);
      if (!check(s >= 0)) return -1;
      size -= s;
      buf += s;
    }
    return 0;
  }
  
  int receive_all_data(Socket_t sock, char* buf, int size)
  {
    while (size > 0)
    {
      int r = ::recv(sock, buf, size, 0);
      if (!check(r > 0)) return -1;
      size -= r;
      buf += r;
    }
    return 0;
  }
  
  int SocketHandle::send(const std::string &str, uint32_t id) const
  {
    MsgHeader msg{.magic = htonl(MAGIC), .id = htonl(id), .content_length = htonl(str.size())};
    if (send_all_data(fd, reinterpret_cast<const char *>(&msg), sizeof(MsgHeader)) != 0) return -1;
    if (send_all_data(fd, str.data(), str.size()) != 0) return -1;
    return 0;
  }
  
  std::optional<std::tuple<MsgHeader, std::string>> SocketHandle::recv(uint32_t id) const
  {
    if(auto it = g::buffer.find(id); it!= g::buffer.end())
    {
      auto ret = it->second;
      g::buffer.erase(it);
      return ret;
    }
    
    MsgHeader header{};
    std::string recv_result;
    while (true)
    {
      if (receive_all_data(fd, reinterpret_cast<char *>(&header), sizeof(MsgHeader)) != 0)
        return std::nullopt;
      
      if (ntohl(header.magic) != MAGIC)
      {
        msg::warn(g::user_id, "Ignore invalid data");
        return std::nullopt;
      }
      
      recv_result.resize(ntohl(header.content_length), 0);
      
      if (receive_all_data(fd, reinterpret_cast<char *>(recv_result.data()), recv_result.size()) != 0)
        return std::nullopt;
      
      if(id == 0)
      {
        return std::make_tuple(header, recv_result);
      }
      else
      {
        if (ntohl(header.id) != id)
          break;
        else
        {
          g::buffer[ntohl(header.id)] = std::make_tuple(header, recv_result);
          recv_result.clear();
          header.id = 0;
          header.magic = 0;
          header.content_length = 0;
        }
      }
    }
    return std::make_tuple(header, recv_result);
  }
  
  int SocketHandle::bind(Addr addr) const
  {
    if (!check(::bind(fd, (sockaddr *) &addr.addr, addr.len) == 0))
    {
      return -1;
    }
    return 0;
  }
  
  int SocketHandle::listen() const
  {
    if (!check(::listen(fd, 0) != -1))
    {
      return -1;
    }
    return 0;
  }
  
  int SocketHandle::connect(Addr addr) const
  {
    if (!check(::connect(fd, (sockaddr *) &addr.addr, addr.len) != -1))
    {
      return -1;
    }
    return 0;
  }
  
  std::optional<Addr> SocketHandle::get_peer_addr() const
  {
    struct sockaddr_in peer_addr{};
    decltype(Addr::len) peer_len;
    peer_len = sizeof(peer_addr);
    if (!check(getpeername(fd, (struct sockaddr *) &peer_addr, &peer_len) != -1))
    {
      return std::nullopt;
    }
    return Addr{peer_addr, peer_len};
  }
  
  void SocketHandle::reset()
  {
    if (fd != -1)
      ::close(fd);
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&on), sizeof(on));
    utils::tank_assert(fd != -1, "socket reset failed.");
  }
  
  Socket_t SocketHandle::release()
  {
    int f = fd;
    fd = -1;
    return f;
  }
  
  Req::Req(std::string ip_, std::string content_)
      : ip(std::move(ip_)), content(std::move(content_)) {}
  
  const auto &Req::get_ip() const { return ip; }
  
  const auto &Req::get_content() const { return content; }
  
  void Res::set_content(const std::string c) { content = c; }
  
  const auto &Res::get_content() const { return content; }
  
  SocketServer::SocketServer() : thpool(16) {}
  
  SocketServer::SocketServer(const std::function<void(const Req &, Res &)> &router_)
      : router(router_), thpool(16), running(false) {}
  
  void SocketServer::init(const std::function<void(const Req &, Res &)> &router_)
  {
    router = router_;
    running = false;
  }
  
  void SocketServer::start(int port)
  {
    running = true;
    SocketHandle socket;
    socket.bind({port});
    socket.listen();
    while (running)
    {
      auto tmp = socket.accept();
      auto &[clnt_socket_, clnt_addr] = tmp;
      utils::tank_assert(clnt_socket_.get_fd() != -1, "socket accept failed");
      auto fd = clnt_socket_.release();
      
      thpool.add_task(
          [this, fd]
          {
            SocketHandle clnt_socket{fd};
            while (running)
            {
              auto request = clnt_socket.recv();
              auto [header, content] = *request;
              if (!request.has_value() || content == "quit")
                break;
              Res response;
              auto addr = clnt_socket.get_peer_addr();
              if (!addr.has_value())
                break;
              router(Req{addr->to_string(), content}, response);
              if (!response.get_content().empty())
              {
                clnt_socket.send(response.get_content(), header.id);
              }
            }
          });
    }
  }
  
  void SocketServer::stop()
  {
    running = false;
  }
  
  SocketClient::~SocketClient()
  {
    disconnect();
  }
  
  int SocketClient::disconnect()
  {
    return socket.send("quit");
  }
  
  
  int SocketClient::connect(const std::string &addr, int port)
  {
    return socket.connect({addr, port});
  }
  
  int SocketClient::send(const std::string &str)
  {
    return socket.send(str);
  }
  
  std::optional<std::string> SocketClient::recv()
  {
    auto a = socket.recv();
    if(a.has_value())
      return std::get<1>(*a);
    return std::nullopt;
  }
  
  std::optional<std::string> SocketClient::send_and_recv(const std::string &str)
  {
    uint16_t id = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    int ret = socket.send(str, id);
    if (ret != 0) return std::nullopt;
    return std::get<1>(*socket.recv(id));
  }
  
  void SocketClient::reset()
  {
    socket.reset();
  }
  
  
  std::string serialize_zone(const czh::map::Zone &b)
  {
    return utils::join(delim::zo, b.x_min, b.x_max, b.y_min, b.y_max);
  }
  
  czh::map::Zone deserialize_zone(const std::string &str)
  {
    auto s = czh::utils::split<std::vector<std::string_view>>(str, delim::zo);
    czh::map::Zone c;
    c.x_min = std::stoi(std::string{s[0]});
    c.x_max = std::stoi(std::string{s[1]});
    c.y_min = std::stoi(std::string{s[2]});
    c.y_max = std::stoi(std::string{s[3]});
    return c;
  }
  
  std::string serialize_changes(const std::set<map::Pos> &changes)
  {
    if (changes.empty()) return "e";
    std::string ret;
    for (auto &b: changes)
    {
      ret += utils::join(delim::c, b.x, b.y) + delim::cs;
    }
    return ret;
  }
  
  std::set<map::Pos> deserialize_changes(const std::string &str)
  {
    if (str == "e") return {};
    auto s1 = czh::utils::split<std::vector<std::string_view>>(str, delim::cs);
    std::set<map::Pos> ret;
    for (auto &r: s1)
    {
      auto s2 = czh::utils::split<std::vector<std::string_view>>(r, delim::c);
      ret.insert({std::stoi(std::string{s2[0]}), std::stoi(std::string{s2[1]})});
    }
    return ret;
  }
  
  std::string serialize_tanksview(const std::map<size_t, renderer::TankView> &view)
  {
    if (view.empty()) return "e";
    std::string ret;
    for (auto &b: view)
    {
      ret += utils::join(delim::tv,
                         b.second.info.name,
                         b.second.info.bullet.hp,
                         b.second.info.bullet.lethality,
                         b.second.info.bullet.range,
                         b.second.info.gap,
                         b.second.info.id,
                         b.second.info.max_hp,
                         static_cast<int>(b.second.info.type),
                         b.second.hp,
                         b.second.is_alive,
                         b.second.is_auto,
                         static_cast<int>(b.second.direction),
                         b.second.pos.x,
                         b.second.pos.y) + delim::tvs;
    }
    return ret;
  }
  
  std::map<size_t, renderer::TankView> deserialize_tanksview(const std::string &str)
  {
    if (str == "e") return {};
    auto s1 = czh::utils::split<std::vector<std::string_view>>(str, delim::tvs);
    std::map<size_t, renderer::TankView> ret;
    for (auto &r: s1)
    {
      auto s2 = czh::utils::split<std::vector<std::string_view>>(r, delim::tv);
      renderer::TankView c;
      c.info.name = s2[0];
      c.info.bullet.hp = std::stoi(std::string{s2[1]});
      c.info.bullet.lethality = std::stoi(std::string{s2[2]});
      c.info.bullet.range = std::stoi(std::string{s2[3]});
      c.info.gap = std::stoi(std::string{s2[4]});
      c.info.id = std::stoi(std::string{s2[5]});
      c.info.max_hp = std::stoi(std::string{s2[6]});
      c.info.type = static_cast<info::TankType>(std::stoi(std::string{s2[7]}));
      c.hp = std::stoi(std::string{s2[8]});
      c.is_alive = static_cast<bool>(std::stoi(std::string{s2[9]}));
      c.is_auto = static_cast<bool>(std::stoi(std::string{s2[10]}));
      c.direction = static_cast<map::Direction>(std::stoi(std::string{s2[11]}));
      c.pos.x = std::stoi(std::string{s2[12]});
      c.pos.y = std::stoi(std::string{s2[13]});
      
      ret.insert({c.info.id, c});
    }
    return ret;
  }
  
  std::string serialize_mapview(const renderer::MapView &view)
  {
    if (view.view.empty()) return "e";
    std::string ret;
    for (auto &b: view.view)
    {
      ret += utils::join(delim::pv,
                  b.second.pos.x,
                  b.second.pos.y,
                  b.second.tank_id,
                  (b.second.text.empty() ? "et" : b.second.text),
                  static_cast<int>(b.second.status)) + delim::mv;
    }
    return ret;
  }
  
  renderer::MapView deserialize_mapview(const std::string &str)
  {
    if (str == "e") return {};
    auto s1 = czh::utils::split<std::vector<std::string_view>>(str, delim::mv);
    renderer::MapView ret;
    for (auto &r: s1)
    {
      auto s2 = czh::utils::split<std::vector<std::string_view>>(r, delim::pv);
      czh::renderer::PointView c;
      c.pos.x = std::stoi(std::string{s2[0]});
      c.pos.y = std::stoi(std::string{s2[1]});
      c.tank_id = std::stoi(std::string{s2[2]});
      c.text = s2[3] == "et" ? "" : std::string{s2[3]};
      c.status = static_cast<czh::map::Status>(std::stoi(std::string{s2[4]}));
      ret.view.insert({c.pos, c});
    }
    return ret;
  }
  
  std::string serialize_messages(const std::deque<msg::Message> &msg)
  {
    if (msg.empty()) return "e";
    std::string ret;
    for (auto &b : msg)
    {
      ret += utils::join(delim::m, b.from, b.content) + delim::ms;
    }
    return ret;
  }
  
  std::deque<msg::Message> deserialize_messages(const std::string &str)
  {
    if (str == "e") return {};
    auto s1 = czh::utils::split<std::vector<std::string_view>>(str, delim::ms);
    std::deque<msg::Message> ret;
    for (auto &r: s1)
    {
      auto s2 = czh::utils::split<std::vector<std::string_view>>(r, delim::m);
      msg::Message c;
      c.from = std::stoi(std::string{s2[0]});
      c.content = std::string{s2[1]};
      ret.emplace_back(c);
    }
    return ret;
  }
  
  TankServer::TankServer()
  {
    svr.init([](const Req &req, Res &res)
             {
               auto s = utils::split<std::vector<std::string_view>>(req.get_content(), delim::req);
               if (s[0] == "tank_react")
               {
                 size_t id = std::stoi(std::string{s[1]});
                 tank::NormalTankEvent event =
                     static_cast<tank::NormalTankEvent>(std::stoi(std::string{s[2]}));
                 game::tank_react(id, event);
               }
               else if (s[0] == "update")
               {
                 auto beg = std::chrono::steady_clock::now();
                 std::lock_guard<std::mutex> l(g::mainloop_mtx);
                 size_t id = std::stoi(std::string{s[1]});
                 map::Zone zone = deserialize_zone(std::string{s[2]});
                 renderer::MapView map_view = renderer::extract_map(zone);
                 std::set<map::Pos> changes;
                 for (auto &r: g::userdata[id].map_changes)
                 {
                   if (zone.contains(r))
                   {
                     changes.insert(r);
                   }
                 }
                 auto d = std::chrono::duration_cast<std::chrono::milliseconds>
                     (std::chrono::steady_clock::now() - beg);
                 res.set_content(utils::join(delim::res,
                                             d.count(),
                                             serialize_changes(changes),
                                             serialize_mapview(map_view),
                                             serialize_tanksview(renderer::extract_tanks()),
                                             serialize_messages(g::userdata[id].messages)));
                 g::userdata[id].map_changes.clear();
                 g::userdata[id].messages.clear();
                 g::userdata[id].last_update = std::chrono::steady_clock::now();
               }
               else if (s[0] == "register")
               {
                 std::lock_guard<std::mutex> l(g::mainloop_mtx);
                 auto id = game::add_tank();
                 g::userdata[g::user_id] = game::UserData{.user_id = g::user_id};
                 msg::info(g::user_id, req.get_ip() + " connected as " + std::to_string(id));
                 res.set_content(std::to_string(id));
               }
               else if (s[0] == "deregister")
               {
                 std::lock_guard<std::mutex> l(g::mainloop_mtx);
                 int id = std::stoi(std::string{s[1]});
                 msg::info(-1, req.get_ip() + " (" + std::to_string(id) + ") disconnected.");
                 g::tanks[id]->kill();
                 g::tanks[id]->clear();
                 delete g::tanks[id];
                 g::tanks.erase(id);
                 g::userdata.erase(id);
               }
               else if (s[0] == "add_auto_tank")
               {
                 std::lock_guard<std::mutex> l(g::mainloop_mtx);
                 int lvl = std::stoi(std::string{s[1]});
                 int pos_x = std::stoi(std::string{s[2]});
                 int pos_y = std::stoi(std::string{s[3]});
                 game::add_auto_tank(lvl, {pos_x, pos_y});
               }
               else if (s[0] == "run_command")
               {
                 int id = std::stoi(std::string{s[1]});
                 std::string command = std::string{s[2]};
                 cmd::run_command(id, command);
               }
             });
  }
  
  void TankServer::start(int port)
  {
    std::thread th{
        [this, port] { svr.start(port); }
    };
    th.detach();
  }
  
  void TankServer::stop()
  {
    svr.stop();
  }
  
  std::optional<size_t> TankClient::connect(const std::string &addr_, int port_)
  {
    host = addr_;
    port = port_;
    if (cli.connect(addr_, port_) != 0)
    {
      cli.reset();
      return std::nullopt;
    }
    auto ret = cli.send_and_recv("register");
    if (!ret.has_value())
    {
      cli.reset();
      return std::nullopt;
    }
    auto s = utils::split<std::vector<std::string_view>>(*ret, delim::res);
    if (s.size() != 1)
    {
      cli.reset();
      return std::nullopt;
    }
    return std::stoul(std::string{s[0]});
  }
  
  void TankClient::disconnect()
  {
    cli.disconnect();
    cli.reset();
  }
  
  int TankClient::tank_react(tank::NormalTankEvent e)
  {
    std::string content = utils::join(delim::req, "tank_react", g::user_id, static_cast<int>(e));
    return cli.send(content);
  }
  
  std::tuple<int, renderer::Frame> TankClient::update()
  {
    auto beg = std::chrono::steady_clock::now();
    std::string content = utils::join(delim::req, "update", g::user_id,
                                      serialize_zone(g::render_zone.bigger_zone(10)));
    auto ret = cli.send_and_recv(content);
    if (!ret.has_value() || ret->empty())
    {
      g::delay = -1;
      g::output_inited = false;
      return {-1, {}};
    }
    
    auto s = utils::split<std::vector<std::string_view>>(*ret, delim::res);
    int curr_delay = std::chrono::duration_cast<std::chrono::milliseconds>
                          (std::chrono::steady_clock::now() - beg).count() - std::stoi(std::string{s[0]});
    g::delay = static_cast<int>((g::delay + 0.1 * curr_delay) / 1.1);
    renderer::Frame f{
        .map = deserialize_mapview(std::string{s[2]}),
        .tanks = deserialize_tanksview(std::string{s[3]}),
        .changes = deserialize_changes(std::string{s[1]}),
        .zone = g::render_zone.bigger_zone(10)
    };
    auto msgs = deserialize_messages(std::string{s[4]});
    g::userdata[g::user_id].messages.insert(g::userdata[g::user_id].messages.end(),
                                            msgs.begin(), msgs.end());
    return {0, f};
  }
  
  int TankClient::add_auto_tank(size_t lvl)
  {
    auto pos = game::get_available_pos();
    if (!pos.has_value())
    {
      msg::error(g::user_id, "No available space.");
      return -1;
    }
    std::string content = utils::join(delim::req, "add_auto_tank", g::user_id, pos->x, pos->y, lvl);
    return cli.send(content);
  }
  
  int TankClient::run_command(const std::string &str)
  {
    std::string content = utils::join(delim::req, "run_command", g::user_id, str);
    return cli.send(content);
  }
}