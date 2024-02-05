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
#ifndef TANK_LOGGER_H
#define TANK_LOGGER_H
#pragma once

#include "term.h"
#include <string>
#include <fstream>
#include <memory>
#include <chrono>

namespace czh::logger
{
  enum class Severity
  {
    NONE,
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERR,
    CRITICAL,
  };
  
  enum class Output
  {
    file, console, file_and_console, none
  };
  
  std::string get_severity_str(Severity severity);
  
  std::string time_to_str(const std::chrono::system_clock::time_point &now);
  
  class Record
  {
  private:
    std::chrono::system_clock::time_point time_point;
    Severity severity;
    std::string message;
  public:
    Record(std::chrono::system_clock::time_point time_point_, Severity severity_)
        : time_point(time_point_), severity(severity_)
    {
    }
    
    Severity get_severity() const;
    
    std::string get_message() const;
    
    auto get_time_point() const;
    
    template<typename T>
    void add(T &&data)
    {
      using DataType = std::remove_cvref_t<T>;
      if constexpr (std::is_same_v<char, DataType>)
      {
        message += data;
      }
      else if constexpr (std::is_same_v<DataType, std::string>)
      {
        message += data;
      }
      else if constexpr (std::is_same_v<std::decay_t<DataType>, const char *>
                         || std::is_same_v<std::decay_t<DataType>, char *>
          )
      {
        message += data;
      }
      else
      {
        message += std::to_string(data);
      }
    }
    
    template<typename ...Args>
    void add(Args &&...args)
    {
      add_helper(std::forward<Args>(args)...);
    }
  
  private:
    template<typename T, typename ...Args>
    void add_helper(T &&f, Args &&...args)
    {
      add(std::forward<T>(f));
      add_helper(std::forward<Args>(args)...);
    }
    
    template<typename T>
    void add_helper(T &&f)
    {
      add(std::forward<T>(f));
    }
  };
  
  
  class Logger
  {
  private:
    Severity min_severity;
    Output mode;
    std::unique_ptr<std::ofstream> os;
  public:
    Logger()
        : min_severity(Severity::NONE), mode(Output::none), os(nullptr) {}
    
    
    void init(Severity min_severity_, Output mode_, const std::string &filename = "");
    
    void add(const Record &record);
    
    Output get_mode() const;
  };
  
  Logger &get_logger_instance();
  
  void init_logger(Severity min_severity, Output mode, std::string filename = "");
  
  template<typename ...Args>
  void log_helper(Severity severity, Args &&...args)
  {
    if (get_logger_instance().get_mode() == Output::none)
    {
      return;
    }
    Record rec(std::chrono::system_clock::now(), severity);
    rec.add(std::forward<Args>(args)...);
    get_logger_instance().add(rec);
  }
  
  template<typename ...Args>
  void trace(Args &&...args)
  {
    log_helper(Severity::TRACE, std::forward<Args>(args)...);
  }
  
  template<typename ...Args>
  void debug(Args &&...args)
  {
    log_helper(Severity::DEBUG, std::forward<Args>(args)...);
  }
  
  template<typename ...Args>
  void info(Args &&...args)
  {
    log_helper(Severity::INFO, std::forward<Args>(args)...);
  }
  
  template<typename ...Args>
  void warn(Args &&...args)
  {
    log_helper(Severity::WARN, std::forward<Args>(args)...);
  }
  
  template<typename ...Args>
  void error(Args &&...args)
  {
    log_helper(Severity::ERR, std::forward<Args>(args)...);
  }
  
  template<typename ...Args>
  void critical(Args &&...args)
  {
    log_helper(Severity::CRITICAL, std::forward<Args>(args)...);
  }
  
  void output_at_bottom(const std::string &str);
}
#endif