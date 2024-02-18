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
#ifndef TANK_ONLINE_H
#define TANK_ONLINE_H
#pragma once

#include "tank.h"
#include "game_map.h"
#include "drawing.h"
#include "utils.h"

#ifdef _WIN32

#include <WinSock2.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")
#else

#include <unistd.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#endif

#include <cstring>
#include <functional>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <memory>
#include <exception>
#include <type_traits>

namespace czh::online
{
  namespace delim
  {
    constexpr char req = '<'; // Request
    constexpr char res = '>'; // Response
    constexpr char pv = '!'; // PointView
    constexpr char mv = '@'; // MapView
    constexpr char tv = '$'; // TankView
    constexpr char tvs = '%'; // TankViews
    constexpr char zo = '^'; // Zone
    constexpr char c = '&'; // Change
    constexpr char cs = '*'; // Changes
    constexpr char m = '{'; // Msg
    constexpr char ms = '}'; // Msgs
  }
  
  constexpr int HEADER_MAGIC = 0x18273645;

#ifdef _WIN32
  extern WSADATA wsa_data;
#endif
  
  class Thpool
  {
  private:
    using Task = std::function<void()>;
    std::vector<std::thread> pool;
    std::queue<Task> tasks;
    std::atomic<bool> run;
    std::mutex th_mutex;
    std::exception_ptr err_ptr;
    std::condition_variable cond;
  public:
    explicit Thpool(std::size_t size);
    
    ~Thpool();
    
    void add_task(const std::function<void()> &func);
    
    void add_thread(std::size_t num);
  };
  
  struct MsgHeader
  {
    uint32_t magic;
    uint32_t content_length;
  };
  
  struct Addr
  {
    struct sockaddr_in addr;
#ifdef _WIN32
    int len;
#else
    socklen_t len;
#endif
    
    Addr();
    
    Addr(struct sockaddr_in addr_, decltype(len) len_);
    
    Addr(const std::string &ip, int port);
    
    explicit Addr(int port);
    
    [[nodiscard]] std::string to_string() const;
    
    [[nodiscard]] int port() const;
    
    [[nodiscard]] std::string ip() const;
  };


#ifdef _WIN32
  using Socket_t = SOCKET;
#else
  using Socket_t = int;
#endif

//  class UDPSocket
//  {
//  private:
//    Socket_t fd;
//    Addr addr;
//  public:
//    UDPSocket();
//
//    UDPSocket(const UDPSocket &) = delete;
//
//    ~UDPSocket();
//
//    Socket_t release();
//
//    int send(Addr to, const std::string &str) const;
//
//    std::optional<std::tuple<Addr, std::string>>  recv() const;
//
//    int bind(Addr addr) const;
//
//    std::optional<Addr> get_peer_addr() const;
//
//  };
  
  class TCPSocket
  {
  private:
    Socket_t fd;
  public:
    TCPSocket();
    
    explicit TCPSocket(Socket_t fd_);
    
    TCPSocket(const TCPSocket &) = delete;
    
    TCPSocket(TCPSocket &&soc) noexcept;
    
    ~TCPSocket();
    
    [[nodiscard]] std::tuple<TCPSocket, Addr> accept() const;
    
    [[nodiscard]] Socket_t get_fd() const;
    
    Socket_t release();
    
    [[nodiscard]] int send(const std::string &str) const;
    
    [[nodiscard]] std::optional<std::string> recv() const;
    
    [[nodiscard]] int bind(Addr addr) const;
    
    [[nodiscard]] int listen() const;
    
    [[nodiscard]] int connect(Addr addr) const;
    
    [[nodiscard]] std::optional<Addr> get_peer_addr() const;
    
    void reset();
    
    void init();
  };
  
  class Req
  {
  private:
    Addr addr;
    std::string content;
  public:
    Req(Addr addr_, std::string content_);
    
    [[nodiscard]] const Addr &get_addr() const;
    
    [[nodiscard]] const auto &get_content() const;
  };
  
  class Res
  {
  private:
    std::string content;
  public:
    Res() = default;
    
    void set_content(const std::string &c);
    
    [[nodiscard]] const auto &get_content() const;
  };
  
  class TCPServer
  {
  private:
    bool running;
    std::function<void(const Req &, Res &)> router;
    Thpool thpool;
  public:
    TCPServer();
    
    explicit TCPServer(std::function<void(const Req &, Res &)> router_);
    
    void init(const std::function<void(const Req &, Res &)> &router_);
    
    void start(int port);
    
    void stop();
  };
  
  class TCPClient
  {
  private:
    TCPSocket socket;
  public:
    TCPClient() = default;
    
    ~TCPClient();
    
    int connect(const std::string &addr, int port);
    
    int disconnect();
    
    std::optional<std::string> send_and_recv(const std::string &str);
    
    int send(const std::string &str);
    
    std::optional<std::string> recv();
    
    void reset();
  };
  
  class TankServer
  {
  private:
    TCPServer *svr;
    //UDPSocket* udp;
  public:
    TankServer() = default;
    
    ~TankServer();
    
    void init();
    
    void start(int port);
    
    void stop();
  };
  
  class TankClient
  {
  private:
    std::string host;
    int port;
    TCPClient *cli;
    //UDPSocket* udp;
  public:
    TankClient() = default;
    
    ~TankClient();
    
    void init();
    
    std::optional<size_t> connect(const std::string &addr_, int port_);
    
    void disconnect();
    
    int tank_react(tank::NormalTankEvent e);
    
    int update();
    
    int add_auto_tank(size_t l);
    
    int run_command(const std::string &str);
  };
}
#endif