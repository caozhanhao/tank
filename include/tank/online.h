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
#include "renderer.h"
#include "utils.h"

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <sys/types.h>

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
  WSADATA wsa_data;
  [[maybe_unused]] int wsa_startup_err = WSAStartup(MAKEWORD(2,2),&wsa_data);
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
    
    template<typename Func, typename... Args>
    auto add_task(Func &&f, Args &&... args)
    -> std::future<typename std::result_of<Func(Args...)>::type>
    {
      utils::tank_assert(run, "Can not add task on stopped Thpool");
      using ret_type = typename std::result_of<Func(Args...)>::type;
      auto task = std::make_shared<std::packaged_task<ret_type() >>
          (std::bind(std::forward<Func>(f),
                     std::forward<Args>(args)...));
      std::future<ret_type> ret = task->get_future();
      {
        std::lock_guard<std::mutex> lock(th_mutex);
        tasks.emplace([task] { (*task)(); });
      }
      cond.notify_one();
      return ret;
    }
    
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
    
    Addr(int port);
    
    std::string to_string() const;
    
    int port() const;
    
    std::string ip() const;
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
    
    TCPSocket(Socket_t fd_);
    
    TCPSocket(const TCPSocket &) = delete;
    
    TCPSocket(TCPSocket &&soc) noexcept;
    
    ~TCPSocket();
    
    std::tuple<TCPSocket, Addr> accept() const;
    
    Socket_t get_fd() const;
    
    Socket_t release();
    
    int send(const std::string &str) const;
    
    std::optional<std::string> recv() const;
    
    int bind(Addr addr) const;
    
    int listen() const;
    
    int connect(Addr addr) const;
    
    std::optional<Addr> get_peer_addr() const;
    
    void reset();
  };
  
  class Req
  {
  private:
    Addr addr;
    std::string content;
  public:
    Req(Addr addr_, std::string content_);
    
    const Addr& get_addr() const ;
    
    const auto& get_content() const;
  };
  
  class Res
  {
  private:
    std::string content;
  public:
    Res() = default;
    
    void set_content(const std::string c) ;
    
    const auto& get_content() const ;
  };
  
  class TCPServer
  {
  private:
    bool running;
    std::function<void(const Req &, Res &)> router;
    Thpool thpool;
  public:
    TCPServer();
    TCPServer(const std::function<void(const Req &, Res &)> &router_);
    void init(const std::function<void(const Req &, Res &)> &router_);
    
    void start(int port);
    void stop();
  };
  
  class TCPClient
  {
  private:
    TCPSocket socket;
  public:
    TCPClient() =default;
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
    TCPServer* svr;
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
    TCPClient* cli;
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