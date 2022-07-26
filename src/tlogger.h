#pragma once
#include "tterm.h"
#include <stdexcept>
#include <string>
#include <iostream>
#include <chrono>
#include <ctime>
#include <cstdio>

#define CZH_STRINGFY(x) _CZH_STRINGFY(x)
#define _CZH_STRINGFY(x) #x
#define CZH_LOCATION  __FILE__ ":" CZH_STRINGFY(__LINE__)

#define _CZH_LOGGER(level, msg) \
czh::logger::output("[" + std::to_string(level) + "] " + czh::logger::get_time()\
+ " " + std::to_string(CZH_LOCATION) \
+ " " msg);

#define CZH_WARNING(msg) do{_CZH_LOGGER("WARNING", msg);}while(0);
#define CZH_DEBUG(msg) do{_CZH_LOGGER("DEBUG", msg);}while(0);
#define CZH_FATAL(msg, errorcode) do{_CZH_LOGGER("FATAL", msg); exit(errorcode);} while(0);
#define CZH_NOTICE(msg) do{czh::logger::output("[NOTICE] " + czh::logger::get_time()\
+ std::string(" ") + msg); }while(0);
namespace czh::logger
{
  void output(const std::string& str)
  {
    term::move_cursor(term::TermPos(0, term::get_height() - 1));
    term::output(str + std::string(term::get_width() - str.size(), ' '));
  }

  std::string get_time()
  {
    auto tt = std::chrono::system_clock::to_time_t
    (std::chrono::system_clock::now());
    struct tm* ptm = localtime(&tt);
    char date[60] = { 0 };
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
      (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
      (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
  }
}