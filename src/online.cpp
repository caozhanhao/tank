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
#include "tank/drawing.h"
#include "tank/utils.h"
#include "tank/serialization.hpp"

#include <string>
#include <string_view>
#include <chrono>
#include <utility>
#include <vector>

namespace czh::g
{
  online::TankServer online_server{};
  online::TankClient online_client{};
  int client_failed_attempts = 0;
  int delay = 0;
  std::mutex online_mtx;
}
namespace czh::online
{
#ifdef _WIN32
  WSADATA wsa_data;
  [[maybe_unused]] int wsa_startup_err = WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
  
  Thpool::Thpool(std::size_t size) : run(true) { add_thread(size); }
  
  Thpool::~Thpool()
  {
    run = false;
    cond.notify_all();
    for (auto &th: pool)
    {
      if (th.joinable())
      {
        th.join();
      }
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
  
  void Thpool::add_task(const std::function<void()> &func)
  {
    utils::tank_assert(run, "Can not add task on stopped Thpool");
    {
      std::lock_guard<std::mutex> lock(th_mutex);
      tasks.emplace([func] { func(); });
    }
    cond.notify_one();
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
  
  std::string Addr::ip() const
  {
    std::string str(16, '\0');
#ifdef _WIN32
    str = inet_ntoa(addr.sin_addr);
#else
    inet_ntop(AF_INET, &addr.sin_addr, str.data(), len);
#endif
    return str;
  }
  
  int Addr::port() const
  {
    return ntohs(addr.sin_port);
  }
  
  std::string Addr::to_string() const
  {
    return ip() + ":" + std::to_string(port());
  }
  
  bool check(bool a)
  {
    if (!a)
    {
      msg::error(g::user_id, strerror(errno));
    }
    return a;
  }
  
  TCPSocket::TCPSocket() : fd(-1)
  {
    init();
  }
  
  TCPSocket::TCPSocket(Socket_t fd_) : fd(fd_) {}
  
  TCPSocket::TCPSocket(TCPSocket &&soc) noexcept: fd(soc.fd)
  {
    soc.fd = -1;
  }
  
  TCPSocket::~TCPSocket()
  {
    if (fd != -1)
    {
#ifdef _WIN32
      closesocket(fd);
#else
      ::close(fd);
#endif
      fd = -1;
    }
  }
  
  std::tuple<TCPSocket, Addr> TCPSocket::accept() const
  {
    Addr addr;
#ifdef _WIN32
    return {std::move(TCPSocket{::accept(fd, reinterpret_cast<sockaddr *>(&addr.addr),
                                         reinterpret_cast<int *> (&addr.len))}), addr};
#else
    return {std::move(
        TCPSocket{::accept(fd, reinterpret_cast<sockaddr *>(&addr.addr), reinterpret_cast<socklen_t *>(&addr.len))}),
            addr};
#endif
  }
  
  Socket_t TCPSocket::get_fd() const { return fd; }
  
  int send_all_data(Socket_t sock, const char *buf, int size)
  {
    while (size > 0)
    {
#ifdef _WIN32
      int s = ::send(sock, buf, size, 0);
#else
      int s = ::send(sock, buf, size, MSG_NOSIGNAL);
#endif
      if (!check(s >= 0)) return -1;
      size -= s;
      buf += s;
    }
    return 0;
  }
  
  int receive_all_data(Socket_t sock, char *buf, int size)
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
  
  int TCPSocket::send(const std::string &str) const
  {
    MsgHeader header{.magic = htonl(HEADER_MAGIC), .content_length = htonl(str.size())};
    if (send_all_data(fd, reinterpret_cast<const char *>(&header), sizeof(MsgHeader)) != 0) return -1;
    if (send_all_data(fd, str.data(), static_cast<int>(str.size())) != 0) return -1;
    return 0;
  }
  
  std::optional<std::string> TCPSocket::recv() const
  {
    MsgHeader header{};
    std::string recv_result;
    if (receive_all_data(fd, reinterpret_cast<char *>(&header), sizeof(MsgHeader)) != 0)
    {
      return std::nullopt;
    }
    
    if (ntohl(header.magic) != HEADER_MAGIC)
    {
      msg::warn(g::user_id, "Ignore invalid data");
      return std::nullopt;
    }
    
    recv_result.resize(ntohl(header.content_length), 0);
    
    if (receive_all_data(fd, reinterpret_cast<char *>(recv_result.data()), static_cast<int>(recv_result.size())) != 0)
    {
      return std::nullopt;
    }
    
    return recv_result;
  }
  
  int TCPSocket::bind(Addr addr) const
  {
    if (!check(::bind(fd, (sockaddr *) &addr.addr, addr.len) == 0))
    {
      return -1;
    }
    return 0;
  }
  
  int TCPSocket::listen() const
  {
    if (!check(::listen(fd, 0) != -1))
    {
      return -1;
    }
    return 0;
  }
  
  int TCPSocket::connect(Addr addr) const
  {
    if (!check(::connect(fd, (sockaddr *) &addr.addr, addr.len) != -1))
    {
      return -1;
    }
    return 0;
  }
  
  std::optional<Addr> TCPSocket::get_peer_addr() const
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
  
  void TCPSocket::reset()
  {
    if (fd != -1)
    {
#ifdef _WIN32
      closesocket(fd);
#else
      ::close(fd);
#endif
    }
    init();
  }
  
  void TCPSocket::init()
  {
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&on), sizeof(on));
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char *>(&on), sizeof(on));
    utils::tank_assert(fd != -1, "socket reset failed.");
  }
  
  Socket_t TCPSocket::release()
  {
    auto f = fd;
    fd = -1;
    return f;
  }
  
  Req::Req(Addr addr_, std::string content_)
      : addr(addr_), content(std::move(content_)) {}
  
  const Addr &Req::get_addr() const { return addr; }
  
  const auto &Req::get_content() const { return content; }
  
  void Res::set_content(const std::string &c) { content = c; }
  
  const auto &Res::get_content() const { return content; }
  
  TCPServer::TCPServer() : thpool(16) {}
  
  TCPServer::TCPServer(std::function<void(const Req &, Res &)> router_)
      : router(std::move(router_)), thpool(16), running(false) {}
  
  void TCPServer::init(const std::function<void(const Req &, Res &)> &router_)
  {
    router = router_;
    running = false;
  }
  
  void TCPServer::start(int port)
  {
    running = true;
    TCPSocket socket;
    check(socket.bind(Addr{port}) == 0);
    check(socket.listen() == 0);
    //sockets.emplace_back(socket.get_fd());
    while (running)
    {
      auto tmp = socket.accept();
      auto &[clnt_socket_, clnt_addr] = tmp;
      utils::tank_assert(clnt_socket_.get_fd() != -1, "socket accept failed");
      auto fd = clnt_socket_.release();
      sockets.emplace_back(fd);
      thpool.add_task(
          [this, fd]
          {
            TCPSocket clnt_socket{fd};
            while (running)
            {
              auto request = clnt_socket.recv();
              if (!request.has_value() || *request == "quit") break;
              Res response;
              auto addr = clnt_socket.get_peer_addr();
              if (!addr.has_value())
              {
                break;
              }
              router(Req{*addr, *request}, response);
              if (!response.get_content().empty())
              {
                check(clnt_socket.send(response.get_content()) == 0);
              }
            }
          });
    }
  }
  
  void TCPServer::stop()
  {
    for (auto &r: sockets)
    {
#ifdef _WIN32
      shutdown(r, SD_BOTH);
#else
      ::shutdown(r, SHUT_RDWR);
#endif
    }
    sockets.clear();
    running = false;
  }
  
  TCPServer::~TCPServer()
  {
    stop();
  }
  
  TCPClient::~TCPClient()
  {
    disconnect();
  }
  
  int TCPClient::disconnect()
  {
    return socket.send("quit");
  }
  
  
  int TCPClient::connect(const std::string &addr, int port)
  {
    return socket.connect({addr, port});
  }
  
  int TCPClient::send(const std::string &str)
  {
    return socket.send(str);
  }
  
  std::optional<std::string> TCPClient::recv()
  {
    return socket.recv();
  }
  
  std::optional<std::string> TCPClient::send_and_recv(const std::string &str)
  {
    int ret = socket.send(str);
    if (ret != 0) return std::nullopt;
    return socket.recv();
  }
  
  void TCPClient::reset()
  {
    socket.reset();
  }
  
  
  template<typename ...Args>
  std::string make_request(const std::string &cmd, Args &&...args)
  {
    return ser::serialize(cmd, ser::serialize(std::forward<Args>(args)...));
  }
  
  template<typename ...Args>
  std::string make_response(Args &&...args)
  {
    return ser::serialize(std::forward<Args>(args)...);
  }
  
  void TankServer::init()
  {
    if (svr != nullptr)
    {
      svr->stop();
      delete svr;
    }
    svr = new TCPServer{};
    svr->init([](const Req &req, Res &res)
              {
                auto[cmd, args] = ser::deserialize<std::string, std::string>(req.get_content());
                if (cmd == "tank_react")
                {
                  auto[id, event] = ser::deserialize<size_t, tank::NormalTankEvent>(args);
                  game::tank_react(id, event);
                }
                else if (cmd == "update")
                {
                  auto[id, zone] = ser::deserialize<size_t, map::Zone>(args);
                  auto beg = std::chrono::steady_clock::now();
                  std::lock_guard<std::mutex> l(g::mainloop_mtx);
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
                  res.set_content(make_response(d.count(), changes, drawing::extract_tanks(),
                                                g::userdata[id].messages, drawing::extract_map(zone)));
                  g::userdata[id].messages = decltype(g::userdata[id].messages){};
                  g::userdata[id].map_changes.clear();
                  g::userdata[id].last_update = std::chrono::steady_clock::now();
                }
                else if (cmd == "register")
                {
                  auto[port, screen_width, screen_height] =
                  ser::deserialize<int, size_t, size_t>(args);
                  std::lock_guard<std::mutex> l(g::mainloop_mtx);
                  auto id = game::add_tank();
                  g::userdata[id] = g::UserData{
                      .user_id = g::user_id,
                      .ip = req.get_addr().ip(),
                      .port = port,
                      .screen_width = screen_width,
                      .screen_height = screen_height
                  };
                  g::userdata[id].last_update = std::chrono::steady_clock::now();
                  msg::info(-1, req.get_addr().ip() + " connected as " + std::to_string(id));
                  res.set_content(make_response(id));
                }
                else if (cmd == "deregister")
                {
                  auto id = ser::deserialize<size_t>(args);
                  std::lock_guard<std::mutex> l(g::mainloop_mtx);
                  msg::info(-1, req.get_addr().ip() + " (" + std::to_string(id) + ") disconnected.");
                  g::tanks[id]->kill();
                  g::tanks[id]->clear();
                  delete g::tanks[id];
                  g::tanks.erase(id);
                  g::userdata.erase(id);
                }
                else if (cmd == "add_auto_tank")
                {
                  auto[lvl, pos] = ser::deserialize<int, map::Pos>(args);
                  std::lock_guard<std::mutex> l(g::mainloop_mtx);
                  game::add_auto_tank(lvl, pos);
                }
                else if (cmd == "run_command")
                {
                  auto[id, command] = ser::deserialize<size_t, std::string>(args);
                  cmd::run_command(id, command);
                }
              });
  }
  
  void TankServer::start(int port)
  {
    std::thread th{
        [this, port] { svr->start(port); }
    };
    th.detach();
  }
  
  void TankServer::stop()
  {
    svr->stop();
    delete svr;
    svr = nullptr;
  }
  
  TankServer::~TankServer()
  {
    delete svr;
  }
  
  std::optional<size_t> TankClient::connect(const std::string &addr_, int port_)
  {
    std::lock_guard<std::mutex> l(g::online_mtx);
    host = addr_;
    port = port_;
    if (cli->connect(addr_, port_) != 0)
    {
      cli->reset();
      return std::nullopt;
    }
    
    std::string content = make_request("register", 8080, g::screen_width, g::screen_height);
    auto ret = cli->send_and_recv(content);
    if (!ret.has_value())
    {
      cli->reset();
      return std::nullopt;
    }
    auto id = ser::deserialize<size_t>(*ret);
    return id;
  }
  
  void TankClient::disconnect()
  {
    std::lock_guard<std::mutex> l(g::online_mtx);
    std::string content = make_request("deregister", g::user_id);
    cli->send(content);
    cli->disconnect();
    cli->reset();
    delete cli;
    cli = nullptr;
  }
  
  int TankClient::tank_react(tank::NormalTankEvent e)
  {
    std::lock_guard<std::mutex> l(g::online_mtx);
    std::string content = make_request("tank_react", g::user_id, e);
    return cli->send(content);
  }
  
  int TankClient::update()
  {
    std::lock_guard<std::mutex> l(g::online_mtx);
    auto beg = std::chrono::steady_clock::now();
    std::string content = make_request("update", g::user_id, g::visible_zone.bigger_zone(10));
    auto ret = cli->send_and_recv(content);
    if (!ret.has_value())
    {
      g::delay = -1;
      g::output_inited = false;
      return -1;
    }
    int delay;
    auto old_seed = g::snapshot.map.seed;
    std::priority_queue<msg::Message> msgs;
    std::tie(delay, g::snapshot.changes, g::snapshot.tanks, msgs, g::snapshot.map)
        = ser::deserialize<int, std::set<map::Pos>, std::map<size_t, drawing::TankView>,
        std::priority_queue<msg::Message>,
        drawing::MapView>(*ret);
    int curr_delay = std::chrono::duration_cast<std::chrono::milliseconds>
                         (std::chrono::steady_clock::now() - beg).count() - delay;
    g::delay = static_cast<int>((g::delay + 0.1 * curr_delay) / 1.1);
    while (!msgs.empty())
    {
      g::userdata[g::user_id].messages.push(msgs.top());
      msgs.pop();
    }
    if (old_seed != g::snapshot.map.seed) g::output_inited = false;
    return 0;
  }
  
  int TankClient::add_auto_tank(size_t lvl)
  {
    std::lock_guard<std::mutex> l(g::online_mtx);
    auto pos = game::get_available_pos();
    if (!pos.has_value())
    {
      msg::error(g::user_id, "No available space.");
      return -1;
    }
    std::string content = make_request("add_auto_tank", g::user_id, pos, lvl);
    return cli->send(content);
  }
  
  int TankClient::run_command(const std::string &str)
  {
    std::lock_guard<std::mutex> l(g::online_mtx);
    std::string content = make_request("run_command", g::user_id, str);
    return cli->send(content);
  }
  
  TankClient::~TankClient()
  {
    delete cli;
  }
  
  void TankClient::init()
  {
    if (cli != nullptr)
    {
      cli->disconnect();
      delete cli;
    }
    cli = new TCPClient{};
  }
}