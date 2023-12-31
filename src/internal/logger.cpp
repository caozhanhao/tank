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
#include "internal/term.h"
#include "internal/logger.h"
#include <string>
#include <chrono>
#include <ctime>
#include <cstdio>
namespace czh::logger
{
  std::string get_severity_str(Severity severity)
  {
    switch (severity)
    {
      case Severity::TRACE:
        return "[TRACE]";
      case Severity::DEBUG:
        return "[DEBUG]";
      case Severity::INFO:
        return "[INFO]";
      case Severity::WARN:
        return "\x1B[93m[WARNING]\x1B[0m\x1B[0K";
      case Severity::ERR:
        return "\x1B[91m[ERROR]\x1B[0m\x1B[0K";
      case Severity::CRITICAL:
        return "\x1B[97m\x1B[41m[CRITICAL]\x1B[0m\x1B[0K";
      default:
        return "[NONE]";
    }
  }
  
  std::string time_to_str(const std::chrono::system_clock::time_point &now)
  {
    auto tt = std::chrono::system_clock::to_time_t(now);
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int) ptm->tm_year + 1900, (int) ptm->tm_mon + 1, (int) ptm->tm_mday,
            (int) ptm->tm_hour, (int) ptm->tm_min, (int) ptm->tm_sec);
    return {date};
  }
  
  Severity Record::get_severity() const { return severity; }
  
  std::string Record::get_message() const { return message; }
  
  auto Record::get_time_point() const { return time_point; }
  void Logger::init(Severity min_severity_, Output mode_, const std::string &filename)
  {
    min_severity = min_severity_;
    mode = mode_;
    if(filename != "")
    {
      os = std::make_unique<std::ofstream>(filename);
      if (!os->is_open())
        throw std::runtime_error("Open log file failed");
    }
    else
      os = nullptr;
  }
  
  void output_at_bottom(const std::string &str)
  {
    term::move_cursor(term::TermPos(0, term::get_height() - 1));
    int a = term::get_width() - str.size();
    if (a > 0)
    {
      term::output(str + std::string(a, ' '));
    }
    else
    {
      term::output(str);
    }
  }
  
  void Logger::add(const Record &record)
  {
    if (record.get_severity() < min_severity) return;
    std::string str;
    str += get_severity_str(record.get_severity()) + " ";
    if(record.get_severity() > Severity::INFO)
    {
      std::string time = time_to_str(record.get_time_point());
      str += time + " ";
    }
    str += record.get_message();
    if (mode == Output::file || mode == Output::file_and_console)
    {
      *os << str;
      os->flush();
    }
    
    if (mode == Output::console || mode == Output::file_and_console)
    {
      output_at_bottom(str);
    }
    
    if (record.get_severity() == Severity::CRITICAL)
    {
      std::terminate();
    }
  }
  
  Output Logger::get_mode() const { return mode; }
  
  Logger &get_logger_instance()
  {
    static Logger instance;
    return instance;
  }
  
  void init_logger(Severity min_severity, Output mode, std::string filename)
  {
    get_logger_instance().init(min_severity, mode, filename);
  }
}